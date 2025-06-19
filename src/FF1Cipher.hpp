#pragma once
#include <cstdint>
#include <fpe.h>
#include <openssl/aes.h>
#include <vector>

class FF1Cipher
{
  public:
    FF1Cipher(
        const std::vector<uint8_t>& key, const std::vector<uint8_t>& tweak, unsigned int radix
    );
    ~FF1Cipher();

    FF1Cipher(const FF1Cipher&) = delete;
    FF1Cipher& operator=(const FF1Cipher&) = delete;
    FF1Cipher(FF1Cipher&&) noexcept;
    FF1Cipher& operator=(FF1Cipher&&) noexcept;

    std::vector<unsigned int> encrypt(std::vector<unsigned int>&& digits) const;
    std::vector<unsigned int> decrypt(std::vector<unsigned int>&& digits) const;

  private:
    FPE_KEY _key{};
    bool _valid = false;
    unsigned int _radix = 10;

    void cleanup();
};
