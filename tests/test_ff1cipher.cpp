#include <catch2/catch_test_macros.hpp>
#include "FF1Cipher.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

static std::vector<std::string> load_wordlist() {
    std::filesystem::path file = std::filesystem::current_path() / "data" / "google-10000-english.txt";
    std::ifstream in(file);
    if (!in) throw std::runtime_error("Failed to open word list");
    std::vector<std::string> words;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) words.push_back(line);
    }
    return words;
}

static std::vector<uint8_t> fixed_key_128() {
    return std::vector<uint8_t>{
        0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB,
        0xCC, 0xDD, 0xEE, 0xFF
    };
}

static std::vector<uint8_t> fixed_tweak() {
    return std::vector<uint8_t>{ 0xDE, 0xAD, 0xBE, 0xEF };
}

// Alphabet: uppercase + lowercase ASCII letters only
static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static std::unordered_map<char, unsigned> alpha_idx = []{
    std::unordered_map<char, unsigned> map;
    for (unsigned i = 0; i < alphabet.size(); ++i)
        map[alphabet[i]] = i;
    return map;
}();
static const unsigned radix = alphabet.size();

TEST_CASE("FF1Cipher 10000 word roundtrip using A-Za-z alphabet", "[ff1cipher][wordlist]") {
    const auto words = load_wordlist();
    REQUIRE(words.size() >= 10000);

    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

    std::vector<std::vector<unsigned>> encoded;
    encoded.reserve(words.size());

    // Encode: map each character to index in alphabet
    for (const std::string& word : words) {
        std::vector<unsigned> digits;
        for (char c : word) {
            REQUIRE(alpha_idx.count(c));
            digits.push_back(alpha_idx[c]);
        }
        encoded.push_back(cipher.encrypt(std::move(digits)));
    }

    // Decode and verify
    for (size_t i = 0; i < words.size(); ++i) {
        std::vector<unsigned> decoded_digits = cipher.decrypt(std::move(encoded[i]));

        std::string restored;
        for (unsigned d : decoded_digits) {
            REQUIRE(d < radix);
            restored.push_back(alphabet[d]);
        }
        REQUIRE(restored == words[i]);
    }
}

TEST_CASE("FF1Cipher roundtrip simple (A-Za-z)", "[ff1cipher]") {
    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

    std::vector<unsigned int> data = {0, 1, 2, 3, 4}; // A, B, C, D, E
    auto encrypted = cipher.encrypt(std::move(data));
    auto decrypted = cipher.decrypt(std::move(encrypted));

    REQUIRE(decrypted == std::vector<unsigned int>{0, 1, 2, 3, 4});
}

TEST_CASE("FF1Cipher 10000 word roundtrip, collision check, and file output (A-Za-z)", "[ff1cipher][collision][output]") {
    const auto words = load_wordlist();
    REQUIRE(words.size() >= 10000);

    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

    std::unordered_set<std::string> seen_encodings;
    std::vector<std::pair<std::string, std::string>> word_enc_pairs;

    for (const std::string& word : words) {
        std::vector<unsigned> digits;
        for (char c : word) {
            REQUIRE(alpha_idx.count(c));
            digits.push_back(alpha_idx[c]);
        }
        auto enc_digits = cipher.encrypt(std::move(digits));
        std::string encoded;
        for (unsigned d : enc_digits)
            encoded.push_back(alphabet[d]);

        auto [it, inserted] = seen_encodings.insert(encoded);
        REQUIRE(inserted);

        word_enc_pairs.emplace_back(word, encoded);
    }

    std::ofstream out("ff1_word_encodings_alpha.tsv");
    for (const auto& [word, encoded] : word_enc_pairs)
        out << word << "\t" << encoded << "\n";
    out.close();

    std::cout << "[output] Encoded results written to ff1_word_encodings_alpha.tsv" << std::endl;
}

TEST_CASE("FF1Cipher different inputs encrypt differently (A-Za-z)", "[ff1cipher]") {
    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

    std::vector<unsigned int> a = {0, 1, 2}; // A B C
    std::vector<unsigned int> b = {2, 1, 0}; // C B A

    auto ea = cipher.encrypt(std::move(a));
    auto eb = cipher.encrypt(std::move(b));

    REQUIRE(ea != eb);
}

TEST_CASE("FF1Cipher rejects invalid key sizes", "[ff1cipher]") {
    std::vector<uint8_t> short_key(8, 0x01); // 64-bit

    REQUIRE_THROWS_AS(FF1Cipher(short_key, fixed_tweak(), radix), std::invalid_argument);
}

TEST_CASE("FF1Cipher handles empty input (A-Za-z)", "[ff1cipher]") {
    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

    std::vector<unsigned int> empty;
    auto enc = cipher.encrypt(std::move(empty));
    auto dec = cipher.decrypt(std::move(enc));

    REQUIRE(dec.empty());
}

TEST_CASE("FF1Cipher works across radix values (small, A-Za-z)", "[ff1cipher][radix]") {
    for (unsigned small_radix : {2, 10, 16, 26, 52}) {
        FF1Cipher cipher(fixed_key_128(), fixed_tweak(), small_radix);

        std::vector<unsigned int> digits;
        for (unsigned i = 0; i < 12; ++i)
            digits.push_back(i % small_radix);

        auto encrypted = cipher.encrypt(std::move(digits));
        auto decrypted = cipher.decrypt(std::move(encrypted));

        for (unsigned i = 0; i < 12; ++i)
            REQUIRE(decrypted[i] == (i % small_radix));
    }
}

TEST_CASE("FF1Cipher 10000 word benchmark encode/decode (A-Za-z)", "[ff1cipher][benchmark]") {
    const auto words = load_wordlist();
    REQUIRE(words.size() >= 10000);

    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

    std::vector<std::vector<unsigned>> encoded;
    encoded.reserve(words.size());
    std::vector<std::vector<unsigned>> decoded_digits(words.size());

    // Prepare: encode chars to indices
    std::vector<std::vector<unsigned>> digit_words;
    digit_words.reserve(words.size());
    for (const std::string& word : words) {
        std::vector<unsigned> digits;
        for (char c : word) {
            REQUIRE(alpha_idx.count(c));
            digits.push_back(alpha_idx[c]);
        }
        digit_words.push_back(std::move(digits));
    }

    // Benchmark encode
    auto t1 = std::chrono::high_resolution_clock::now();
    for (auto& digits : digit_words) {
        encoded.push_back(cipher.encrypt(std::move(digits)));
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    double encode_sec = std::chrono::duration<double>(t2 - t1).count();
    std::cout << "[encode] " << encoded.size() << " items in " << encode_sec << "s = "
              << (encoded.size() / encode_sec) << " ops/s\n";

    // Benchmark decode
    auto t3 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < encoded.size(); ++i) {
        decoded_digits[i] = cipher.decrypt(std::move(encoded[i]));
    }
    auto t4 = std::chrono::high_resolution_clock::now();

    double decode_sec = std::chrono::duration<double>(t4 - t3).count();
    std::cout << "[decode] " << decoded_digits.size() << " items in " << decode_sec << "s = "
              << (decoded_digits.size() / decode_sec) << " ops/s\n";

    // Sanity check
    for (size_t i = 0; i < words.size(); ++i) {
        std::string restored;
        for (unsigned d : decoded_digits[i])
            restored.push_back(alphabet[d]);
        REQUIRE(restored == words[i]);
    }
}
