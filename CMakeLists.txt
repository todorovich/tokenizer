cmake_minimum_required(VERSION 3.20)
project(secure_tokenizer_cpp LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

#blake3
FetchContent_Declare(
    blake3
    GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(blake3)

set(BLAKE3_SRC
        ${blake3_SOURCE_DIR}/c/blake3.c
        ${blake3_SOURCE_DIR}/c/blake3_dispatch.c
        ${blake3_SOURCE_DIR}/c/blake3_portable.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    include(CheckCCompilerFlag)

    check_c_compiler_flag("-msse2"   HAS_SSE2)
    check_c_compiler_flag("-msse4.1" HAS_SSE41)
    check_c_compiler_flag("-mavx2"   HAS_AVX2)
    check_c_compiler_flag("-mavx512f" HAS_AVX512)

    if(HAS_SSE2)
        list(APPEND BLAKE3_SRC ${blake3_SOURCE_DIR}/c/blake3_sse2.c)
    endif()
    if(HAS_SSE41)
        list(APPEND BLAKE3_SRC ${blake3_SOURCE_DIR}/c/blake3_sse41.c)
    endif()
    if(HAS_AVX2)
        list(APPEND BLAKE3_SRC ${blake3_SOURCE_DIR}/c/blake3_avx2.c)
    endif()
    if(HAS_AVX512)
        list(APPEND BLAKE3_SRC ${blake3_SOURCE_DIR}/c/blake3_avx512.c)
    endif()
endif()

if(HAS_SSE2)
    set_source_files_properties(${blake3_SOURCE_DIR}/c/blake3_sse2.c PROPERTIES COMPILE_FLAGS "-msse2")
endif()
if(HAS_SSE41)
    set_source_files_properties(${blake3_SOURCE_DIR}/c/blake3_sse41.c PROPERTIES COMPILE_FLAGS "-msse4.1")
endif()
if(HAS_AVX2)
    set_source_files_properties(${blake3_SOURCE_DIR}/c/blake3_avx2.c PROPERTIES COMPILE_FLAGS "-mavx2")
endif()
if(HAS_AVX512)
    set_source_files_properties(${blake3_SOURCE_DIR}/c/blake3_avx512.c PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512vl")
endif()

add_library(blake3 STATIC ${BLAKE3_SRC})
set_target_properties(blake3 PROPERTIES LINKER_LANGUAGE C)
target_include_directories(blake3 PUBLIC ${blake3_SOURCE_DIR}/c)
add_library(BLAKE3::blake3 ALIAS blake3)


FetchContent_Declare(
    Google10000Words
    GIT_REPOSITORY https://github.com/first20hours/google-10000-english.git
    GIT_TAG        master
)

# WORDS

FetchContent_Populate(Google10000Words)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)

file(COPY ${google10000words_SOURCE_DIR}/google-10000-english.txt
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data
)

# simdjson library

FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        master # or a stable release tag like v3.9.2
)

FetchContent_MakeAvailable(simdjson)



# OpenSSL
find_package(OpenSSL REQUIRED)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
