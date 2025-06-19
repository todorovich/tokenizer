#include "FF1Cipher.hpp"
#include <stdexcept>
#include <utility>
#include <cstring>
#include <fpe.h>

FF1Cipher::FF1Cipher(
    const std::vector<uint8_t>& key, const std::vector<uint8_t>& tweak, unsigned int radix
)
    : _radix(radix)
{
    int bits = static_cast<int>(key.size() * 8);
    if (bits != 128 && bits != 192 && bits != 256)
        throw std::invalid_argument("FF1Cipher: Key must be 128, 192, or 256 bits");

    if (FPE_set_ff1_key(key.data(), bits, tweak.data(), tweak.size(), radix, &_key) != 0)
        throw std::runtime_error("FF1Cipher: Failed to initialize FF1 key");

    _valid = true;
}

FF1Cipher::~FF1Cipher()
{
    cleanup();
}

FF1Cipher::FF1Cipher(FF1Cipher&& other) noexcept
    : _key(other._key)
    , _valid(other._valid)
    , _radix(other._radix)
{
    other._valid = false;
    std::memset(&other._key, 0, sizeof(FPE_KEY));
}

FF1Cipher& FF1Cipher::operator=(FF1Cipher&& other) noexcept
{
    if (this != &other)
    {
        cleanup();
        _key = other._key;
        _radix = other._radix;
        _valid = other._valid;

        other._valid = false;
        std::memset(&other._key, 0, sizeof(FPE_KEY));
    }

    return *this;
}

std::vector<unsigned int> FF1Cipher::encrypt(std::vector<unsigned int>&& digits) const
{
    if (!_valid)
        throw std::logic_error("FF1Cipher not initialized");

    std::vector<unsigned int> out(digits.size());

    FPE_ff1_encrypt(
        digits.data(), out.data(),digits.size(), const_cast<FPE_KEY*>(&_key), FPE_ENCRYPT
    );

    return out;
}

std::vector<unsigned int> FF1Cipher::decrypt(std::vector<unsigned int>&& digits) const
{
    if (!_valid)
        throw std::logic_error("FF1Cipher not initialized");

    std::vector<unsigned int> out(digits.size());

    FPE_ff1_encrypt(
        digits.data(), out.data(),digits.size(), const_cast<FPE_KEY*>(&_key), FPE_DECRYPT
    );

    return out;
}

void FF1Cipher::cleanup()
{
    if (_valid)
    {
        FPE_unset_ff1_key(&_key);
        _valid = false;
    }
}
