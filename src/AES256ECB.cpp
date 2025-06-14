#include "AES256ECB.hpp"

#include <iostream>
#include <stdexcept>
#include "Base64.hpp"
#include <openssl/aes.h>

AES256ECB::AES256ECB(const std::string& key) : _key(key.begin(), key.end()) {
    if (_key.size() != 32)
        throw std::invalid_argument("Key must be 32 bytes for AES-256");
}

std::string AES256ECB::pkcs7_pad(std::string_view input) {
    size_t pad_len = 16 - (input.size() % 16);
    std::string result;
    result.reserve(input.size() + pad_len);
    result.append(input);
    result.append(pad_len, static_cast<char>(pad_len));
    return result;
}

std::string AES256ECB::pkcs7_unpad(std::string_view input) {
    if (input.empty()) throw std::runtime_error("Invalid padding");
    uint8_t pad = static_cast<uint8_t>(input.back());
    if (pad == 0 || pad > 16 || pad > input.size())
        throw std::runtime_error("Invalid padding byte");

    for (size_t i = input.size() - pad; i < input.size(); ++i) {
        if (static_cast<uint8_t>(input[i]) != pad)
            throw std::runtime_error("Inconsistent padding bytes");
    }

    return std::string(input.substr(0, input.size() - pad));
}

std::string AES256ECB::encode(std::string_view plaintext) const
{
    std::string padded = pkcs7_pad(plaintext);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Encrypt ctx failed");

    EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, _key.data(), nullptr);
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    std::string out(padded.size() + AES_BLOCK_SIZE, '\0');
    int out_len1 = 0, out_len2 = 0;

    int r1 = EVP_EncryptUpdate(ctx, reinterpret_cast<uint8_t*>(&out[0]), &out_len1,
                      reinterpret_cast<const uint8_t*>(padded.data()), padded.size());
    int r2 = EVP_EncryptFinal_ex(ctx, reinterpret_cast<uint8_t*>(&out[0]) + out_len1, &out_len2);

    EVP_CIPHER_CTX_free(ctx);
    out.resize(out_len1 + out_len2);

    return Base64::encode(out);
}

std::string AES256ECB::decode(std::string_view ciphertext) const {
    std::string binary = Base64::decode(std::string(ciphertext));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Decrypt ctx failed");

    EVP_DecryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, _key.data(), nullptr);
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    std::string out(binary.size(), '\0');
    int out_len1 = 0, out_len2 = 0;

    EVP_DecryptUpdate(ctx, reinterpret_cast<uint8_t*>(&out[0]), &out_len1,
                      reinterpret_cast<const uint8_t*>(binary.data()), binary.size());
    EVP_DecryptFinal_ex(ctx, reinterpret_cast<uint8_t*>(&out[0]) + out_len1, &out_len2);
    EVP_CIPHER_CTX_free(ctx);
    out.resize(out_len1 + out_len2);

    return pkcs7_unpad(out);
}
