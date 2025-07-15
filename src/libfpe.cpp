#include "libfpe.hpp"
#include "UnicodeFPECipher.hpp"
#include "PreconfiguredIndexedGlyphSet.hpp"
#include <cstring>
#include <new>
#include <stdexcept>

std::vector<uint8_t> test_key(16, 0x00);
std::vector<uint8_t> test_tweak(4, 0x00);

extern "C"
{
    // Create a cipher covering all Unicode
    UnicodeFPECipherHandle unicodefpe_create() {
        try {
            return new UnicodeFPECipher(
                UnicodeGlyphCipherIndex(
                    PreconfiguredIndexedGlyphSet::buildUnicodeGlyphCiphers(test_key, test_tweak),
                    test_key,
                    test_tweak
                )
            );
        }
        catch (...)
        {
            return nullptr;
        }
    }

    // Encrypt UTF-8 input, write to output buffer (must be preallocated).
    // Returns 0 on success, nonzero on error.
    int unicodefpe_encrypt(
        UnicodeFPECipherHandle handle,
        const char* input, size_t input_len,
        char* output, size_t output_capacity
    )
    {
        if (!handle || !input || !output) return 1;
        try
        {
            UnicodeFPECipher* cipher = static_cast<UnicodeFPECipher*>(handle);
            std::string result = cipher->encrypt(std::string_view(input, input_len));
            if (result.size() >= output_capacity) return 2; // Not enough room
            std::memcpy(output, result.data(), result.size());
            output[result.size()] = 0; // null-terminate
            return 0;
        }
        catch (...)
        {
            return 3;
        }
    }

    // Decrypt UTF-8 input, write to output buffer (must be preallocated).
    // Returns 0 on success, nonzero on error.
    int unicodefpe_decrypt(
        UnicodeFPECipherHandle handle,
        const char* input, size_t input_len,
        char* output, size_t output_capacity
    ) {
        if (!handle || !input || !output) return 1;
        try {
            UnicodeFPECipher* cipher = static_cast<UnicodeFPECipher*>(handle);
            std::string result = cipher->decrypt(std::string_view(input, input_len));
            if (result.size() >= output_capacity) return 2; // Not enough room
            std::memcpy(output, result.data(), result.size());
            output[result.size()] = 0; // null-terminate
            return 0;
        } catch (...) {
            return 3;
        }
    }

    // Destroy the cipher
    void unicodefpe_destroy(UnicodeFPECipherHandle handle) {
        delete static_cast<UnicodeFPECipher*>(handle);
    }
} // extern "C"
