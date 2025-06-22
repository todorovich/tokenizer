#include <catch2/catch_test_macros.hpp>
#include "IndexedGlyphSet.hpp"

TEST_CASE("IndexedGlyphSet constructs with unique glyphs and infers glyph size", "[IndexedGlyphSet]") {
    std::string glyphs;
    glyphs.append("\xC3\xB1"); // ñ (2 bytes)
    glyphs.append("\xC3\xB6"); // ö (2 bytes)

    IndexedGlyphSet igs("test", (std::move(glyphs)));

    REQUIRE(igs.glyph_size() == 2);
    REQUIRE(igs.size() == 2);
}

TEST_CASE("IndexedGlyphSet maps glyphs to indices and back", "[IndexedGlyphSet]") {
    std::string glyphs;
    glyphs.append("\xC3\xB1"); // ñ
    glyphs.append("\xC3\xB6"); // ö

    IndexedGlyphSet igs("test", (std::move(glyphs)));

    REQUIRE(igs.contains("\xC3\xB1"));
    REQUIRE(igs.contains("\xC3\xB6"));
    REQUIRE(!igs.contains("\xC3\xB2")); // ò, not in set

    unsigned idx1 = igs.to_index("\xC3\xB1");
    unsigned idx2 = igs.to_index("\xC3\xB6");

    REQUIRE(idx1 != idx2);

    REQUIRE(igs.from_index(idx1) == std::string_view("\xC3\xB1", 2));
    REQUIRE(igs.from_index(idx2) == std::string_view("\xC3\xB6", 2));
}

TEST_CASE("IndexedGlyphSet throws on duplicate glyphs", "[IndexedGlyphSet]") {
    std::string glyphs;
    glyphs.append("\xC3\xB1");
    glyphs.append("\xC3\xB6");
    glyphs.append("\xC3\xB1"); // duplicate

    REQUIRE_THROWS_AS(IndexedGlyphSet("test", std::move(glyphs)), std::invalid_argument);
}

TEST_CASE("IndexedGlyphSet throws on invalid glyph size input", "[IndexedGlyphSet]") {
    REQUIRE_THROWS_AS(IndexedGlyphSet("test", std::string("\xC3")), std::invalid_argument);
}

TEST_CASE("IndexedGlyphSet iterator works", "[IndexedGlyphSet]") {
    std::string glyphs;
    glyphs.append("\xC3\xB1");
    glyphs.append("\xC3\xB6");

    IndexedGlyphSet igs("test", (std::move(glyphs)));

    auto it = igs.begin();
    REQUIRE(*it == std::string_view("\xC3\xB1", 2));
    ++it;
    REQUIRE(*it == std::string_view("\xC3\xB6", 2));
    ++it;
    REQUIRE(it == igs.end());
}

TEST_CASE("IndexedGlyphSet encode/decode performance benchmark", "[IndexedGlyphSet][performance]") {
    // Create a simple ASCII alpha codebook (all single-byte glyphs)
    std::string ascii_alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    IndexedGlyphSet codebook("test", (std::move(ascii_alpha)));

    constexpr size_t input_size = 100000;
    std::vector<std::string> inputs;
    inputs.reserve(input_size);

    // Generate test strings: single-character glyph repeated
    for (size_t i = 0; i < input_size; ++i) {
        inputs.emplace_back(std::string(1, codebook.glyphs()[i % codebook.glyphs().size()]));
    }

    // Benchmark encode: glyph -> index
    auto start_enc = std::chrono::steady_clock::now();
    std::vector<unsigned> encoded;
    encoded.reserve(input_size);
    for (const auto& glyph : inputs) {
        encoded.push_back(codebook.to_index(glyph));
    }
    auto end_enc = std::chrono::steady_clock::now();

    // Benchmark decode: index -> glyph
    auto start_dec = std::chrono::steady_clock::now();
    std::vector<std::string> decoded;
    decoded.reserve(input_size);
    for (auto idx : encoded) {
        auto glyph_view = codebook.from_index(idx);
        decoded.emplace_back(glyph_view.data(), glyph_view.size());
    }
    auto end_dec = std::chrono::steady_clock::now();

    std::chrono::duration<double> encode_duration = end_enc - start_enc;
    std::chrono::duration<double> decode_duration = end_dec - start_dec;

    double enc_ops_per_sec = input_size / encode_duration.count();
    double dec_ops_per_sec = input_size / decode_duration.count();

    std::cout << "[benchmark] IndexedGlyphSet encoded " << input_size << " glyphs in "
              << encode_duration.count() << " seconds (" << enc_ops_per_sec << " ops/s)\n";
    std::cout << "[benchmark] IndexedGlyphSet decoded " << input_size << " glyphs in "
              << decode_duration.count() << " seconds (" << dec_ops_per_sec << " ops/s)\n";

    REQUIRE(true); // dummy to satisfy Catch2
}