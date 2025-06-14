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
