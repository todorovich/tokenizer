#include "FF1Cipher.hpp"
#include <stdexcept>
#include <utility>
#include <cstring>
#include <fpe.h>
#include <vector>

namespace {
    // Validates and converts digits in-place (no extra copy)
    void validate_digits(const std::vector<uint32_t>& input, const uint32_t radix) {
        for (const auto digit : input) {
            if (digit >= radix) {
                throw std::invalid_argument("Digit out of range");
            }
        }
    }
}

FF1Cipher::FF1Cipher(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& tweak,
    const int32_t radix
) : _radix(radix)
{
    const int bits = static_cast<int>(key.size() * 8);
    if (bits != 128 && bits != 192 && bits != 256)
        throw std::invalid_argument("FF1Cipher: Key must be 128, 192, or 256 bits");

    if (FPE_set_ff1_key(key.data(), bits, tweak.data(), tweak.size(), _radix, &_key) != 0)
        throw std::runtime_error("FF1Cipher: Failed to initialize FF1 key");

    _valid = true;
}

FF1Cipher::~FF1Cipher() noexcept
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
        _valid = other._valid;
        _radix = other._radix;

        other._valid = false;
        std::memset(&other._key, 0, sizeof(FPE_KEY));
    }
    return *this;
}

std::vector<uint32_t> FF1Cipher::encrypt(std::vector<uint32_t>&& digits) const
{
    if (!_valid)
        throw std::logic_error("FF1Cipher not initialized");
    if (digits.empty())
        return std::move(digits);

    validate_digits(digits, _radix);

    // Allocate output buffer
    std::vector<uint32_t> out(digits.size());

    // reinterpret_cast is safe if sizeof(uint32_t) == sizeof(unsigned int)
    FPE_ff1_encrypt(
        digits.data(),
        reinterpret_cast<unsigned int*>(out.data()),
        static_cast<unsigned int>(digits.size()),
        const_cast<FPE_KEY*>(&_key),
        FPE_ENCRYPT
    );

    return out;
}

std::vector<uint32_t> FF1Cipher::decrypt(std::vector<uint32_t>&& digits) const
{
    if (!_valid)
        throw std::logic_error("FF1Cipher not initialized");
    if (digits.empty())
        return std::move(digits);

    validate_digits(digits, _radix);

    std::vector<uint32_t> out(digits.size());

    FPE_ff1_encrypt(
        digits.data(),
        reinterpret_cast<unsigned int*>(out.data()),
        static_cast<unsigned int>(digits.size()),
        &_key,
        FPE_DECRYPT
    );

    return out;
}

void FF1Cipher::cleanup() noexcept
{
    if (_valid)
    {
        FPE_unset_ff1_key(&_key);
        _valid = false;
    }
}
