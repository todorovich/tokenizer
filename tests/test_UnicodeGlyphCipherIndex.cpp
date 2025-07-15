#include <catch2/catch_test_macros.hpp>
#include "UnicodeGlyphCipherIndex.hpp"
#include "IndexedGlyphSet.hpp"
#include <vector>

#include "codebook_helpers.hpp"

TEST_CASE("UnicodeGlyphCipherIndex: mapped codepoint returns correct cipher") {
    uint32_t cp1 = 0x61; // 'a'
    uint32_t cp2 = 0x62; // 'b'
    auto book1 = codebook_from_cps({cp1, 0x63});  // 'a', 'c' distinct glyphs
    auto book2 = codebook_from_cps({cp2, 0x64});  // 'b', 'd' distinct glyphs

    std::vector<uint8_t> key(16, 0x00);
    std::vector<uint8_t> tweak(4, 0x00);

    std::vector<GlyphFPECipher> ciphers;
    ciphers.emplace_back(&book1, key, tweak, false);
    ciphers.emplace_back(&book2, key, tweak, false);

    UnicodeGlyphCipherIndex cipher_index(std::move(ciphers), key, tweak);

    GlyphFPECipher& c1 = cipher_index[cp1];
    GlyphFPECipher& c2 = cipher_index[cp2];

    REQUIRE(c1.glyphs().contains(std::string(1, static_cast<char>(cp1))));
    REQUIRE(c2.glyphs().contains(std::string(1, static_cast<char>(cp2))));
}

TEST_CASE("UnicodeGlyphCipherIndex: unmapped codepoint returns noop cipher") {
    uint32_t cp1 = 0x61; // 'a'
    auto book1 = codebook_from_cps({cp1, 0x63});  // 'a', 'c'

    std::vector<uint8_t> key(16, 0x00);
    std::vector<uint8_t> tweak(4, 0x00);

    std::vector<GlyphFPECipher> ciphers;
    ciphers.emplace_back(&book1, key, tweak, false);

    UnicodeGlyphCipherIndex ugci(std::move(ciphers), key, tweak);

    uint32_t unmapped_cp = 0x7A; // 'z', not in any codebook
    GlyphFPECipher& c3 = ugci[unmapped_cp];

    REQUIRE(&c3 == &ugci.noop_cipher);
}

// TODO: this should actually test overlap
TEST_CASE("UnicodeGlyphCipherIndex: overlapping codepoint, first codebook wins") {
    uint32_t cp = 0x61; // 'a'
    auto book1 = codebook_from_cps({cp, 0x63});  // 'a', 'c'
    auto book2 = codebook_from_cps({0x64, 0x65}); // 'd', 'e' no overlap

    std::vector<uint8_t> key(16, 0xAB);
    std::vector<uint8_t> tweak(4, 0xCD);

    std::vector<GlyphFPECipher> ciphers;
    ciphers.emplace_back(&book1, key, tweak, false);
    ciphers.emplace_back(&book2, key, tweak, false);

    UnicodeGlyphCipherIndex ugci(std::move(ciphers), key, tweak);

    GlyphFPECipher& cipher = ugci[cp];
    REQUIRE(cipher.glyphs().contains(std::string(1, static_cast<char>(cp))));
}

TEST_CASE("UnicodeGlyphCipherIndex: move semantics") {
    uint32_t cp = 0x62;
    auto book = codebook_from_cps({cp, 0x63});

    std::vector<uint8_t> key(16, 0x55);
    std::vector<uint8_t> tweak(4, 0x33);

    std::vector<GlyphFPECipher> ciphers;
    ciphers.emplace_back(&book, key, tweak, false);

    UnicodeGlyphCipherIndex ugci(std::move(ciphers), key, tweak);

    // Move construct
    UnicodeGlyphCipherIndex ugci2(std::move(ugci));
    GlyphFPECipher& cipher = ugci2[cp];
    REQUIRE(cipher.glyphs().contains(std::string(1, static_cast<char>(cp))));

    // Create a new non-empty vector for ugci3
    auto book2 = codebook_from_cps({0x64, 0x65});
    std::vector<GlyphFPECipher> ciphers2;
    ciphers2.emplace_back(&book2, key, tweak, false);

    UnicodeGlyphCipherIndex ugci3(std::move(ciphers2), key, tweak);
    ugci3 = std::move(ugci2);
    GlyphFPECipher& cipher2 = ugci3[cp];
    REQUIRE(cipher2.glyphs().contains(std::string(1, static_cast<char>(cp))));
}

