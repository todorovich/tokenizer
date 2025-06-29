#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "IndexedGlyphSet.hpp"
#include "FF1Cipher.hpp"
#include <immintrin.h> // for AVX2 intrinsics if available

#include <cstring>

template <typename T>
class not_null {
    T ptr_;
public:
    // Implicit converting constructor from T*
    not_null(T ptr) : ptr_(ptr) {
        if (ptr_ == nullptr) throw std::invalid_argument("not_null: null pointer");
    }
    not_null(const not_null&) = default;
    not_null(not_null&&) noexcept = default;
    not_null& operator=(const not_null&) = default;
    not_null& operator=(not_null&&) noexcept = default;

    operator T() const noexcept { return ptr_; }
    T get() const noexcept { return ptr_; }
    T operator->() const noexcept { return ptr_; }
    auto& operator*() const noexcept { return *ptr_; }
};

class GlyphFPECipher {
public:
    explicit GlyphFPECipher(
        const IndexedGlyphSet& glyph_set,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& tweak,
        bool noop = false
    )
        : GlyphFPECipher(not_null(&glyph_set), key, tweak, noop)
    {}

    GlyphFPECipher(
        not_null<const IndexedGlyphSet*> glyph_set,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& tweak,
        bool noop = false
    )
        : _glyph_set(glyph_set)
        , _noop(noop)
        , _ff1_cipher(FF1Cipher<uint32_t>(key, tweak, glyph_set->size()))
    {
        _encode_func = noop ? &GlyphFPECipher::encode_noop : &GlyphFPECipher::encode_ff1;
        _decode_func = noop ? &GlyphFPECipher::decode_noop : &GlyphFPECipher::decode_ff1;
    }

    explicit GlyphFPECipher(not_null<const IndexedGlyphSet*> glyph_set)
        : GlyphFPECipher(
            glyph_set,
            std::vector<uint8_t>(16, 0x00),  // dummy 128-bit key
            std::vector<uint8_t>(8, 0x00),   // dummy tweak size (adjust if needed)
            true
        )
    {}

    GlyphFPECipher(const GlyphFPECipher&) = default;
    GlyphFPECipher(GlyphFPECipher&&) noexcept = default;
    GlyphFPECipher& operator=(const GlyphFPECipher&) = default;
    GlyphFPECipher& operator=(GlyphFPECipher&&) noexcept = default;
    ~GlyphFPECipher() = default;

    const IndexedGlyphSet& glyphs() const noexcept {
        return *_glyph_set;
    }

    std::string encrypt(std::string_view utf8_input) const {
        return (this->*_encode_func)(utf8_input);
    }

    std::string decrypt(std::string_view utf8_input) const {
        return (this->*_decode_func)(utf8_input);
    }

    std::string_view getGlyphSetName()const { return _glyph_set->name(); }

    bool operator==(const GlyphFPECipher& other) const noexcept {
        // TODO: Do better
        return this == &other;
    }

private:
    not_null<const IndexedGlyphSet*> _glyph_set;
    bool _noop;
    FF1Cipher<uint32_t> _ff1_cipher;

    using EncodeDecodeFunc = std::string (GlyphFPECipher::*)(std::string_view) const;
    EncodeDecodeFunc _encode_func;
    EncodeDecodeFunc _decode_func;

    std::string encode_noop(const std::string_view utf8_input) const {
        return std::string(utf8_input);
    }

    std::string decode_noop(const std::string_view utf8_input) const {
        return std::string(utf8_input);
    }

    std::string encode_ff1(const std::string_view utf8_input) const {
        auto glyph_indexes = utf8_to_glyph_indexes(utf8_input);
        const auto encrypted_indexes = _ff1_cipher.encrypt(std::move(glyph_indexes));
        return glyph_indexes_to_utf8(encrypted_indexes);
    }

    std::string decode_ff1(const std::string_view utf8_input) const {
        auto glyph_indexes = utf8_to_glyph_indexes(utf8_input);
        const auto decrypted_indexes = _ff1_cipher.decrypt(std::move(glyph_indexes));
        return glyph_indexes_to_utf8(decrypted_indexes);
    }

    std::vector<unsigned int> utf8_to_glyph_indexes(std::string_view utf8_str) const {
        std::vector<unsigned int> indexes;
        indexes.reserve(utf8_str.size());
        size_t glyph_len = _glyph_set->glyph_size();

        size_t pos = 0;
        while (pos < utf8_str.size()) {
            std::string_view glyph = utf8_str.substr(pos, glyph_len);
            indexes.push_back(_glyph_set->to_index(glyph));
            pos += glyph_len;
        }

        return indexes;
    }

    std::string glyph_indexes_to_utf8(const std::vector<unsigned int>& indexes) const {
        std::string result;
        size_t glyph_len = _glyph_set->glyph_size();
        result.reserve(indexes.size() * glyph_len);

        for (unsigned int index : indexes) {
            std::string_view glyph = _glyph_set->from_index(index);
            result.append(glyph.data(), glyph.size());
        }
        return result;
    }

    std::string glyph_indexes_to_utf8_simd(const std::vector<unsigned int>& indexes) const {
        const size_t glyph_len = _glyph_set->glyph_size();
        const size_t total_glyphs = indexes.size();

        // Get contiguous glyph data buffer
        const char* glyph_table = _glyph_set->data(); // or appropriate method returning contiguous glyph bytes

        std::string result;
        result.resize(total_glyphs * glyph_len);
        char* dest = result.data();

        const size_t simd_width = 8;  // AVX2 256-bit / 32-bit lanes

        size_t i = 0;

        // Process full SIMD batches
        for (; i + simd_width <= total_glyphs; i += simd_width) {
            __m256i idx_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&indexes[i]));

            // Multiply indexes by glyph_len to get byte offsets
            __m256i offsets = _mm256_mullo_epi32(idx_vec, _mm256_set1_epi32(static_cast<int>(glyph_len)));

            // For each byte within a glyph, gather bytes from glyph_table
            for (size_t byte_pos = 0; byte_pos < glyph_len; ++byte_pos) {
                // Calculate byte offsets + byte_pos
                __m256i byte_offsets = _mm256_add_epi32(offsets, _mm256_set1_epi32(static_cast<int>(byte_pos)));

                // Gather bytes at those offsets
                __m256i gathered = _mm256_i32gather_epi32(reinterpret_cast<const int*>(glyph_table), byte_offsets, 1);

                // Extract bytes from gathered 32-bit ints and store sequentially
                // This requires extracting the least significant byte of each 32-bit lane
                alignas(32) uint8_t bytes[simd_width];
                for (int lane = 0; lane < 8; ++lane) {
                    bytes[lane] = static_cast<uint8_t>(_mm256_extract_epi32(gathered, lane) & 0xFF);
                }
                std::memcpy(dest + (i * glyph_len) + byte_pos, bytes, simd_width);
            }
        }

        // Process remaining glyphs scalar
        for (; i < total_glyphs; ++i) {
            std::memcpy(dest + i * glyph_len, glyph_table + indexes[i] * glyph_len, glyph_len);
        }

        return result;
    }
};
