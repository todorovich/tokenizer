
FetchContent_Declare(
    xxhash
    GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
    GIT_TAG v0.8.1  # or latest stable tag
)

FetchContent_MakeAvailable(xxhash)

add_library(xxhash_lib
    STATIC
        ${xxhash_SOURCE_DIR}/xxh_x86dispatch.c
        ${xxhash_SOURCE_DIR}/xxhash.c
)

target_include_directories(xxhash_lib PUBLIC ${xxhash_SOURCE_DIR}/lib)
