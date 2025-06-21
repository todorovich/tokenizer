FetchContent_Declare(
    bbhash
    GIT_REPOSITORY https://github.com/rizkg/BBHash.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(bbhash)

add_library(bbhash_lib INTERFACE)

target_include_directories(bbhash_lib
    INTERFACE
        ${bbhash_SOURCE_DIR}
)
