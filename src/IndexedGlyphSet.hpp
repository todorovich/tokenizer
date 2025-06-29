#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <immintrin.h>  // For AVX2 SIMD

// TODO: performance can be better.
class IndexedGlyphSet
{
public:
    // Construct from a flat UTF-8 string of concatenated glyphs,
    // all glyphs must be the same byte length (inferred from first glyph)
    explicit IndexedGlyphSet(std::string&& name, std::string&& flat_glyphs)
        : _glyph_size(get_glyph_size(flat_glyphs))
        , _glyphs(std::move(flat_glyphs))
        , _name(std::move(name))
    {
        if (_glyphs.empty())
            throw std::invalid_argument("IndexedGlyphSet: input string empty");

        if (_glyphs.size() < 2)
            throw std::invalid_argument("IndexedGlyphSet: not enough characters to form an indexed set, requires as size >1");

        if (_glyphs.size() % _glyph_size != 0)
            throw std::invalid_argument("IndexedGlyphSet: invalid glyph size or input length");

        const size_t num_glyphs = _glyphs.size() / _glyph_size;
        glyph_views_.reserve(num_glyphs);

        for (size_t i = 0; i < num_glyphs; ++i)
            glyph_views_.emplace_back(&_glyphs[i * _glyph_size], _glyph_size);

        std::ranges::sort(glyph_views_);

        // Check duplicates by comparing adjacent after sort
        for (size_t i = 1; i < glyph_views_.size(); ++i) {
            if (glyph_views_[i] == glyph_views_[i - 1]) {
                // Format glyph bytes as hex string for clarity
                const auto& dup = glyph_views_[i];
                std::string hex_bytes;
                for (unsigned char c : dup) {
                    char buf[4];
                    std::snprintf(buf, sizeof(buf), "%02X ", c);
                    hex_bytes += buf;
                }

                std::string glyph_str;
                // For printable ASCII glyphs, show as characters too
                bool printable = std::all_of(dup.begin(), dup.end(), [](char ch) { return std::isprint(static_cast<unsigned char>(ch)); });
                if (printable) {
                    glyph_str = std::string(dup);
                }

                throw std::invalid_argument(
                    "IndexedGlyphSet: duplicate glyph detected at indices " + std::to_string(i - 1) + " and " + std::to_string(i) +
                    " Glyph bytes (hex): " + hex_bytes +
                    (printable ? " Glyph text: \"" + glyph_str + "\"" : "")
                );
            }
        }

        // Build map
        for (unsigned i = 0; i < glyph_views_.size(); ++i)
            glyph_to_index_[glyph_views_[i]] = i;
    }

    ~IndexedGlyphSet() = default;

    IndexedGlyphSet(const IndexedGlyphSet& other) = delete;
    IndexedGlyphSet& operator=(const IndexedGlyphSet& other) = delete;

    IndexedGlyphSet(IndexedGlyphSet&& other) noexcept = default;
    IndexedGlyphSet& operator=(IndexedGlyphSet&& other) noexcept = delete;

    std::string_view glyphs() const noexcept { return _glyphs; }
    const char* data() const noexcept { return _glyphs.c_str(); }
    size_t glyph_size() const noexcept { return _glyph_size; }
    size_t size() const noexcept { return glyph_views_.size(); }
    std::string_view name() const noexcept { return _name; }

    unsigned to_index(const std::string_view glyph) const
    {
        auto it = glyph_to_index_.find(glyph);
        if (it == glyph_to_index_.end())
            throw std::out_of_range("IndexedGlyphSet: glyph not found");
        return it->second;
    }

    std::string_view from_index(const unsigned index) const
    {
        if (index >= glyph_views_.size())
            throw std::out_of_range("IndexedGlyphSet: index out of range");
        return glyph_views_[index];
    }

    bool contains(const std::string_view glyph) const
    {
        return glyph_to_index_.count(glyph) != 0;
    }

    auto begin() const { return glyph_views_.begin(); }
    auto end() const { return glyph_views_.end(); }

    // --- Added SIMD glyph index validation method ---
    bool validateGlyphIndexesSIMD(const unsigned* indexes, size_t count) const
    {
        if (_glyphs.empty()) return count == 0;

        const unsigned maxValid = static_cast<unsigned>(glyph_views_.size() - 1);

#ifdef __AVX2__
        size_t i = 0;
        __m256i maxVec = _mm256_set1_epi32(maxValid);

        for (; i + 8 <= count; i += 8) {
            __m256i vals = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(indexes + i));
            __m256i cmp = _mm256_cmpgt_epi32(vals, maxVec);
            if (!_mm256_testz_si256(cmp, cmp)) {
                return false;
            }
        }
        // tail scalar
        for (; i < count; ++i) {
            if (indexes[i] > maxValid) return false;
        }
        return true;
#else
        for (size_t i = 0; i < count; ++i) {
            if (indexes[i] > maxValid) return false;
        }
        return true;
#endif
    }

private:
    const size_t _glyph_size;
    const std::string _glyphs;  // owns all glyph data
    const std::string _name;

    std::vector<std::string_view> glyph_views_;  // index → glyph (view into backing_)
    std::unordered_map<std::string_view, unsigned> glyph_to_index_;  // glyph → index

    static size_t get_glyph_size(const std::string& s)
    {
        unsigned char c = static_cast<unsigned char>(s[0]);
        if (c < 0x80) return 1;
        if ((c >> 5) == 0x6) return 2;
        if ((c >> 4) == 0xE) return 3;
        if ((c >> 3) == 0x1E) return 4;
        throw std::invalid_argument("IndexedGlyphSet: invalid UTF-8 glyph at start");
    }
};
