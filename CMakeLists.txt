cmake_minimum_required(VERSION 3.20)
project(secure_tokenizer_cpp LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)

include(FetchContent)

message(STATUS "JAVA_HOME is: $ENV{JAVA_HOME}")

# Required Packages and External Dependencies
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(JNI REQUIRED)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


add_subdirectory(external)

# Main Artifacts

add_library(fpe_cpp STATIC)
add_library(fpe SHARED)
add_library(fpe-jni SHARED)

add_executable(http_server)

# Test Artifacts

add_executable(encryption_test)
add_executable(http_server_test)
add_executable(libfpe_test)

# Install header(s) to ${PROJECT_SOURCE_DIR}/install/include/
install(DIRECTORY include/
    DESTINATION ${CMAKE_SOURCE_DIR}/install/include
    FILES_MATCHING PATTERN "*.h"
)

if (UNIX AND NOT APPLE)  # Applies to WSL/Linux
    target_compile_options(fpe_cpp PRIVATE -mavx2)
    target_compile_options(http_server PRIVATE -mavx2)
    target_compile_options(encryption_test PRIVATE -mavx2)
    target_compile_options(http_server_test PRIVATE -mavx2)

    if(DEFINED ENV{CLION_IDE})
        target_compile_definitions(fpe_cpp PRIVATE __AVX2__)
        target_compile_definitions(http_server PRIVATE __AVX2__)
        target_compile_definitions(encryption_test PRIVATE __AVX2__)
        target_compile_definitions(http_server_test PRIVATE __AVX2__)
    endif()
endif()

add_subdirectory(src)

# Test Artifacts



add_subdirectory(tests)


