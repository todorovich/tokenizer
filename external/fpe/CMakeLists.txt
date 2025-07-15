include(FetchContent)

find_package(OpenSSL REQUIRED)

FetchContent_Declare(
    ng_fpe
    GIT_REPOSITORY https://github.com/0NG/Format-Preserving-Encryption.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(ng_fpe)

message(STATUS "${ng_fpe_SOURCE_DIR}/src")

add_library(fpe_lib STATIC
    ${ng_fpe_SOURCE_DIR}/src/ff1.c
    ${ng_fpe_SOURCE_DIR}/src/ff3.c
    ${ng_fpe_SOURCE_DIR}/src/fpe_locl.c
)

target_compile_definitions(fpe_lib PRIVATE OPENSSL_API_COMPAT=0x10100000L)

target_include_directories(fpe_lib PUBLIC
    ${ng_fpe_SOURCE_DIR}/src
)

target_link_libraries(fpe_lib PUBLIC OpenSSL::Crypto)