#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "../src/tokenizer.hpp"

#include <fstream>
#include <sstream>
#include <random>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <filesystem>

std::vector<std::string> generate_random_tokens(size_t count) {
	static std::vector<std::string> dictionary;
	static bool loaded = false;
	if (!loaded) {
		std::filesystem::path path = std::filesystem::current_path().parent_path() / "data" / "google-10000-english.txt";
		std::ifstream infile(path);
		if (!infile)
			throw std::runtime_error("Failed to open " + path.string());

		std::string line;
		while (std::getline(infile, line)) {
			if (!line.empty())
				dictionary.push_back(std::move(line));
		}

		if (dictionary.empty())
			throw std::runtime_error("Dictionary is empty after reading file: " + path.string());

		loaded = true;
	}

	std::vector<std::string> result;
	result.reserve(count);
	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<size_t> dist(0, dictionary.size() - 1);

	for (size_t i = 0; i < count; ++i)
		result.push_back(dictionary[dist(rng)]);

	return result;
}
struct Algo {
	using func_t = std::string(*)(const std::string&, const std::string&, const std::string&, const std::string&);
	func_t fn;
	explicit Algo(func_t f) : fn(f) {}
	std::string operator()(const std::string& t, const std::string& k, const std::string& s, const std::string& d) const {
		return fn(t, k, s, d);
	}
};

// Distinct types for Catch2
struct Algo1 : Algo { Algo1() : Algo(encode_token) {} };
struct Algo2 : Algo { Algo2() : Algo(encode_token2) {} };
struct Algo3 : Algo { Algo3() : Algo(encode_token3) {} };
struct Algo4 : Algo { Algo4() : Algo(encode_token4) {} };
struct Algo5 : Algo { Algo5() : Algo(encode_token5) {} };
struct Algo6 : Algo { Algo6() : Algo(encode_token6) {} };
struct Algo7 : Algo { Algo7() : Algo(encode_token7) {} };

TEMPLATE_TEST_CASE(
	"Token encoding performance", "[performance][tokenizer]",
	Algo1, Algo2, Algo3, Algo4, Algo5, Algo6, Algo7
) {
	const std::string key = "secure_app_key_32_bytes_long____";
	const std::string salt_key = "separate_salt_key_32_bytes__";
	const std::string domain = "name";
	auto tokens = generate_random_tokens(1'000'000);

	TestType algo;
	auto start = std::chrono::high_resolution_clock::now();
	for (const auto& token : tokens)
		[[maybe_unused]] auto encoded = algo(token, key, salt_key, domain);
	auto end = std::chrono::high_resolution_clock::now();

	double elapsed = std::chrono::duration<double>(end - start).count();
	double rate = tokens.size() / elapsed;

	INFO("[Algorithm] Elapsed: " << elapsed << "s, Rate: " << static_cast<int>(rate) << " tokens/sec");
	CHECK(rate > 1'000'000);
}




