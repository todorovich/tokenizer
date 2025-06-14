#pragma once
#include <string>

class Base64 {
public:
	Base64() = delete;

	static std::string encode(const std::string& input);
	static std::string decode(const std::string& encoded);
};
