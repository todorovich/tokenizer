#include "FF1Cipher.hpp"
#include <stdexcept>
#include <utility>
#include <cstring>
#include <fpe.h>

// Combined validation and conversion helper
template <typename DigitType>
static std::vector<unsigned int> validate_and_convert(const std::vector<DigitType>& input, DigitType radix) {
    std::vector<unsigned int> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] >= radix) {
            throw std::invalid_argument("Digit out of range");
        }
        output[i] = static_cast<unsigned int>(input[i]);
    }
    return output;
}

// Conversion back helper remains unchanged
template <typename DigitType>
static std::vector<DigitType> from_uint_vec(const std::vector<unsigned int>& input) {
    std::vector<DigitType> out(input.size());
    for (size_t i = 0; i < input.size(); ++i)
        out[i] = static_cast<DigitType>(input[i]);
    return out;
}

template <ValidDigitType DigitType>
FF1Cipher<DigitType>::FF1Cipher(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& tweak,
    DigitType radix
) : _radix(static_cast<unsigned int>(radix))
{
    int bits = static_cast<int>(key.size() * 8);
    if (bits != 128 && bits != 192 && bits != 256)
        throw std::invalid_argument("FF1Cipher: Key must be 128, 192, or 256 bits");

    if (FPE_set_ff1_key(key.data(), bits, tweak.data(), tweak.size(), _radix, &_key) != 0)
        throw std::runtime_error("FF1Cipher: Failed to initialize FF1 key");

    _valid = true;
}

template <ValidDigitType DigitType>
FF1Cipher<DigitType>::~FF1Cipher()
{
    cleanup();
}

template <ValidDigitType DigitType>
FF1Cipher<DigitType>::FF1Cipher(FF1Cipher&& other) noexcept
    : _key(other._key)
    , _valid(other._valid)
    , _radix(other._radix)
{
    other._valid = false;
    std::memset(&other._key, 0, sizeof(FPE_KEY));
}

template <ValidDigitType DigitType>
FF1Cipher<DigitType>& FF1Cipher<DigitType>::operator=(FF1Cipher&& other) noexcept
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

template <ValidDigitType DigitType>
std::vector<DigitType> FF1Cipher<DigitType>::encrypt(std::vector<DigitType>&& digits) const
{
    if (!_valid)
        throw std::logic_error("FF1Cipher not initialized");

    // Combined validation and conversion
    auto input_u32 = validate_and_convert(digits, static_cast<DigitType>(_radix));
    std::vector<unsigned int> out(input_u32.size());

    FPE_ff1_encrypt(
        input_u32.data(),
        out.data(),
        static_cast<unsigned int>(input_u32.size()),
        const_cast<FPE_KEY*>(&_key),
        FPE_ENCRYPT
    );

    return from_uint_vec<DigitType>(out);
}

template <ValidDigitType DigitType>
std::vector<DigitType> FF1Cipher<DigitType>::decrypt(std::vector<DigitType>&& digits) const
{
    if (!_valid)
        throw std::logic_error("FF1Cipher not initialized");

    // Combined validation and conversion
    auto input_u32 = validate_and_convert(digits, static_cast<DigitType>(_radix));
    std::vector<unsigned int> out(input_u32.size());

    FPE_ff1_encrypt(
        input_u32.data(),
        out.data(),
        static_cast<unsigned int>(input_u32.size()),
        const_cast<FPE_KEY*>(&_key),
        FPE_DECRYPT
    );

    return from_uint_vec<DigitType>(out);
}

template <ValidDigitType DigitType>
void FF1Cipher<DigitType>::cleanup()
{
    if (_valid)
    {
        FPE_unset_ff1_key(&_key);
        _valid = false;
    }
}

// Explicit template instantiations
template class FF1Cipher<std::uint8_t>;
template class FF1Cipher<std::uint16_t>;
template class FF1Cipher<std::uint32_t>;
