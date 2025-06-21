#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "UnicodeGlyphCipherIndex.hpp"
#include "GlyphFPECipher.hpp"

class UnicodeFPECipher
{
public:
    explicit UnicodeFPECipher(UnicodeGlyphCipherIndex&& index);

    std::string encrypt(std::string_view input);
    std::string decrypt(std::string_view input);

private:
    UnicodeGlyphCipherIndex cipher_index;

    std::vector<uint32_t> parse_and_dispatch(std::string_view input, std::vector<std::string>& cipher_buffers);
    void encrypt_cipher_buffers(std::vector<std::string>& cipher_buffers);
    void decrypt_cipher_buffers(std::vector<std::string>& cipher_buffers);
    std::string reassemble_output(const std::vector<uint32_t>& glyph_cipher_indices, const std::vector<std::string>& cipher_buffers);
};
