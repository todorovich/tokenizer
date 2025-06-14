FetchContent_Declare(
    fastbase64
    GIT_REPOSITORY https://github.com/lemire/fastbase64.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(fastbase64)

add_library(fastbase64
    STATIC
        ${fastbase64_SOURCE_DIR}/src/fastavxbase64.c
        ${fastbase64_SOURCE_DIR}/src/chromiumbase64.c
)

target_include_directories(fastbase64
    PUBLIC
        ${fastbase64_SOURCE_DIR}/include
)

target_compile_definitions(fastbase64 PRIVATE HAVE_AVX2)
target_compile_options(fastbase64 PRIVATE -march=native -mavx2)