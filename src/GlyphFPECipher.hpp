#pragma once
#include <vector>
#include <stdexcept>
#include "IndexedGlyphSet.hpp"
#include "FF1Cipher.hpp"

// TODO: Make a version that operates on codepoints instead of glyphs and benchmark against it.
class GlyphFPECipher {
public:
    // Construct from glyph set plus key and tweak for FF1
    GlyphFPECipher(
        IndexedGlyphSet glyphs, const std::vector<uint8_t>& key, const std::vector<uint8_t>& tweak
    )
        : glyphs_(std::move(glyphs))
        , cipher_(key, tweak, static_cast<unsigned>(glyphs_.size()))
    {
        if (glyphs_.size() == 0)
            throw std::invalid_argument("GlyphFPECipher: glyph set cannot be empty");
    }

    ~GlyphFPECipher() = default;

    GlyphFPECipher(const GlyphFPECipher& other) = delete;
    GlyphFPECipher& operator=(const GlyphFPECipher& other) = delete;
    GlyphFPECipher(GlyphFPECipher&& other) noexcept = default;
    GlyphFPECipher& operator=(GlyphFPECipher&& other) noexcept = default;

    // Encrypt indices by moving vector
    std::vector<unsigned> encrypt(std::vector<unsigned>&& indices) const {
        return cipher_.encrypt(std::move(indices));
    }

    // Decrypt indices by moving vector
    std::vector<unsigned> decrypt(std::vector<unsigned>&& indices) const {
        return cipher_.decrypt(std::move(indices));
    }

    const IndexedGlyphSet& glyphs() const { return glyphs_; }
    const FF1Cipher& cipher() const { return cipher_; }

private:
    IndexedGlyphSet glyphs_;
    FF1Cipher cipher_;
};
