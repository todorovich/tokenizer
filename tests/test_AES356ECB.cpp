#include <catch2/catch_test_macros.hpp>
#include "AES256ECB.hpp"
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <iostream>

static std::vector<std::string> load_words() {
	std::filesystem::path path = std::filesystem::current_path() / "data" / "google-10000-english.txt";
	std::ifstream in(path);
	if (!in) throw std::runtime_error("Missing word list: " + path.string());

	std::vector<std::string> words;
	std::string word;
	while (std::getline(in, word)) {
		if (!word.empty()) words.push_back(word);
	}
	return words;
}

TEST_CASE("AES256ECB encode/decode is deterministic and reversible", "[aes256ecb]") {
	const AES256ECB aes("0123456789ABCDEF0123456789ABCDEF");
	const auto words = load_words();
	REQUIRE(words.size() >= 10000);

	std::unordered_set<std::string> encoded_set;
	std::vector<std::pair<std::string, std::string>> outputs;
    outputs.reserve(words.size());

	for (const auto& word : words) {
		std::string encoded = aes.encode(word);
		std::string decoded = aes.decode(encoded);

		REQUIRE(decoded == word);
		REQUIRE(encoded_set.insert(encoded).second); // no collision

		outputs.emplace_back(word, encoded);
	}

	std::ofstream out("aes256ecb_outputs.txt");
	for (const auto& [orig, enc] : outputs)
		out << orig << '\t' << enc << '\n';
}

TEST_CASE("AES256ECB encode/decode performance", "[aes256ecb][benchmark]") {
    const AES256ECB aes("0123456789ABCDEF0123456789ABCDEF");
    const auto words = load_words();
    REQUIRE(words.size() >= 10000);

    std::vector<std::string> encoded;
    encoded.reserve(words.size());
    const auto start_enc = std::chrono::high_resolution_clock::now();
    {
        for (const auto& word : words)
        {
            encoded.push_back(aes.encode(word));
        }
    }
    const auto end_enc = std::chrono::high_resolution_clock::now();

    std::vector<std::string> decoded;
    decoded.reserve(words.size());
    const auto start_dec = std::chrono::high_resolution_clock::now();
    {
        for (const auto& enc : encoded)
        {
            decoded.push_back(aes.decode(enc));
        }
    }
    const auto end_dec = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < words.size(); ++i)
        REQUIRE(decoded[i] == words[i]);

    using namespace std::chrono;
    auto encode_time = duration_cast<duration<double>>(end_enc - start_enc).count();
    auto decode_time = duration_cast<duration<double>>(end_dec - start_dec).count();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[encode] AES256ECB encoded " << words.size() << " items in " << encode_time
              << "s = " << (words.size() / encode_time) << " ops/s\n";
    std::cout << "[decode] AES256ECB decoded " << words.size() << " items in " << decode_time
              << "s = " << (words.size() / decode_time) << " ops/s\n";
}
