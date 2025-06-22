#pragma once

#include <vector>
#include <concepts>
#include <cstdint>
#include <fpe.h>

// Concept to constrain DigitType to valid integral types
template<typename T>
concept ValidDigitType = std::same_as<T, std::uint8_t> || std::same_as<T, std::uint16_t> || std::same_as<T, std::uint32_t>;

template <ValidDigitType DigitType = std::uint32_t>
class FF1Cipher
{
  public:
    FF1Cipher(
        const std::vector<uint8_t>& key, const std::vector<uint8_t>& tweak, DigitType radix
    );
    ~FF1Cipher();

    FF1Cipher(const FF1Cipher&) = delete;
    FF1Cipher& operator=(const FF1Cipher&) = delete;
    FF1Cipher(FF1Cipher&&) noexcept;
    FF1Cipher& operator=(FF1Cipher&&) noexcept;

    // Use DigitType for inputs and outputs
    std::vector<DigitType> encrypt(std::vector<DigitType>&& digits) const;
    std::vector<DigitType> decrypt(std::vector<DigitType>&& digits) const;

  private:
    mutable FPE_KEY _key{};
    bool _valid = false;
    DigitType _radix = 10;

    void cleanup();
};
