#include <catch2/catch_test_macros.hpp>
#include "UnicodeFPECipher.hpp"
#include "UnicodeGlyphCipherIndex.hpp"
#include "GlyphFPECipher.hpp"
#include "IndexedGlyphSet.hpp"
#include "PreconfiguredIndexedGlyphSet.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

#include "codebook_helpers.hpp"

std::vector<uint8_t> test_key(16, 0x00);
std::vector<uint8_t> test_tweak(4, 0x00);

TEST_CASE("UnicodeFPECipher: encrypt/decrypt roundtrip basic", "[UnicodeFPECipher]")
{
    auto book1 = codebook_from_cps({0x61, 0x62, 0x63}); // a, b, c
    auto book2 = codebook_from_cps({0x31, 0x32, 0x33}); // 1, 2, 3

    std::vector<GlyphFPECipher> glyph_ciphers;
    glyph_ciphers.emplace_back(&book1, test_key, test_tweak, false);
    glyph_ciphers.emplace_back(&book2, test_key, test_tweak, false);

    UnicodeGlyphCipherIndex ugci(std::move(glyph_ciphers), test_key, test_tweak);
    UnicodeFPECipher cipher(std::move(ugci));

    std::string input = "ab12c3";
    std::string encrypted = cipher.encrypt(input);
    REQUIRE(encrypted.size() == input.size());

    std::string decrypted = cipher.decrypt(encrypted);
    REQUIRE(decrypted == input);
}

TEST_CASE("UnicodeFPECipher: noop glyphs pass through unchanged", "[UnicodeFPECipher]")
{
    auto book1 = codebook_from_cps({0x61, 0x62}); // a, b

    std::vector<GlyphFPECipher> glyph_ciphers;
    glyph_ciphers.emplace_back(&book1, test_key, test_tweak, false);

    UnicodeGlyphCipherIndex ugci(std::move(glyph_ciphers), test_key, test_tweak);
    UnicodeFPECipher cipher(std::move(ugci));

    std::string input = "xyz"; // all unmapped glyphs, so noop
    std::string encrypted = cipher.encrypt(input);
    REQUIRE(encrypted == input);

    std::string decrypted = cipher.decrypt(encrypted);
    REQUIRE(decrypted == input);
}

TEST_CASE("UnicodeFPECipher: mixed mapped and unmapped glyphs", "[UnicodeFPECipher]")
{
    auto book1 = codebook_from_cps({0x61, 0x62, 0x63}); // a,b,c
    auto book2 = codebook_from_cps({0x31, 0x32, 0x33}); // 1,2,3

    std::vector<GlyphFPECipher> glyph_ciphers;
    glyph_ciphers.emplace_back(&book1, test_key, test_tweak, false);
    glyph_ciphers.emplace_back(&book2, test_key, test_tweak, false);

    UnicodeGlyphCipherIndex ugci(std::move(glyph_ciphers), test_key, test_tweak);
    UnicodeFPECipher cipher(std::move(ugci));

    std::string input = "a1x2b3y";
    std::string encrypted = cipher.encrypt(input);
    REQUIRE(encrypted.size() == input.size());

    std::string decrypted = cipher.decrypt(encrypted);
    REQUIRE(decrypted == input);
}


// Load Google's 10,000 words from file
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

TEST_CASE("UnicodeFPECipher benchmark 10,000 words", "[UnicodeFPECipher][performance]") {
    std::vector<uint8_t> key(16, 0x01);   // example key
    std::vector<uint8_t> tweak(4, 0x02);  // example tweak

    std::vector<GlyphFPECipher> glyph_ciphers = PreconfiguredIndexedGlyphSet::buildAsciiGlyphCiphers(key, tweak);
    UnicodeGlyphCipherIndex ugci(std::move(glyph_ciphers), key, tweak);
    UnicodeFPECipher cipher(std::move(ugci));

    auto words = load_words();

    // Encrypt each word individually
    std::vector<std::string> encrypted_words;
    encrypted_words.reserve(words.size());
    auto start_enc = std::chrono::steady_clock::now();
    {
        for (const auto& w : words) {
            encrypted_words.push_back(cipher.encrypt(w));
        }
    }
    auto end_enc = std::chrono::steady_clock::now();

    // Decrypt each word individually
    std::vector<std::string> decrypted_words;
    decrypted_words.reserve(encrypted_words.size());
    auto start_dec = std::chrono::steady_clock::now();
    {
        for (const auto& ew : encrypted_words) {
            decrypted_words.push_back(cipher.decrypt(ew));
        }
    }
    auto end_dec = std::chrono::steady_clock::now();

    // Verify correctness
    REQUIRE(decrypted_words.size() == words.size());
    for (size_t i = 0; i < words.size(); ++i) {
        REQUIRE(decrypted_words[i] == words[i]);
    }

    double enc_sec = std::chrono::duration<double>(end_enc - start_enc).count();
    double dec_sec = std::chrono::duration<double>(end_dec - start_dec).count();

    double enc_ops_per_sec = words.size() / enc_sec;
    double dec_ops_per_sec = words.size() / dec_sec;

    std::cout << "[benchmark] UnicodeFPECipher encoded " << words.size() << " words in " << enc_sec
              << " s (" << enc_ops_per_sec << " ops/s)\n";
    std::cout << "[benchmark] UnicodeFPECipher decoded " << words.size() << " words in " << dec_sec
              << " s (" << dec_ops_per_sec << " ops/s)\n";

    CHECK(enc_ops_per_sec > 30000);
    CHECK(dec_ops_per_sec > 30000);
}

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

static std::vector<std::string> generate_all_unicode_words(size_t word_min_len = 1, size_t word_max_len = 10, size_t max_codepoint = 0xFFFF)
{
    std::vector<std::string> words;
    std::string current_word;

    // Helper to encode codepoint to UTF-8
    auto append_utf8 = [](std::string& s, uint32_t cp) {
        if (cp <= 0x7F) {
            s.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            s.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    };

    for (uint32_t cp = 0; cp <= max_codepoint; ++cp) {
        // Skip surrogate halves (invalid UTF-8 codepoints)
        if (cp >= 0xD800 && cp <= 0xDFFF) continue;

        append_utf8(current_word, cp);

        if (current_word.size() >= word_min_len && current_word.size() >= word_max_len) {
            words.push_back(std::move(current_word));
            current_word.clear();
        }
    }

    if (!current_word.empty())
        words.push_back(std::move(current_word));

    return words;
}
