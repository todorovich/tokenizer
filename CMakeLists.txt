cmake_minimum_required(VERSION 3.20)
project(secure_tokenizer_cpp LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Required Packages and External Dependencies
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

add_subdirectory(external)

# Main Artifacts

add_library(main_library)
add_executable(http_server)

add_subdirectory(src)

# Test Artifacts

add_executable(encryption_test)
add_executable(http_server_test)

add_subdirectory(tests)
