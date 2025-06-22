#pragma once
#include "IndexedGlyphSet.hpp"
#include <initializer_list>
#include <string>

inline IndexedGlyphSet codebook_from_cps(std::initializer_list<uint32_t> cps) {
    std::string glyphs;
    for (auto cp : cps) {
        if (cp <= 0x7F) {
            glyphs += static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            glyphs += static_cast<char>(0xC0 | (cp >> 6));
            glyphs += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            glyphs += static_cast<char>(0xE0 | (cp >> 12));
            glyphs += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            glyphs += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            glyphs += static_cast<char>(0xF0 | (cp >> 18));
            glyphs += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            glyphs += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            glyphs += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return IndexedGlyphSet("test", std::move(glyphs));
}
