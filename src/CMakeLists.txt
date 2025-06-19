# TOKENIZER

target_sources(main_library
  PRIVATE
    AES256ECB.cpp
    Base64.cpp
    FF1Cipher.cpp
    WebServer.cpp
  PUBLIC
    AES256ECB.hpp
    Base64.hpp
    CircularPool.hpp
    Curl.hpp
    FF1Cipher.hpp
    WebServer.hpp
)

message(STATUS "main lib ${CMAKE_BINARY_DIR}/_deps/fpe-src/src")

target_include_directories(main_library
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/_deps/fpe-src/src
    PRIVATE
        ${web_sockets_SOURCE_DIR}/src
        ${web_sockets_SOURCE_DIR}/uSockets/src
        ${fpe_SOURCE_DIR}/src
)

target_link_libraries(main_library
    PUBLIC
        OpenSSL::Crypto
        fpe_lib
        uWebSockets
        uSockets
        fastbase64
    PRIVATE
        #BLAKE3::blake3
        #simdjson
)

# HTTP SERVER

target_sources(http_server
    PUBLIC
        main.cpp
)

target_include_directories(http_server
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${web_sockets_SOURCE_DIR}/src
        ${web_sockets_SOURCE_DIR}/uSockets/src
        ${blake3_SOURCE_DIR}/c
)

target_link_libraries(http_server
    PRIVATE
        uWebSockets
        main_library
        uSockets
)
