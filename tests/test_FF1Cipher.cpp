#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "FF1Cipher.hpp"

#include <array>
#include <vector>
#include <random>
#include <unordered_set>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <fstream>

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

template <typename DigitType>
std::vector<DigitType> generate_valid_digits(unsigned int radix, size_t length) {
    std::vector<DigitType> digits(length);
    for (size_t i = 0; i < length; ++i) {
        digits[i] = static_cast<DigitType>(i % radix);
    }
    return digits;
}

template <typename DigitType>
std::vector<DigitType> generate_invalid_digits(unsigned int radix) {
    std::vector<DigitType> digits = {0, 1, 2};
    digits.push_back(static_cast<DigitType>(radix));     // invalid digit == radix
    digits.push_back(static_cast<DigitType>(radix + 1)); // invalid digit > radix
    return digits;
}

TEMPLATE_TEST_CASE("FF1Cipher encrypt/decrypt roundtrip", "[FF1Cipher]", uint8_t, uint16_t, uint32_t) {
    using DigitType = TestType;

    auto key = fixed_key_128();
    auto tweak = fixed_tweak();
    unsigned int radix = 10;

    FF1Cipher<DigitType> cipher(key, tweak, radix);
    auto data = generate_valid_digits<DigitType>(radix, 10);

    auto encrypted = cipher.encrypt(std::move(data));
    auto decrypted = cipher.decrypt(std::move(encrypted));

    REQUIRE(decrypted == generate_valid_digits<DigitType>(radix, 10));
}

TEMPLATE_TEST_CASE("FF1Cipher different inputs produce different ciphertexts", "[FF1Cipher]", uint8_t, uint16_t, uint32_t) {
    using DigitType = TestType;

    auto key = fixed_key_128();
    auto tweak = fixed_tweak();
    unsigned int radix = 10;

    FF1Cipher<DigitType> cipher(key, tweak, radix);
    std::vector<DigitType> a = generate_valid_digits<DigitType>(radix, 3);
    std::vector<DigitType> b = {a[2], a[1], a[0]};

    auto ea = cipher.encrypt(std::move(a));
    auto eb = cipher.encrypt(std::move(b));

    REQUIRE(ea != eb);
}

TEMPLATE_TEST_CASE("FF1Cipher handles empty input", "[FF1Cipher]", uint32_t) {
    using DigitType = TestType;

    auto key = fixed_key_128();
    auto tweak = fixed_tweak();
    unsigned int radix = 10;

    FF1Cipher<DigitType> cipher(key, tweak, radix);
    std::vector<DigitType> empty;
    auto enc = cipher.encrypt(std::move(empty));
    auto dec = cipher.decrypt(std::move(enc));

    REQUIRE(dec.empty());
}

TEST_CASE("FF1Cipher rejects invalid keys", "[FF1Cipher]") {
    std::vector<uint8_t> short_key(8, 0x01); // 64-bit key
    std::vector<uint8_t> long_key(33, 0x01); // 264-bit key
    auto tweak = fixed_tweak();

    REQUIRE_THROWS_AS(FF1Cipher<uint32_t>(short_key, tweak, 10), std::invalid_argument);
    REQUIRE_THROWS_AS(FF1Cipher<uint32_t>(long_key, tweak, 10), std::invalid_argument);
}

TEMPLATE_TEST_CASE("FF1Cipher works with various radices", "[FF1Cipher]", uint8_t, uint16_t, uint32_t) {
    using DigitType = TestType;

    auto key = fixed_key_128();
    auto tweak = fixed_tweak();

    for (unsigned radix : {2, 10, 16, 36, 62}) {
        FF1Cipher<DigitType> cipher(key, tweak, radix);
        auto digits = generate_valid_digits<DigitType>(radix, 12);

        auto encrypted = cipher.encrypt(std::move(digits));
        auto decrypted = cipher.decrypt(std::move(encrypted));

        for (unsigned i = 0; i < 12; ++i)
            REQUIRE(decrypted[i] == static_cast<DigitType>(i % radix));
    }
}

template <size_t N>
struct ArrayHash {
    size_t operator()(const std::array<unsigned, N>& arr) const noexcept {
        size_t hash = 0;
        for (unsigned x : arr) {
            hash ^= std::hash<unsigned>{}(x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

TEMPLATE_TEST_CASE("FF1Cipher collision test with fixed-length arrays", "[FF1Cipher][collision]", uint8_t, uint16_t, uint32_t) {
    using DigitType = TestType;

    constexpr unsigned radix = 62;
    constexpr size_t length = 8;
    auto key = fixed_key_128();
    auto tweak = fixed_tweak();

    FF1Cipher<DigitType> cipher(key, tweak, radix);

    std::mt19937 rng(42);
    std::uniform_int_distribution<unsigned> dist(0, radix - 1);

    std::unordered_set<std::array<unsigned, length>, ArrayHash<length>> seen;

    for (int i = 0; i < 10000; ++i) {
        std::vector<DigitType> input(length);
        for (auto& d : input) d = static_cast<DigitType>(dist(rng));

        auto encrypted = cipher.encrypt(std::move(input));
        if (encrypted.size() != length) {
            FAIL("Encrypted output length mismatch");
        }

        std::array<unsigned, length> encrypted_arr;
        std::copy_n(encrypted.begin(), length, encrypted_arr.begin());

        REQUIRE(seen.insert(encrypted_arr).second); // no duplicates allowed
    }
}

static std::vector<std::string> load_words_2()
{
    std::filesystem::path path = std::filesystem::current_path() / "data" / "google-10000-english.txt";
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Missing word list: " + path.string());

    std::vector<std::string> words;
    std::string word;
    while (std::getline(in, word)) {
        if (!word.empty() && word.size() > 1) words.push_back(word);
    }
    return words;
}

TEMPLATE_TEST_CASE("FF1Cipher encode/decode performance benchmark with words", "[ff1cipher][performance]", uint8_t, uint16_t, uint32_t) {
    using DigitType = TestType;

    constexpr unsigned radix = 62; // assuming radix 62 (0-9, A-Z, a-z)
    auto key = fixed_key_128();
    auto tweak = fixed_tweak();

    FF1Cipher<DigitType> cipher(key, tweak, radix);

    auto words = load_words_2();

    // Convert each word to vector<DigitType> based on your radix scheme.
    // Here we assume a function that maps chars to digit values in [0, radix)
    auto char_to_digit = [](char c) -> DigitType {
        if (c >= '0' && c <= '9') return static_cast<DigitType>(c - '0');
        else if (c >= 'A' && c <= 'Z') return static_cast<DigitType>(c - 'A' + 10);
        else if (c >= 'a' && c <= 'z') return static_cast<DigitType>(c - 'a' + 36);
        else throw std::runtime_error("Invalid character in word");
    };

    std::vector<std::vector<DigitType>> inputs;
    inputs.reserve(words.size());

    for (const auto& word : words) {
        std::vector<DigitType> digits;
        digits.reserve(word.size());
        for (char c : word) {
            digits.push_back(char_to_digit(c));
        }
        inputs.push_back(std::move(digits));
    }

    // Benchmark encoding
    std::vector<std::vector<DigitType>> encrypted;
    encrypted.reserve(inputs.size());

    auto start_enc = std::chrono::steady_clock::now();
    for (auto& digits : inputs) {
        encrypted.push_back(cipher.encrypt(std::move(digits)));
    }
    auto end_enc = std::chrono::steady_clock::now();

    // Benchmark decoding
    std::vector<std::vector<DigitType>> decrypted;
    decrypted.reserve(encrypted.size());

    auto start_dec = std::chrono::steady_clock::now();
    for (auto& digits : encrypted) {
        decrypted.push_back(cipher.decrypt(std::move(digits)));
    }
    auto end_dec = std::chrono::steady_clock::now();

    REQUIRE(decrypted.size() == inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        REQUIRE(decrypted[i] == inputs[i]);
    }

    std::chrono::duration<double> encode_duration = end_enc - start_enc;
    std::chrono::duration<double> decode_duration = end_dec - start_dec;

    double enc_ops_per_sec = inputs.size() / encode_duration.count();
    double dec_ops_per_sec = inputs.size() / decode_duration.count();

    std::cout << "[benchmark] FF1Cipher<"
              << (std::is_same_v<DigitType, uint8_t> ? "uint8_t" : std::is_same_v<DigitType, uint16_t> ? "uint16_t" : "uint32_t")
              << "> Encoded " << inputs.size() << " words in " << encode_duration.count() << " seconds (" << enc_ops_per_sec << " ops/s)" << std::endl;
    std::cout << "[benchmark] FF1Cipher<"
              << (std::is_same_v<DigitType, uint8_t> ? "uint8_t" : std::is_same_v<DigitType, uint16_t> ? "uint16_t" : "uint32_t")
              << "> Decoded " << inputs.size() << " words in " << decode_duration.count() << " seconds (" << dec_ops_per_sec << " ops/s)" << std::endl;

    CHECK(enc_ops_per_sec > 75000);
    CHECK(dec_ops_per_sec > 75000);
}

TEMPLATE_TEST_CASE("FF1Cipher decrypt throws or fails on invalid ciphertext", "[FF1Cipher][error]", uint8_t, uint16_t, uint32_t) {
    using DigitType = TestType;

    auto key = fixed_key_128();
    auto tweak = fixed_tweak();
    unsigned int radix = 10;

    FF1Cipher<DigitType> cipher(key, tweak, static_cast<DigitType>(radix));

    // Create invalid ciphertext (values outside radix range)
    auto invalid_ct = generate_invalid_digits<DigitType>(radix);

    try {
        auto result = cipher.decrypt(std::move(invalid_ct));
        FAIL("Expected exception due to invalid ciphertext, but decrypt returned");
    } catch (const std::exception& e) {
        SUCCEED("Caught expected exception: " << e.what());
    } catch (...) {
        SUCCEED("Caught expected unknown exception");
    }
}

TEMPLATE_TEST_CASE("FF1Cipher encrypt throws on invalid plaintext digits", "[FF1Cipher][error]", uint8_t, uint16_t, uint32_t) {
    using DigitType = TestType;

    auto key = fixed_key_128();
    auto tweak = fixed_tweak();
    unsigned int radix = 10;

    FF1Cipher<DigitType> cipher(key, tweak, static_cast<DigitType>(radix));

    auto invalid_pt = generate_invalid_digits<DigitType>(radix);

    REQUIRE_THROWS_AS(cipher.encrypt(std::move(invalid_pt)), std::invalid_argument);
}
