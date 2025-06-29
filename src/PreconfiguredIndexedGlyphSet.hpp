#pragma once

#include "IndexedGlyphSet.hpp"

#include "GlyphFPECipher.hpp"
#include "UnicodeBlockList.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <algorithm>

struct PreconfiguredIndexedGlyphSet
{

    template <typename BlockType, size_t N>
    static void check_unicode_blocks(const BlockType (&unicode_blocks)[N]) {
        // Copy blocks to a vector for sorting
        std::vector<BlockType> blocks(std::begin(unicode_blocks), std::end(unicode_blocks));

        // Sort blocks by start codepoint
        std::sort(blocks.begin(), blocks.end(),
            [](const BlockType& a, const BlockType& b) { return a.start < b.start; });

        // Check for overlaps between blocks
        for (size_t i = 1; i < blocks.size(); ++i) {
            const auto& prev = blocks[i - 1];
            const auto& curr = blocks[i];
            if (curr.start <= prev.end) {
                std::cout << "Overlap detected between blocks:\n"
                          << "  " << prev.name << " (" << std::hex << prev.start << "-" << prev.end << ")\n"
                          << "  " << curr.name << " (" << curr.start << "-" << curr.end << ")\n";
            }
        }

        // Check for duplicate codepoints across blocks
        // Unicode max codepoint is 0x10FFFF
        const uint32_t max_cp = 0x10FFFF;
        std::vector<bool> coverage(max_cp + 1, false);

        for (const auto& block : blocks) {
            for (uint32_t cp = block.start; cp <= block.end; ++cp) {
                if (coverage[cp]) {
                    std::cout << "Duplicate codepoint U+"
                              << std::hex << cp
                              << " in block " << block.name << "\n";
                }
                coverage[cp] = true;
            }
        }
    }

    static std::unordered_map<std::string, IndexedGlyphSet> unicode_blocks_glyph_set;

    static void encode_utf8(const uint32_t codepoint, std::string& out) {
        if (codepoint <= 0x7F) {
            // 1-byte ASCII
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            // 2-byte sequence
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            // 3-byte sequence
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0x10FFFF) {
            // 4-byte sequence
            out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            throw std::runtime_error("Invalid Unicode codepoint");
        }
    }

    static std::unordered_map<std::string, IndexedGlyphSet> buildIndexedGlyphSetsFromBlocks() {
        std::unordered_map<std::string, IndexedGlyphSet> glyph_sets;

        for (const auto& [name, start, end] : unicode_blocks) {
            std::string all_glyphs;  // Single string holding all glyphs for this block

            for (uint32_t cp = start; cp <= end; ++cp) {
                if (cp >= 0xD800 && cp <= 0xDFFF)  // skip surrogates
                    continue;

                encode_utf8(cp, all_glyphs);  // Append encoded UTF-8 glyph bytes
            }

            if (all_glyphs.empty()) continue;

            glyph_sets.emplace(name, IndexedGlyphSet(name, std::move(all_glyphs)));
        }

        return glyph_sets;
    }

    // Control (non-printable, non-whitespace)
    static const IndexedGlyphSet& control()
    {
        static IndexedGlyphSet set("control",[] {
            std::string glyphs;
            for (char c = 0x00; c <= 0x08; ++c) glyphs += c;
            for (char c = 0x0E; c <= 0x1F; ++c) glyphs += c;
            glyphs += static_cast<char>(0x7F);
            return glyphs;
        }());
        return set;
    }

    // Whitespace: tab, LF, VT, FF, CR, space
    static const IndexedGlyphSet& whitespace()
    {
        static IndexedGlyphSet set("whitespace", "\t\n\v\f\r ");
        return set;
    }

    // Digits: '0'–'9'
    static const IndexedGlyphSet& digits()
    {
        static IndexedGlyphSet set("digits", "0123456789");
        return set;
    }

    // Letters: 'A'-'Z', 'a'-'z'
    static const IndexedGlyphSet& letters()
    {
        static IndexedGlyphSet set("letters", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
        return set;
    }

    // Symbols: printable, non-alphanumeric, non-space
    static const IndexedGlyphSet& symbols()
    {
        static IndexedGlyphSet set(
            "symbols","!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
        );
        return set;
    }

    static std::vector<GlyphFPECipher> buildAsciiGlyphCiphers(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& tweak
    )
    {
        std::vector<GlyphFPECipher> result;
        result.reserve(5); // known count

        result.emplace_back(&control(), key, tweak, false);
        result.emplace_back(&whitespace(), key, tweak, false);
        result.emplace_back(&digits(), key, tweak, false);
        result.emplace_back(&letters(), key, tweak, false);
        result.emplace_back(&symbols(), key, tweak, false);

        return result;
    }

    static std::vector<GlyphFPECipher> buildUnicodeGlyphCiphers(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& tweak
    )
    {
        std::vector<GlyphFPECipher> glyph_fpe_ciphers;// = buildAsciiGlyphCiphers(key, tweak);
        glyph_fpe_ciphers.reserve(unicode_blocks_glyph_set.size());

        for (const auto& [block_name, indexed_glyph_set] : unicode_blocks_glyph_set)
        {
            glyph_fpe_ciphers.push_back(GlyphFPECipher(indexed_glyph_set, key, tweak));
        }

        return glyph_fpe_ciphers;
    }

};
