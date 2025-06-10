#include <catch2/catch_test_macros.hpp>
#include "../src/tokenizer.hpp"

const std::string key = "secure_app_key_32_bytes_long____";
const std::string salt_key = "separate_salt_key_32_bytes__";
const std::string domain = "name";

TEST_CASE("encode_token is deterministic") {
	std::string a = encode_token("Fred", key, salt_key, domain);
	std::string b = encode_token("Fred", key, salt_key, domain);
	REQUIRE(a == b);
}

TEST_CASE("different tokens produce different output") {
	std::string x = encode_token("Alice", key, salt_key, domain);
	std::string y = encode_token("Bob", key, salt_key, domain);
	REQUIRE(x != y);
}

TEST_CASE("output length matches input") {
	std::string out = encode_token("Example", key, salt_key, domain);
	REQUIRE(out.length() == 7);
}
