#pragma once

#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

    typedef void *UnicodeFPECipherHandle;

    // Create a cipher that covers all Unicode
    UnicodeFPECipherHandle unicodefpe_create();

    // Encrypt: input/output are UTF-8 strings. Returns 0 on success.
    int unicodefpe_encrypt(
        UnicodeFPECipherHandle handle,
        const char* input,
        size_t input_len,
        char* output,
        size_t output_capacity
    );

    // Decrypt: input/output are UTF-8 strings. Returns 0 on success.
    int unicodefpe_decrypt(
        UnicodeFPECipherHandle handle,
        const char* input,
        size_t input_len,
        char* output,
        size_t output_capacity
    );

    // Destroy the cipher object.
    void unicodefpe_destroy(UnicodeFPECipherHandle handle);

#ifdef __cplusplus
}
#endif
