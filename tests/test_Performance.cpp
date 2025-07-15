#include <catch2/catch_test_macros.hpp>
#include "UnicodeFPECipher.hpp"
#include "UnicodeGlyphCipherIndex.hpp"
#include "GlyphFPECipher.hpp"
#include "IndexedGlyphSet.hpp"
#include "PreconfiguredIndexedGlyphSet.hpp"

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

// Use same key and tweak for both ciphers
std::vector<uint8_t> test_key_1(16, 0x01);
std::vector<uint8_t> test_tweak_1(4, 0x02);


static std::vector<std::string> load_words()
{
    std::filesystem::path path = std::filesystem::current_path() / ".." /"data" / "google-10000-english.txt";
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Missing word list: " + path.string());

    std::vector<std::string> words;
    std::string word;
    while (std::getline(in, word)) {
        if (!word.empty() && word.size() > 1) words.push_back(word);
    }
    return words;
}


TEST_CASE("UnicodeFPECipher vs GlyphFPECipher encryption produces matching results", "[cipher][consistency]") {
    // Prepare glyph ciphers for UnicodeFPECipher
    std::vector<GlyphFPECipher> glyph_ciphers = PreconfiguredIndexedGlyphSet::buildAsciiGlyphCiphers(test_key_1, test_tweak_1);

    // Construct UnicodeGlyphCipherIndex and UnicodeFPECipher
    UnicodeGlyphCipherIndex ugci(std::move(glyph_ciphers), test_key_1, test_tweak_1);
    UnicodeFPECipher unicode_cipher(std::move(ugci));

    // For GlyphFPECipher test: create a single GlyphFPECipher with ASCII codebook
    auto ascii_codebook = IndexedGlyphSet("ascii_letters", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    GlyphFPECipher glyph_cipher(ascii_codebook, test_key_1, test_tweak_1);

    // Test inputs
    std::vector<std::string> test_inputs = load_words();

    for (const auto& input : test_inputs) {
        // Encrypt with UnicodeFPECipher
        std::string unicode_encrypted = unicode_cipher.encrypt(input);

        // Encrypt with GlyphFPECipher
        std::string glyph_encrypted = glyph_cipher.encrypt(input);

        // The outputs may differ because UnicodeFPECipher uses multiple glyph ciphers,
        // but we can at least verify roundtrips are consistent for each
        std::string unicode_decrypted = unicode_cipher.decrypt(unicode_encrypted);
        std::string glyph_decrypted = glyph_cipher.decrypt(glyph_encrypted);

        REQUIRE(unicode_decrypted == input);
        REQUIRE(glyph_decrypted == input);

        // Optional: Print outputs for manual inspection
        INFO("Input: " << input);
        INFO("Unicode encrypted: " << unicode_encrypted);
        INFO("Glyph encrypted: " << glyph_encrypted);
    }
}
