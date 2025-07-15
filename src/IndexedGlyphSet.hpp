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
    static int utf8CharLen(unsigned char c) {
        if ((c & 0x80) == 0x00) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        throw std::runtime_error("Invalid UTF-8 encoding");
    }

    static void verifyUniformUtf8CharWidth(const std::string& utf8str)
    {
        int expectedLen = -1;
        for (size_t i = 0; i < utf8str.size();)
        {
            int charLen = utf8CharLen(static_cast<unsigned char>(utf8str[i]));

            if (expectedLen == -1)
                expectedLen = charLen;

            if (charLen != expectedLen)
                throw std::runtime_error("Inconsistent UTF-8 character widths");

            i += charLen;
        }
    }

    void _check_for_duplicates() const
    {
        // Check duplicates by comparing adjacent after sort
        for (size_t i = 1; i < _glyphs.size(); ++i) {
            if (_glyphs[i] == _glyphs[i - 1]) {
                // Format glyph bytes as hex string for clarity
                const auto& dup = _glyphs[i];
                std::string hex_bytes;
                for (unsigned char c : dup)
                {
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
    }

public:
    // Construct from a flat UTF-8 string of concatenated glyphs,
    // all glyphs must be the same byte length (inferred from first glyph)
    explicit IndexedGlyphSet(std::string&& name, const std::string& flat_glyphs)
        : _glyph_size(get_glyph_size(flat_glyphs))
        , _name(std::move(name))
    {
        if (flat_glyphs.empty())
            throw std::invalid_argument("IndexedGlyphSet: input string empty");

        if (flat_glyphs.size() < 2)
            throw std::invalid_argument("IndexedGlyphSet: not enough characters to form an indexed set, requires as size >1");

        if (flat_glyphs.size() % _glyph_size != 0)
            throw std::invalid_argument("IndexedGlyphSet: invalid glyph size or input length");

        verifyUniformUtf8CharWidth(flat_glyphs);

        const size_t num_glyphs = flat_glyphs.size() / _glyph_size;
        _glyphs.reserve(num_glyphs);

        for (size_t i = 0; i < num_glyphs; ++i)
            _glyphs.emplace_back(&flat_glyphs[i * _glyph_size], _glyph_size);

        std::ranges::sort(_glyphs);

        glyph_to_index_.clear();
        for (unsigned i = 0; i < _glyphs.size(); ++i) {
            glyph_to_index_[_glyphs[i]] = i;
        }

        _check_for_duplicates();
    }

    ~IndexedGlyphSet() = default;

    IndexedGlyphSet(const IndexedGlyphSet& other) = delete;
    IndexedGlyphSet& operator=(const IndexedGlyphSet& other) = delete;

    IndexedGlyphSet(IndexedGlyphSet&& other) noexcept = default;
    IndexedGlyphSet& operator=(IndexedGlyphSet&& other) noexcept = delete;

    std::vector<std::string> glyphs() const noexcept { return _glyphs; }
    //const char* data() const noexcept { return _glyphs.c_str(); }
    size_t glyph_size() const noexcept { return _glyph_size; }
    size_t size() const noexcept { return _glyphs.size(); }
    std::string_view name() const noexcept { return _name; }

    unsigned to_index(const std::string_view glyph) const
    {
        const auto it = glyph_to_index_.find(glyph);
        if (it == glyph_to_index_.end())
            throw std::out_of_range("IndexedGlyphSet: glyph not found");
        return it->second;
    }

    std::string_view from_index(const unsigned index) const
    {
        if (index >= _glyphs.size())
            throw std::out_of_range("IndexedGlyphSet: index out of range");
        return _glyphs[index];
    }

    bool contains(const std::string_view glyph) const
    {
        return glyph_to_index_.contains(glyph);
    }

    auto begin() const { return _glyphs.begin(); }
    auto end() const { return _glyphs.end(); }

    // --- Added SIMD glyph index validation method ---
    bool validateGlyphIndexesSIMD(const unsigned* indexes, size_t count) const
    {
        if (_glyphs.empty()) return count == 0;

        const auto maxValid = static_cast<unsigned>(_glyphs.size() - 1);

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
    //const std::string _glyphs;  // owns all glyph data
    const std::string _name;

    std::vector<std::string> _glyphs;  // index → glyph (view into backing_)
    std::unordered_map<std::string_view, unsigned> glyph_to_index_;  // glyph → index

    static size_t get_glyph_size(const std::string& s)
    {
        const auto c = static_cast<unsigned char>(s[0]);
        if (c < 0x80) return 1;
        if ((c >> 5) == 0x6) return 2;
        if ((c >> 4) == 0xE) return 3;
        if ((c >> 3) == 0x1E) return 4;
        throw std::invalid_argument("IndexedGlyphSet: invalid UTF-8 glyph at start");
    }
};
