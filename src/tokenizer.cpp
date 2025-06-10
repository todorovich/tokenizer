#include "tokenizer.hpp"

#include <cstring>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <blake3.h>

const std::string ALPHABET = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";


std::string encode_token(
    const std::string& token,
    const std::string& key,
    const std::string& salt_key,
    const std::string& domain
)
{
	std::string input = domain + ":" + token;

	unsigned char salt[EVP_MAX_MD_SIZE];
	unsigned int salt_len;
	HMAC(EVP_sha256(), salt_key.data(), salt_key.size(),
		 reinterpret_cast<const unsigned char*>(input.data()), input.size(),
		 salt, &salt_len);

	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hash_len;
	HMAC(EVP_sha256(), key.data(), key.size(), salt, salt_len, hash, &hash_len);

	std::string out;
	for (std::size_t i = 0; i < token.size(); ++i)
		out += ALPHABET[hash[i] % ALPHABET.size()];

	return out;
}

std::string encode_token2(
	const std::string& token,
	const std::string& key,
	const std::string& salt_key,
	const std::string& domain
)
{
	std::string input = domain + ":" + token;

	// Combine key and salt_key for a single HMAC
	std::string full_key = key + salt_key;

	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hash_len;
	HMAC(EVP_sha512(), full_key.data(), full_key.size(),
		 reinterpret_cast<const unsigned char*>(input.data()), input.size(),
		 hash, &hash_len);

	std::string out;
	out.reserve(token.size());
	for (size_t i = 0; i < token.size(); ++i) {
		// Bias-free mapping
		out += ALPHABET[(hash[i] * ALPHABET.size()) / 256];
	}

	return out;
}


std::string encode_token3(
	const std::string& token,
	const std::string& key,
	const std::string& salt_key,
	const std::string& domain
)
{
	std::string input = domain + ":" + token;

	// Combine key and salt_key for a single HMAC
	std::string full_key = key + salt_key;

	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hash_len;
	HMAC(EVP_sha256(), full_key.data(), full_key.size(),
		 reinterpret_cast<const unsigned char*>(input.data()), input.size(),
		 hash, &hash_len);

	std::string out(token.size(), 'A');
	for (size_t i = 0; i < token.size(); ++i) {
		out[i] = ALPHABET[(hash[i] * ALPHABET.size()) / 256];
	}

	return out;
}

std::string encode_token4(
	const std::string& token,
	const std::string& key,
	const std::string& salt_key,
	const std::string& domain
)
{
	// Compose input: domain:token
	std::string input;
	input.reserve(domain.size() + 1 + token.size());
	input.append(domain).append(1, ':').append(token);

	// Avoid allocation in full_key by copying into fixed buffer
	unsigned char full_key[64];
	std::memcpy(full_key, key.data(), key.size());
	std::memcpy(full_key + key.size(), salt_key.data(), salt_key.size());
	size_t full_key_len = key.size() + salt_key.size();

	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hash_len;
	HMAC(EVP_sha256(), full_key, static_cast<int>(full_key_len),
		 reinterpret_cast<const unsigned char*>(input.data()), input.size(),
		 hash, &hash_len);

	std::string out(token.size(), 'A');
	for (size_t i = 0; i < token.size(); ++i) {
		out[i] = ALPHABET[(hash[i] * ALPHABET.size()) / 256];
	}

	return out;
}

std::string encode_token5(
	const std::string& token,
	const std::string& key,
	const std::string& salt_key,
	const std::string& domain
)
{
	// Compose input: domain:token
	std::string input;
	input.reserve(domain.size() + 1 + token.size());
	input.append(domain).append(1, ':').append(token);

	// Build full_key
	unsigned char full_key[64] = {0};
	size_t total_key_len = key.size() + salt_key.size();
	if (total_key_len > 64) total_key_len = 64;
	std::memcpy(full_key, key.data(), key.size());
	std::memcpy(full_key + key.size(), salt_key.data(), salt_key.size());

	// Prepare ipad and opad
	unsigned char k_ipad[64], k_opad[64];
	for (int i = 0; i < 64; ++i) {
		k_ipad[i] = full_key[i] ^ 0x36;
		k_opad[i] = full_key[i] ^ 0x5c;
	}

	// Inner SHA256 using EVP
	unsigned char inner_hash[EVP_MAX_MD_SIZE];
	unsigned int inner_len = 0;
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
	EVP_DigestUpdate(ctx, k_ipad, 64);
	EVP_DigestUpdate(ctx, input.data(), input.size());
	EVP_DigestFinal_ex(ctx, inner_hash, &inner_len);

	// Outer SHA256
	unsigned char final_hash[EVP_MAX_MD_SIZE];
	unsigned int final_len = 0;
	EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
	EVP_DigestUpdate(ctx, k_opad, 64);
	EVP_DigestUpdate(ctx, inner_hash, inner_len);
	EVP_DigestFinal_ex(ctx, final_hash, &final_len);

	EVP_MD_CTX_free(ctx);

	std::string out(token.size(), 'A');
	for (size_t i = 0; i < token.size(); ++i) {
		out[i] = ALPHABET[(final_hash[i] * ALPHABET.size()) / 256];
	}

	return out;
}

std::string encode_token6(
    const std::string& token,
    const std::string& key,
    const std::string& salt_key,
    const std::string& domain
) {
    // Compose input: domain:token:salt_key
    std::string input;
    input.reserve(domain.size() + 1 + token.size() + 1 + salt_key.size());
    input.append(domain).append(1, ':').append(token).append(1, ':').append(salt_key);

    unsigned char full_key[64] = {0};
    size_t key_len = key.size() > 64 ? 64 : key.size();
    std::memcpy(full_key, key.data(), key_len);

    unsigned char k_ipad[64], k_opad[64];
    for (int i = 0; i < 64; ++i) {
        k_ipad[i] = full_key[i] ^ 0x36;
        k_opad[i] = full_key[i] ^ 0x5c;
    }

    unsigned char inner_hash[EVP_MAX_MD_SIZE];
    unsigned int inner_len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, k_ipad, 64);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    EVP_DigestFinal_ex(ctx, inner_hash, &inner_len);

    unsigned char final_hash[EVP_MAX_MD_SIZE];
    unsigned int final_len = 0;
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, k_opad, 64);
    EVP_DigestUpdate(ctx, inner_hash, inner_len);
    EVP_DigestFinal_ex(ctx, final_hash, &final_len);

    EVP_MD_CTX_free(ctx);

    std::string out(token.size(), 'A');
    for (size_t i = 0; i < token.size(); ++i)
        out[i] = ALPHABET[(final_hash[i] * ALPHABET.size()) / 256];

    return out;
}

std::string encode_token7(
    const std::string& token,
    const std::string& key,
    const std::string& salt_key,
    const std::string& domain
) {
	std::string input;
	input.reserve(domain.size() + 1 + token.size() + 1 + salt_key.size());
	input.append(domain).append(1, ':').append(token).append(1, ':').append(salt_key);

	blake3_hasher hasher;
	blake3_hasher_init_keyed(&hasher, reinterpret_cast<const uint8_t*>(key.data()));
	blake3_hasher_update(&hasher, input.data(), input.size());
	uint8_t hash[32];
	blake3_hasher_finalize(&hasher, hash, sizeof(hash));

	std::string result(token.size(), 'A');
	for (size_t i = 0; i < token.size(); ++i)
		result[i] = ALPHABET[(hash[i] * ALPHABET.size()) / 256];

	return result;
}