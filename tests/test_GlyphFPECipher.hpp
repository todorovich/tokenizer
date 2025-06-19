#include <catch2/catch_test_macros.hpp>
#include "GlyphFPECipher.hpp"
#include "IndexedGlyphSet.hpp"
#include "FF1Cipher.hpp"

#include <string>
#include <vector>
#include <iostream>
#include <chrono>

// Helper: UTF-8 single-byte ASCII alphabet for tests
static const std::string ascii_alpha =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

// Helper to build IndexedGlyphSet from ASCII alphabet (1-byte glyphs)
IndexedGlyphSet buildAsciiAlphaCodebook() {
    return IndexedGlyphSet(ascii_alpha);
}

// Fixed test key and tweak
static std::vector<uint8_t> test_key = {
    0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
    0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF
};

static std::vector<uint8_t> test_tweak = { 0xDE, 0xAD, 0xBE, 0xEF };

TEST_CASE("GlyphFPECipher encrypt/decrypt roundtrip ASCII alpha", "[GlyphFPECipher]") {
    auto codebook = buildAsciiAlphaCodebook();
    FF1Cipher cipher(test_key, test_tweak, static_cast<unsigned>(codebook.size()));

    GlyphFPECipher gfpc(std::move(codebook), std::move(cipher));

    // Simple input: "HelloWorld"
    std::string input = "HelloWorld";

    // Map input to glyph indices
    std::vector<unsigned> indices;
    for (char c : input) {
        std::string glyph(1, c);
        indices.push_back(gfpc.glyphs().to_index(glyph));
    }

    auto encrypted = gfpc.encrypt(std::move(indices));
    auto decrypted = gfpc.decrypt(std::move(encrypted));

    // Map back decrypted indices to string
    std::string output;
    for (auto idx : decrypted) {
        auto glyph_view = gfpc.glyphs().from_index(idx);
        output.append(glyph_view.data(), glyph_view.size());
    }

    REQUIRE(output == input);
}

TEST_CASE("GlyphFPECipher throws on mismatched radix", "[GlyphFPECipher]") {
    auto codebook = buildAsciiAlphaCodebook();
    // Deliberately wrong radix
    FF1Cipher cipher(test_key, test_tweak, 10);

    REQUIRE_THROWS_AS(GlyphFPECipher(std::move(codebook), std::move(cipher)), std::invalid_argument);
}

TEST_CASE("GlyphFPECipher handles empty input", "[GlyphFPECipher]") {
    auto codebook = buildAsciiAlphaCodebook();
    FF1Cipher cipher(test_key, test_tweak, static_cast<unsigned>(codebook.size()));

    GlyphFPECipher gfpc(std::move(codebook), std::move(cipher));

    std::vector<unsigned> empty;
    auto encrypted = gfpc.encrypt(std::move(empty));
    auto decrypted = gfpc.decrypt(std::move(encrypted));

    REQUIRE(decrypted.empty());
}


