add_library(tokenizer)

target_sources(tokenizer
  PRIVATE
    tokenizer.cpp
  PUBLIC
    tokenizer.hpp
)

target_include_directories(tokenizer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${blake3_SOURCE_DIR}/c
)

target_link_libraries(tokenizer
    PUBLIC
        OpenSSL::Crypto
    PRIVATE
        BLAKE3::blake3
        simdjson
)
