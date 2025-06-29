#include "UnicodeFPECipher.hpp"
#include <numeric>  // for std::accumulate
#include <stdexcept>

#include <string_view>
#include <cstdint>
#include <cstring>
#include <utility>

std::pair<uint32_t, size_t> decode_utf8_glyph(std::string_view str, size_t pos)
{
    if (pos >= str.size())
        throw std::out_of_range("Position out of range in decode_utf8_glyph");

    unsigned char lead = static_cast<unsigned char>(str[pos]);

    if (lead < 0x80)
    {
        // 1-byte ASCII
        return {lead, 1};
    }
    else if ((lead >> 5) == 0x6)
    {
        // 2-byte sequence
        if (pos + 1 >= str.size())
            throw std::runtime_error("Truncated UTF-8 sequence");
        uint32_t cp = ((lead & 0x1F) << 6) | (static_cast<unsigned char>(str[pos + 1]) & 0x3F);
        return {cp, 2};
    }
    else if ((lead >> 4) == 0xE)
    {
        // 3-byte sequence
        if (pos + 2 >= str.size())
            throw std::runtime_error("Truncated UTF-8 sequence");
        uint32_t cp = ((lead & 0x0F) << 12) |
                      ((static_cast<unsigned char>(str[pos + 1]) & 0x3F) << 6) |
                      (static_cast<unsigned char>(str[pos + 2]) & 0x3F);
        return {cp, 3};
    }
    else if ((lead >> 3) == 0x1E)
    {
        // 4-byte sequence
        if (pos + 3 >= str.size())
            throw std::runtime_error("Truncated UTF-8 sequence");
        uint32_t cp = ((lead & 0x07) << 18) |
                      ((static_cast<unsigned char>(str[pos + 1]) & 0x3F) << 12) |
                      ((static_cast<unsigned char>(str[pos + 2]) & 0x3F) << 6) |
                      (static_cast<unsigned char>(str[pos + 3]) & 0x3F);
        return {cp, 4};
    }
    else
    {
        throw std::runtime_error("Invalid UTF-8 lead byte");
    }
}


UnicodeFPECipher::UnicodeFPECipher(UnicodeGlyphCipherIndex&& index)
    : cipher_index(std::move(index))
{}

std::string UnicodeFPECipher::encrypt(std::string_view input)
{
    std::vector<std::string> cipher_buffers;
    auto glyph_cipher_indices = parse_and_dispatch(input, cipher_buffers);
    encrypt_cipher_buffers(cipher_buffers);
    return reassemble_output(glyph_cipher_indices, cipher_buffers);
}

std::string UnicodeFPECipher::decrypt(std::string_view input)
{
    std::vector<std::string> cipher_buffers;
    auto glyph_cipher_indices = parse_and_dispatch(input, cipher_buffers);
    decrypt_cipher_buffers(cipher_buffers);
    return reassemble_output(glyph_cipher_indices, cipher_buffers);
}

std::vector<uint32_t> UnicodeFPECipher::parse_and_dispatch(std::string_view input, std::vector<std::string>& cipher_buffers)
{
    size_t pos = 0;
    size_t cipher_count = cipher_index.glyph_ciphers.size() + 1; // +1 for noop cipher

    // --- First pass: count total bytes per cipher ---
    std::vector<size_t> cipher_byte_counts(cipher_count, 0);
    size_t glyph_count = 0;

    size_t temp_pos = 0;
    while (temp_pos < input.size())
    {
        auto [cp, glyph_len] = decode_utf8_glyph(input, temp_pos);
        uint32_t cidx = cipher_index[cp] == cipher_index.noop_cipher
                            ? static_cast<uint32_t>(cipher_count - 1)
                            : static_cast<uint32_t>(&cipher_index[cp] - &cipher_index.glyph_ciphers[0]);

        cipher_byte_counts[cidx] += glyph_len;
        temp_pos += glyph_len;
        ++glyph_count;
    }

    // --- Prepare cipher_buffers with preallocated capacity ---
    cipher_buffers.resize(cipher_count);
    for (size_t i = 0; i < cipher_count; ++i)
    {
        cipher_buffers[i].reserve(cipher_byte_counts[i]);
    }

    // --- Prepare glyph_cipher_indices with exact capacity ---
    std::vector<uint32_t> glyph_cipher_indices;
    glyph_cipher_indices.reserve(glyph_count);

    // --- Second pass: append glyphs to cipher buffers and record indices ---
    pos = 0;
    while (pos < input.size())
    {
        auto [cp, glyph_len] = decode_utf8_glyph(input, pos);
        uint32_t cidx = cipher_index[cp] == cipher_index.noop_cipher
                            ? static_cast<uint32_t>(cipher_count - 1)
                            : static_cast<uint32_t>(&cipher_index[cp] - &cipher_index.glyph_ciphers[0]);

        cipher_buffers[cidx].append(input.data() + pos, glyph_len);
        glyph_cipher_indices.push_back(cidx);
        pos += glyph_len;
    }

    return glyph_cipher_indices;
}
void UnicodeFPECipher::encrypt_cipher_buffers(std::vector<std::string>& cipher_buffers)
{
    size_t cipher_count = cipher_buffers.size();
    for (size_t i = 0; i < cipher_count; ++i)
    {
        GlyphFPECipher& cipher = (i == cipher_count - 1) ? cipher_index.noop_cipher : cipher_index.glyph_ciphers[i];
        cipher_buffers[i] = cipher.encrypt(cipher_buffers[i]);
    }
}

void UnicodeFPECipher::decrypt_cipher_buffers(std::vector<std::string>& cipher_buffers)
{
    size_t cipher_count = cipher_buffers.size();
    for (size_t i = 0; i < cipher_count; ++i)
    {
        GlyphFPECipher& cipher = (i == cipher_count - 1) ? cipher_index.noop_cipher : cipher_index.glyph_ciphers[i];
        cipher_buffers[i] = cipher.decrypt(cipher_buffers[i]);
    }
}

std::string UnicodeFPECipher::reassemble_output(
    const std::vector<uint32_t>& glyph_cipher_indices, const std::vector<std::string>& cipher_buffers
)
{
    size_t cipher_count = cipher_buffers.size();
    std::vector<size_t> cipher_offsets(cipher_count, 0);

    // Pre-calculate total output size
    size_t total_size = 0;
    for (const auto& s : cipher_buffers)
        total_size += s.size();

    std::string output;
    output.resize(total_size);  // Pre-allocate output size exactly

    size_t output_pos = 0;
    for (uint32_t cidx : glyph_cipher_indices)
    {
        const GlyphFPECipher& cipher = (cidx == cipher_count - 1)
            ? cipher_index.noop_cipher
            : cipher_index.glyph_ciphers[cidx];

        size_t glyph_len = cipher.glyphs().glyph_size();

        size_t offset = cipher_offsets[cidx];
        // Copy glyph_len bytes from cipher_buffers[cidx] + offset to output + output_pos
        std::memcpy(&output[output_pos], cipher_buffers[cidx].data() + offset, glyph_len);

        cipher_offsets[cidx] += glyph_len;
        output_pos += glyph_len;
    }

    return output;
}