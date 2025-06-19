#include <App.h>
#include <string>
#include <string_view>
#include <iostream>
#include <thread>
#include <functional>

#include "AES256ECB.hpp"

constexpr std::string_view STATIC_DOMAIN = "your_domain";
constexpr std::string_view STATIC_SALT   = "your_salt";
constexpr std::string_view STATIC_KEY    = "0123456789ABCDEF0123456789ABCDEF";

inline std::string_view extract_token(std::string_view data) {
	auto end = data.find('\n');
	return data.substr(0, end);
}

void run_server_thread(const std::function<void()>& on_ready = {}) {
	uWS::App()
		.post("/encode/aes256ecb", [](auto* res, auto*) {
			res->onAborted([] {});
			res->onData([res](std::string_view data, bool) {
				thread_local AES256ECB aes{std::string(STATIC_KEY)};
				std::string_view token = extract_token(data);
				std::string encoded = aes.encode(token);
				//std::cout << "[ENCODE] token=[" << token << "] → " << encoded << std::endl;
				res->end(std::move(encoded) + "\n");
			});
		})
		.post("/decode/aes256ecb", [](auto* res, auto*) {
			res->onAborted([] {});
			res->onData([res](std::string_view data, bool) {
				thread_local AES256ECB aes{std::string(STATIC_KEY)};
				std::string_view token = extract_token(data);
				std::string decoded = aes.decode(token);
				//std::cout << "[DECODE] token=[" << token << "] → " << decoded << std::endl;
				res->end(std::move(decoded) + "\n");
			});
		})
		.listen("0.0.0.0",8080, [on_ready](auto* token) {
			if (!token) {
				perror("listen");
				std::exit(1);
			}
			std::cout << "Listening on port 8080\n";
			if (on_ready) on_ready();
		})
		.run();
}