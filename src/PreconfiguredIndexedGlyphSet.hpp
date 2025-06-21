#pragma once

#include "IndexedGlyphSet.hpp"
#include <string>
#include <vector>
#include "GlyphFPECipher.hpp"

struct PreconfiguredIndexedGlyphSet
{
    // Control (non-printable, non-whitespace)
    static const IndexedGlyphSet& control()
    {
        static IndexedGlyphSet set([] {
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
        static IndexedGlyphSet set("\t\n\v\f\r ");
        return set;
    }

    // Digits: '0'–'9'
    static const IndexedGlyphSet& digits()
    {
        static IndexedGlyphSet set("0123456789");
        return set;
    }

    // Letters: 'A'-'Z', 'a'-'z'
    static const IndexedGlyphSet& letters()
    {
        static IndexedGlyphSet set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
        return set;
    }

    // Symbols: printable, non-alphanumeric, non-space
    static const IndexedGlyphSet& symbols()
    {
        static IndexedGlyphSet set(
            "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
        );
        return set;
    }

    static std::vector<GlyphFPECipher> getAllGlyphFPECiphers(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& tweak)
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

};
