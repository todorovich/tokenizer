#include <catch2/catch_test_macros.hpp>
#include "GlyphFPECipher.hpp"
#include "IndexedGlyphSet.hpp"
#include "PreconfiguredIndexedGlyphSet.hpp"

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <fstream>

// Build ASCII alphabet glyph set
IndexedGlyphSet buildAsciiCodebook() {
    return IndexedGlyphSet(
        "ascii_letters",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    );
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

TEST_CASE("GlyphFPECipher performance benchmark", "[performance]") {
    auto codebook = buildAsciiCodebook();
    std::vector<uint8_t> key(16, 0x00);
    std::vector<uint8_t> tweak(4, 0x00);
    GlyphFPECipher cipher(codebook, key, tweak);

    std::string base_str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    auto words = load_words();
    REQUIRE(words.size() >= 9578);

    auto start_enc = std::chrono::steady_clock::now();
    std::vector<std::string> encrypted_words;
    encrypted_words.reserve(words.size());
    for (const auto& word : words)
    {
        auto encrypted = cipher.encrypt(word);
    }
    auto end_enc = std::chrono::steady_clock::now();

    std::vector<std::string> decrypted_words;
    decrypted_words.reserve(encrypted_words.size());
    auto start_dec = std::chrono::steady_clock::now();
    {
        for (const auto& word : words)
        {
            auto decrypted = cipher.decrypt(word);
        }
    }
    auto end_dec = std::chrono::steady_clock::now();

    // Verify correctness
    /*REQUIRE(decrypted_words.size() == words.size());
    for (size_t i = 0; i < words.size(); ++i) {
        REQUIRE(decrypted_words[i] == words[i]);
    }*/

    std::chrono::duration<double> encode_duration = end_enc - start_enc;
    std::chrono::duration<double> decode_duration = end_dec - start_dec;

    double enc_ops_per_sec = words.size() / encode_duration.count();
    double dec_ops_per_sec = words.size() / decode_duration.count();

    std::cout << "[benchmark] GlyphFPECipher encoded " << words.size() << " bytes in " << encode_duration.count()
              << " seconds (" << enc_ops_per_sec << " ops/s)\n";
    std::cout << "[benchmark] GlyphFPECipher decoded " << words.size() << " bytes in " << decode_duration.count()
              << " seconds (" << dec_ops_per_sec << " ops/s)\n";

    CHECK(enc_ops_per_sec > 60000);
    CHECK(dec_ops_per_sec > 60000);
}


