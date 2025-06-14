#pragma once

#include <openssl/evp.h>
#include <string>
#include <vector>

class AES256ECB {
public:
	explicit AES256ECB(const std::string& key); // Must be 32 bytes

	std::string encode(std::string_view plaintext) const;
	std::string decode(std::string_view ciphertext) const;

private:
	const std::vector<uint8_t> _key;

	static std::string pkcs7_pad(std::string_view input);
	static std::string pkcs7_unpad(std::string_view input);
};
