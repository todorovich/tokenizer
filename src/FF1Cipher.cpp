#include "FF1Cipher.hpp"
#include <stdexcept>
#include <utility>
#include <cstring>
#include <fpe.h>

static std::vector<uint32_t> validate_and_convert(const std::vector<uint32_t>& input, uint32_t radix) {
    std::vector<uint32_t> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] >= radix) {
            throw std::invalid_argument("Digit out of range");
        }
        output[i] = static_cast<uint32_t>(input[i]);
    }
    return output;
}

static std::vector<uint32_t> from_uint_vec(const std::vector<uint32_t>& input) {
    std::vector<uint32_t> out(input.size());
    for (size_t i = 0; i < input.size(); ++i)
        out[i] = static_cast<uint32_t>(input[i]);
    return out;
}

FF1Cipher::FF1Cipher(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& tweak,
    uint32_t radix
) : _radix(static_cast<unsigned int>(radix))
{
    int bits = static_cast<int>(key.size() * 8);
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


    // Combined validation and conversion
    auto input_u32 = validate_and_convert(digits, static_cast<uint32_t>(_radix));
    std::vector<unsigned int> out(input_u32.size());

    FPE_ff1_encrypt(
        input_u32.data(),
        out.data(),
        static_cast<unsigned int>(input_u32.size()),
        const_cast<FPE_KEY*>(&_key),
        FPE_ENCRYPT
    );

    return from_uint_vec(out);
}

std::vector<uint32_t> FF1Cipher::decrypt(std::vector<uint32_t>&& digits) const
{
    if (!_valid)
        throw std::logic_error("FF1Cipher not initialized");

    if (digits.empty())
        return std::move(digits);

    // Combined validation and conversion
    auto input_u32 = validate_and_convert(digits, static_cast<uint32_t>(_radix));
    std::vector<unsigned int> out(input_u32.size());

    FPE_ff1_encrypt(
        input_u32.data(),
        out.data(),
        static_cast<unsigned int>(input_u32.size()),
        &_key,
        FPE_DECRYPT
    );

    return from_uint_vec(out);
}

void FF1Cipher::cleanup() noexcept
{
    if (_valid)
    {
        FPE_unset_ff1_key(&_key);
        _valid = false;
    }
}
