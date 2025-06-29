cmake_minimum_required(VERSION 3.20)
project(secure_tokenizer_cpp LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)

include(FetchContent)

# Required Packages and External Dependencies
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

add_subdirectory(external)

# Main Artifacts

add_library(main_library)
add_executable(http_server)
add_executable(encryption_test)
add_executable(http_server_test)

if (UNIX AND NOT APPLE)  # Applies to WSL/Linux
    target_compile_options(main_library PRIVATE -mavx2)
    target_compile_options(http_server PRIVATE -mavx2)
    target_compile_options(encryption_test PRIVATE -mavx2)
    target_compile_options(http_server_test PRIVATE -mavx2)

    if(DEFINED ENV{CLION_IDE})
        target_compile_definitions(main_library PRIVATE __AVX2__)
        target_compile_definitions(http_server PRIVATE __AVX2__)
        target_compile_definitions(encryption_test PRIVATE __AVX2__)
        target_compile_definitions(http_server_test PRIVATE __AVX2__)
    endif()
endif()

add_subdirectory(src)

# Test Artifacts



add_subdirectory(tests)


