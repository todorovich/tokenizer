#include <iostream>
#include <thread>

#include "WebServer.hpp"

constexpr int num_threads = 1;

int main()
{
	std::vector<std::thread> threads;

	for (int i = 0; i < num_threads; ++i) {
		threads.emplace_back(run_server_thread, []{});
	}

	for (auto& t : threads) t.join();

	std::cout << "Event loop exited!\n";
}
