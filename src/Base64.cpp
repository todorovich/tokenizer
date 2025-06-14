#include "Base64.hpp"

#include <fastavxbase64.h>
#include <string>

std::string Base64::encode(const std::string& input) {
	size_t encoded_len = (input.size() + 2) / 3 * 4;
	std::string out(encoded_len, '\0');
	fast_avx2_base64_encode(out.data(), input.data(), input.size());
	return out;
}

std::string Base64::decode(const std::string& encoded) {
	size_t decoded_len = (encoded.size() * 3) / 4;
	std::string out(decoded_len, '\0');
	size_t actual_len = fast_avx2_base64_decode(out.data(), encoded.data(), encoded.size());
	out.resize(actual_len); // trim padding
	return out;
}