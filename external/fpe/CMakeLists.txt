include(FetchContent)

find_package(OpenSSL REQUIRED)

FetchContent_Declare(
    fpe
    GIT_REPOSITORY https://github.com/0NG/Format-Preserving-Encryption.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(fpe)

message(STATUS "${fpe_SOURCE_DIR}/src")

add_library(fpe_lib STATIC
        ${fpe_SOURCE_DIR}/src/ff1.c
        ${fpe_SOURCE_DIR}/src/ff3.c
        ${fpe_SOURCE_DIR}/src/fpe_locl.c
)

target_include_directories(fpe_lib PUBLIC
        ${fpe_SOURCE_DIR}/src
)

target_link_libraries(fpe_lib PUBLIC OpenSSL::Crypto)