#include "libfpe.hpp"

#include <cstring>
#include <catch2/catch_test_macros.hpp>

class C_Unicode_FPE_Wrapper
{
    const UnicodeFPECipherHandle handle;
public:
    C_Unicode_FPE_Wrapper()
        : handle(unicodefpe_create())
    {}

    int encrypt(
        const char* input,
        size_t input_len,
        char* encrypted,
        size_t output_capacity)
    {
        return unicodefpe_encrypt(
            handle, input, input_len, encrypted, output_capacity
        );
    }
    void decrypt(
        const char* input,
        size_t input_len,
        char* decrypted,
        size_t decrypted_capacity
    )
    {
        unicodefpe_decrypt(
            handle, input, input_len, decrypted, decrypted_capacity
        );
    }

    ~C_Unicode_FPE_Wrapper()
    {
        unicodefpe_destroy(handle);
    }
};


TEST_CASE("Can use fpe encrypt", "[fpe]")
{
    constexpr char input[] = "hello";

    char encrypted[sizeof(input)] = {0};
    char decrypted[sizeof(input)] = {0};

    auto  unicode_fpe = C_Unicode_FPE_Wrapper();

    const int result_code = unicode_fpe.encrypt(
        input, strlen(input), encrypted, sizeof(encrypted)
    );

    REQUIRE(result_code == 0);
    REQUIRE(std::strlen(encrypted) == std::strlen(input));

    unicode_fpe.decrypt(encrypted, strlen(encrypted), decrypted, sizeof(decrypted));

    REQUIRE(std::strcmp(input, decrypted) == 0);
}
