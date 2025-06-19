#include <catch2/catch_test_macros.hpp>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <random>
#include <thread>
#include <future>

#include "Curl.hpp"
#include "WebServer.hpp"

std::vector<std::string> load_wordlist() {
    std::filesystem::path file = std::filesystem::current_path() / "data" / "google-10000-english.txt";
    std::cerr << "Trying to open: " << file << "\n";

    std::ifstream in(file);
    if (!in) throw std::runtime_error("Failed to open word list");

    std::vector<std::string> words;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) words.push_back(line);
    }
    return words;
}

const auto wordlist = load_wordlist();
static std::unordered_map<std::string, std::string> encoded_to_original;
static std::unordered_map<std::string, std::string> original_to_decoded;

void handle_encode(long status, const std::string& request_body, const std::string& response_body)
{
    REQUIRE(status == 200);
    std::string original = request_body;
    if (!original.empty() && original.back() == '\n')
        original.pop_back();

    std::string encoded = response_body;
    if (!encoded.empty() && encoded.back() == '\n')
        encoded.pop_back();

    auto [it, inserted] = encoded_to_original.emplace(encoded, original);
    REQUIRE((inserted || it->second == original));
}

void handle_decode(long status, const std::string& request_body, const std::string& response_body)
{
    REQUIRE(status == 200);

    std::string encoded = request_body;
    if (!encoded.empty() && encoded.back() == '\n')
        encoded.pop_back();

    std::string decoded = response_body;
    if (!decoded.empty() && decoded.back() == '\n')
        decoded.pop_back();

    const std::string& original = encoded_to_original.at(encoded);
    original_to_decoded[original] = decoded;
}

void write_encoded_results_json(
    const std::unordered_map<std::string, std::string>& result_map,
    const std::string& path = "multi_encoded_outputs.json")
{
    std::ofstream out(path);
    out << "[\n";
    bool first = true;
    for (const auto& [decoded, encoded] : result_map)
    {
        if (!first) out << ",\n";
        first = false;
        out << "  { \"decoded\": \"" << decoded << "\", \"encoded\": \"" << encoded << "\" }";
    }
    out << "\n]\n";
}

TEST_CASE("startup", "[http][roundtrip][multi]") {
    // Start server and wait for readiness
    std::promise<void> server_ready;
    std::future<void> ready_future = server_ready.get_future();

    const auto start = std::chrono::steady_clock::now();

    std::thread server_thread([&] {
        run_server_thread([&] {
            server_ready.set_value();
        });
    });

    ready_future.wait(); // Wait until server is listening
    const auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Clean shutdown
    pthread_cancel(server_thread.native_handle()); // forcibly cancel uWebSockets loop
    server_thread.join();

    std::cout << "Startup Time: " << elapsed.count() << " microseconds\n";
}

TEST_CASE("libcurl encode/decode roundtrip with timing and collision check", "[http][roundtrip][multi]") {
    // Start server and wait for readiness
    std::promise<void> server_ready;
    std::future<void> ready_future = server_ready.get_future();

    std::thread server_thread([&] {
        run_server_thread([&] {
            server_ready.set_value();
        });
    });

    ready_future.wait(); // Wait until server is listening

    CurlGlobal curl_init;

    const std::string encode_url = "http://127.0.0.1:8080/encode/aes256ecb";
    const std::string decode_url = "http://127.0.0.1:8080/decode/aes256ecb";
    REQUIRE(wordlist.size() >= 10000);

    encoded_to_original.clear();
    original_to_decoded.clear();

    CurlMulti multi{1000};

    // Encode phase
    size_t i = 0;
    const auto encode_start = std::chrono::steady_clock::now();

    while (i < wordlist.size()) {
        while (CurlRequest* req = multi.try_next_request()) {
            req->set_url(encode_url);
            req->set_post_body(wordlist[i++] + "\n");
            multi.enqueue(*req, handle_encode);
            if (i == wordlist.size()) break;
        }
        multi.run();
    }

    const auto encode_end = std::chrono::steady_clock::now();
    const double encode_elapsed = std::chrono::duration<double>(encode_end - encode_start).count();
    std::cout << "[encode] " << encoded_to_original.size() << " items in "
              << encode_elapsed << "s = "
              << (encoded_to_original.size() / encode_elapsed) << " req/s\n";

    // Decode phase
    std::vector<std::string> encoded_values;
    encoded_values.reserve(encoded_to_original.size());
    for (const auto& [encoded, _] : encoded_to_original)
        encoded_values.push_back(encoded);

    i = 0;
    const auto decode_start = std::chrono::steady_clock::now();

    while (i < encoded_values.size()) {
        while (CurlRequest* req = multi.try_next_request()) {
            req->set_url(decode_url);
            req->set_post_body(encoded_values[i++] + "\n");
            multi.enqueue(*req, handle_decode);
            if (i == encoded_values.size()) break;
        }
        multi.run();
    }

    const auto decode_end = std::chrono::steady_clock::now();
    const double decode_elapsed = std::chrono::duration<double>(decode_end - decode_start).count();
    std::cout << "[decode] " << original_to_decoded.size() << " items in "
              << decode_elapsed << "s = "
              << (original_to_decoded.size() / decode_elapsed) << " req/s\n";

    REQUIRE(original_to_decoded.size() == wordlist.size());

    for (const std::string& word : wordlist) {
        REQUIRE(original_to_decoded.at(word) == word);
    }

    write_encoded_results_json(original_to_decoded, "original_to_decoded.json");

    // Clean shutdown
    pthread_cancel(server_thread.native_handle()); // forcibly cancel uWebSockets loop
    server_thread.join();
}

TEST_CASE("libcurl encode/decode roundtrip with timing and collision check, server already up", "[http][roundtrip][multi]") {
    CurlGlobal curl_init;

    const std::string encode_url = "http://www.todorovich.net/encode/aes256ecb";
    const std::string decode_url = "http://www.todorovich.net/decode/aes256ecb";
    REQUIRE(wordlist.size() >= 10000);

    encoded_to_original.clear();
    original_to_decoded.clear();

    CurlMulti multi;

    // Encode phase
    size_t i = 0;
    const auto encode_start = std::chrono::steady_clock::now();

    while (i < wordlist.size()) {
        while (CurlRequest* req = multi.try_next_request()) {
            req->set_url(encode_url);
            req->set_post_body(wordlist[i++] + "\n");
            multi.enqueue(*req, handle_encode);
            if (i == wordlist.size()) break;
        }
        multi.run();
    }

    const auto encode_end = std::chrono::steady_clock::now();
    const double encode_elapsed = std::chrono::duration<double>(encode_end - encode_start).count();
    std::cout << "[encode] " << encoded_to_original.size() << " items in "
              << encode_elapsed << "s = "
              << (encoded_to_original.size() / encode_elapsed) << " req/s\n";

    // Decode phase
    std::vector<std::string> encoded_values;
    encoded_values.reserve(encoded_to_original.size());
    for (const auto& [encoded, _] : encoded_to_original)
        encoded_values.push_back(encoded);

    i = 0;
    const auto decode_start = std::chrono::steady_clock::now();

    while (i < encoded_values.size()) {
        while (CurlRequest* req = multi.try_next_request()) {
            req->set_url(decode_url);
            req->set_post_body(encoded_values[i++] + "\n");
            multi.enqueue(*req, handle_decode);
            if (i == encoded_values.size()) break;
        }
        multi.run();
    }

    const auto decode_end = std::chrono::steady_clock::now();
    const double decode_elapsed = std::chrono::duration<double>(decode_end - decode_start).count();
    std::cout << "[decode] " << original_to_decoded.size() << " items in "
              << decode_elapsed << "s = "
              << (original_to_decoded.size() / decode_elapsed) << " req/s\n";

    REQUIRE(original_to_decoded.size() == wordlist.size());

    for (const std::string& word : wordlist) {
        REQUIRE(original_to_decoded.at(word) == word);
    }

    write_encoded_results_json(original_to_decoded, "original_to_decoded.json");
}