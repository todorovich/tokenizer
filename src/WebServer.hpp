#pragma once

#include <string_view>
#include <functional>

constexpr std::string_view STATIC_DOMAIN = "your_domain";
constexpr std::string_view STATIC_SALT   = "your_salt";
constexpr std::string_view STATIC_KEY    = "0123456789ABCDEF0123456789ABCDEF";

inline std::string_view extract_token(std::string_view data) {
	auto end = data.find('\n');
	return data.substr(0, end);
}

void run_server_thread(const std::function<void()>& on_ready = {});