#include <catch2/catch_test_macros.hpp>
#include "GlyphFPECipher.hpp"
#include "IndexedGlyphSet.hpp"

#include <string>
#include <vector>
#include <chrono>

// Build ASCII alphabet glyph set
IndexedGlyphSet buildAsciiCodebook() {
    static const std::string ascii_alpha =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    return IndexedGlyphSet(ascii_alpha);
}

TEST_CASE("GlyphFPECipher construction and glyphs accessor") {
    auto codebook = buildAsciiCodebook();
    GlyphFPECipher cipher(&codebook);  // noop mode
    REQUIRE(&cipher.glyphs() == &codebook);
}

TEST_CASE("GlyphFPECipher encrypt/decrypt roundtrip") {
    auto codebook = buildAsciiCodebook();
    std::vector<uint8_t> key(16, 0x42);
    std::vector<uint8_t> tweak(4, 0x99);
    GlyphFPECipher cipher(codebook, key, tweak);

    std::string input = "HelloWorld";

    auto encrypted = cipher.encrypt(input);
    REQUIRE(encrypted.size() == input.size());

    auto decrypted = cipher.decrypt(encrypted);
    REQUIRE(decrypted == input);
}

TEST_CASE("GlyphFPECipher noop mode passes through unchanged") {
    auto codebook = buildAsciiCodebook();
    GlyphFPECipher cipher(&codebook);

    std::string input = "NoOpTest";

    auto encrypted = cipher.encrypt(input);
    REQUIRE(encrypted == input);

    auto decrypted = cipher.decrypt(input);
    REQUIRE(decrypted == input);
}

TEST_CASE("different outputs for different keys", "[GlyphFPECipher]") {
    auto codebook1 = buildAsciiCodebook();
    std::vector<uint8_t> key1(16, 0x42);
    std::vector<uint8_t> key2(16, 0x43);  // different key
    std::vector<uint8_t> tweak(4, 0x99);

    GlyphFPECipher cipher1(codebook1, key1, tweak);
    GlyphFPECipher cipher2(codebook1, key2, tweak);

    std::string input = "TestString";

    auto encrypted1 = cipher1.encrypt(input);
    auto encrypted2 = cipher2.encrypt(input);

    REQUIRE(encrypted1 != encrypted2);
}
TEST_CASE("GlyphFPECipher performance benchmark", "[performance]") {
    auto codebook = buildAsciiCodebook();
    std::vector<uint8_t> key(16, 0x42);
    std::vector<uint8_t> tweak(4, 0x99);
    GlyphFPECipher cipher(codebook, key, tweak);

    constexpr size_t input_size = 10000;
    std::string base_str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::string input;
    input.reserve(input_size);
    for (size_t i = 0; i < input_size; ++i) {
        input.push_back(base_str[i % base_str.size()]);
    }

    auto start_enc = std::chrono::steady_clock::now();
    auto encrypted = cipher.encrypt(input);
    auto end_enc = std::chrono::steady_clock::now();

    auto start_dec = std::chrono::steady_clock::now();
    auto decrypted = cipher.decrypt(encrypted);
    auto end_dec = std::chrono::steady_clock::now();

    std::chrono::duration<double> encode_duration = end_enc - start_enc;
    std::chrono::duration<double> decode_duration = end_dec - start_dec;

    REQUIRE(decrypted.size() == input.size());
    REQUIRE(decrypted == input);

    double enc_ops_per_sec = input_size / encode_duration.count();
    double dec_ops_per_sec = input_size / decode_duration.count();

    std::cout << "[benchmark] GlyphFPECipher encoded " << input_size << " bytes in " << encode_duration.count()
              << " seconds (" << enc_ops_per_sec << " ops/s)\n";
    std::cout << "[benchmark] GlyphFPECipher decoded " << input_size << " bytes in " << decode_duration.count()
              << " seconds (" << dec_ops_per_sec << " ops/s)\n";

    CHECK(enc_ops_per_sec > 60000);
    CHECK(dec_ops_per_sec > 60000);
}
