#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <algorithm>
#include <iostream>
#include <stdexcept>

class IndexedGlyphSet
{
public:
    // Construct from a flat UTF-8 string of concatenated glyphs,
    // all glyphs must be the same byte length (inferred from first glyph)
    explicit IndexedGlyphSet(std::string flat_glyphs)
        : backing_(std::move(flat_glyphs))
    {
        if (backing_.empty())
            throw std::invalid_argument("IndexedGlyphSet: input string empty");

        glyph_size_ = get_glyph_size(backing_);
        if (glyph_size_ == 0 || backing_.size() % glyph_size_ != 0)
            throw std::invalid_argument("IndexedGlyphSet: invalid glyph size or input length");

        const size_t num_glyphs = backing_.size() / glyph_size_;
        glyph_views_.reserve(num_glyphs);

        for (size_t i = 0; i < num_glyphs; ++i)
            glyph_views_.emplace_back(&backing_[i * glyph_size_], glyph_size_);

        std::sort(glyph_views_.begin(), glyph_views_.end());

        // Check duplicates by comparing adjacent after sort
        for (size_t i = 1; i < glyph_views_.size(); ++i) {
            if (glyph_views_[i] == glyph_views_[i - 1]) {
                throw std::invalid_argument("IndexedGlyphSet: duplicate glyphs detected");
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
    IndexedGlyphSet& operator=(IndexedGlyphSet&& other) noexcept = default;



    size_t glyph_size() const { return glyph_size_; }
    size_t size() const { return glyph_views_.size(); }

    unsigned to_index(std::string_view glyph) const
    {
        auto it = glyph_to_index_.find(glyph);
        if (it == glyph_to_index_.end())
            throw std::out_of_range("IndexedGlyphSet: glyph not found");
        return it->second;
    }

    std::string_view from_index(unsigned index) const
    {
        if (index >= glyph_views_.size())
            throw std::out_of_range("IndexedGlyphSet: index out of range");
        return glyph_views_[index];
    }

    bool contains(std::string_view glyph) const
    {
        return glyph_to_index_.count(glyph) != 0;
    }

    auto begin() const { return glyph_views_.begin(); }
    auto end() const { return glyph_views_.end(); }

private:
    size_t glyph_size_;
    std::string backing_;  // owns all glyph data
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
