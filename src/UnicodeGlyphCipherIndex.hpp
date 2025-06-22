#pragma once

#include <vector>
#include <cstdint>
#include <string_view>
#include <algorithm>
#include <stdexcept>
#include "GlyphFPECipher.hpp"

inline uint32_t decode_utf8(std::string_view glyph) {
    const auto* s = reinterpret_cast<const unsigned char*>(glyph.data());
    size_t len = glyph.size();
    if (len == 1) return s[0];
    if (len == 2) return ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    if (len == 3) return ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    if (len == 4) return ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
                        ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
    throw std::runtime_error("Invalid UTF-8 glyph for code point extraction");
}

class UnicodeGlyphCipherIndex {
public:
    UnicodeGlyphCipherIndex(
        std::vector<GlyphFPECipher> ciphers,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& tweak
    )
        : glyph_ciphers(std::move(ciphers)),
          noop_cipher(new IndexedGlyphSet("noop", " \n\r"))
    {
        if (glyph_ciphers.empty())
            throw std::invalid_argument("ciphers vector must not be empty");

        _codepoint_to_glyph_cipher = new uint32_t[0x110000];
        std::fill_n(_codepoint_to_glyph_cipher, 0x110000, UINT32_MAX);

        for (uint32_t idx = 0; idx < glyph_ciphers.size(); ++idx) {
            const auto& codebook = glyph_ciphers[idx].glyphs();
            for (size_t i = 0; i < codebook.size(); ++i) {
                uint32_t cp = decode_utf8(codebook.from_index(i));
                if (_codepoint_to_glyph_cipher[cp] == UINT32_MAX)  // first codebook wins
                    _codepoint_to_glyph_cipher[cp] = idx;
            }
        }
    }

    UnicodeGlyphCipherIndex(const UnicodeGlyphCipherIndex&) = delete;
    UnicodeGlyphCipherIndex& operator=(const UnicodeGlyphCipherIndex&) = delete;
    UnicodeGlyphCipherIndex(UnicodeGlyphCipherIndex&& other) noexcept
        : glyph_ciphers(std::move(other.glyph_ciphers)),
          noop_cipher(std::move(other.noop_cipher)),
          _codepoint_to_glyph_cipher(other._codepoint_to_glyph_cipher)
    {
        other._codepoint_to_glyph_cipher = nullptr;
    }
    UnicodeGlyphCipherIndex& operator=(UnicodeGlyphCipherIndex&& other) noexcept {
        if (this != &other) {
            delete[] _codepoint_to_glyph_cipher;
            glyph_ciphers = std::move(other.glyph_ciphers);
            noop_cipher = std::move(other.noop_cipher);
            _codepoint_to_glyph_cipher = other._codepoint_to_glyph_cipher;
            other._codepoint_to_glyph_cipher = nullptr;
        }
        return *this;
    }
    ~UnicodeGlyphCipherIndex() {
        delete[] _codepoint_to_glyph_cipher;
    }

    GlyphFPECipher& operator[](uint32_t codepoint) {
        uint32_t idx = _codepoint_to_glyph_cipher[codepoint];
        if (idx == UINT32_MAX)
            return noop_cipher;
        return glyph_ciphers[idx];
    }
    const GlyphFPECipher& operator[](uint32_t codepoint) const {
        uint32_t idx = _codepoint_to_glyph_cipher[codepoint];
        if (idx == UINT32_MAX)
            return noop_cipher;
        return glyph_ciphers[idx];
    }

    std::vector<GlyphFPECipher> glyph_ciphers; // real ciphers only (no noop)
    GlyphFPECipher noop_cipher;

private:
    uint32_t* _codepoint_to_glyph_cipher = nullptr; // [0x110000], heap-allocated
};
