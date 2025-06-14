FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        master # or a stable release tag like v3.9.2
)

FetchContent_MakeAvailable(simdjson)