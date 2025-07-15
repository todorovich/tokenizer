#pragma once

#include <vector>
#include <cstdint>
#include <fpe.h>

class FF1Cipher
{
  public:
    FF1Cipher(
        const std::vector<uint8_t>& key, const std::vector<uint8_t>& tweak, int32_t radix
    );
    ~FF1Cipher() noexcept;

    FF1Cipher(const FF1Cipher&) = delete;
    FF1Cipher& operator=(const FF1Cipher&) = delete;
    FF1Cipher(FF1Cipher&&) noexcept;
    FF1Cipher& operator=(FF1Cipher&&) noexcept;

    std::vector<uint32_t> encrypt(std::vector<uint32_t>&& digits) const;
    std::vector<uint32_t> decrypt(std::vector<uint32_t>&& digits) const;

  private:
    mutable FPE_KEY _key{};
    bool _valid = false;
    int32_t _radix = 10;

    void cleanup() noexcept;
};
