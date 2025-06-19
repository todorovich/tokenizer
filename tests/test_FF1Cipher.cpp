#include <catch2/catch_test_macros.hpp>
#include "FF1Cipher.hpp"

#include <array>
#include <functional>
#include <vector>
#include <random>
#include <unordered_set>
#include <iostream>
#include <chrono>

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

TEST_CASE("FF1Cipher encrypt/decrypt roundtrip", "[FF1Cipher]") {
    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), 10);

    std::vector<unsigned> data = {0,1,2,3,4,5,6,7,8,9};
    auto encrypted = cipher.encrypt(std::move(data));
    auto decrypted = cipher.decrypt(std::move(encrypted));

    REQUIRE(decrypted == std::vector<unsigned>{0,1,2,3,4,5,6,7,8,9});
}

TEST_CASE("FF1Cipher different inputs produce different ciphertexts", "[FF1Cipher]") {
    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), 10);

    std::vector<unsigned> a = {0,1,2};
    std::vector<unsigned> b = {2,1,0};

    auto ea = cipher.encrypt(std::move(a));
    auto eb = cipher.encrypt(std::move(b));

    REQUIRE(ea != eb);
}

TEST_CASE("FF1Cipher handles empty input", "[FF1Cipher]") {
    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), 10);

    std::vector<unsigned> empty;
    auto enc = cipher.encrypt(std::move(empty));
    auto dec = cipher.decrypt(std::move(enc));

    REQUIRE(dec.empty());
}

TEST_CASE("FF1Cipher rejects invalid keys", "[FF1Cipher]") {
    std::vector<uint8_t> short_key(8, 0x01); // 64-bit key

    REQUIRE_THROWS_AS(FF1Cipher(short_key, fixed_tweak(), 10), std::invalid_argument);
}

TEST_CASE("FF1Cipher works with various radices", "[FF1Cipher]") {
    for (unsigned radix : {2, 10, 16, 36, 62}) {
        FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

        std::vector<unsigned> digits;
        for (unsigned i = 0; i < 12; ++i)
            digits.push_back(i % radix);

        auto encrypted = cipher.encrypt(std::move(digits));
        auto decrypted = cipher.decrypt(std::move(encrypted));

        for (unsigned i = 0; i < 12; ++i)
            REQUIRE(decrypted[i] == i % radix);
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


TEST_CASE("FF1Cipher collision test with fixed-length arrays", "[FF1Cipher][collision]") {
    constexpr unsigned radix = 62;
    constexpr size_t length = 8;
    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

    std::mt19937 rng(42);
    std::uniform_int_distribution<unsigned> dist(0, radix - 1);

    std::unordered_set<std::array<unsigned, length>, ArrayHash<8>> seen;

    for (int i = 0; i < 10000; ++i) {
        std::vector<unsigned> input(length);
        for (auto& d : input) d = dist(rng);

        auto encrypted = cipher.encrypt(std::move(input));
        if (encrypted.size() != length) {
            FAIL("Encrypted output length mismatch");
        }

        std::array<unsigned, length> encrypted_arr;
        std::copy_n(encrypted.begin(), length, encrypted_arr.begin());

        REQUIRE(seen.insert(encrypted_arr).second); // no duplicates allowed
    }
}

TEST_CASE("FF1Cipher encode/decode performance benchmark with ops/s", "[ff1cipher][performance]") {
    constexpr unsigned radix = 62;
    constexpr size_t input_size = 10000;
    constexpr size_t digit_count = 12;

    FF1Cipher cipher(fixed_key_128(), fixed_tweak(), radix);

    std::mt19937 rng(42);
    std::uniform_int_distribution<unsigned> dist(0, radix - 1);

    // Prepare input data
    std::vector<std::vector<unsigned>> inputs(input_size, std::vector<unsigned>(digit_count));
    for (auto& digits : inputs)
        for (auto& d : digits)
            d = dist(rng);

    // Benchmark encoding
    auto start_enc = std::chrono::steady_clock::now();
    std::vector<std::vector<unsigned>> encrypted;
    encrypted.reserve(input_size);
    for (auto& digits : inputs)
        encrypted.push_back(cipher.encrypt(std::move(digits)));
    auto end_enc = std::chrono::steady_clock::now();

    // Benchmark decoding
    auto start_dec = std::chrono::steady_clock::now();
    std::vector<std::vector<unsigned>> decrypted;
    decrypted.reserve(input_size);
    for (auto& digits : encrypted)
        decrypted.push_back(cipher.decrypt(std::move(digits)));
    auto end_dec = std::chrono::steady_clock::now();

    std::chrono::duration<double> encode_duration = end_enc - start_enc;
    std::chrono::duration<double> decode_duration = end_dec - start_dec;

    double enc_ops_per_sec = input_size / encode_duration.count();
    double dec_ops_per_sec = input_size / decode_duration.count();

    std::cout << "[benchmark] Encoded " << input_size << " items of length " << digit_count
              << " in " << encode_duration.count() << " seconds ("
              << enc_ops_per_sec << " ops/s)" << std::endl;
    std::cout << "[benchmark] Decoded " << input_size << " items of length " << digit_count
              << " in " << decode_duration.count() << " seconds ("
              << dec_ops_per_sec << " ops/s)" << std::endl;

    CHECK(enc_ops_per_sec > 85'000);
    CHECK(dec_ops_per_sec > 85'000);
}