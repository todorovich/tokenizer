# TOKENIZER

target_sources(main_library
  PRIVATE
    Base64.cpp
    AES256ECB.cpp
    WebServer.cpp
  PUBLIC
    Base64.hpp
    AES256ECB.hpp
    CircularPool.hpp
    Curl.hpp
    WebServer.hpp
)

target_include_directories(main_library
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${web_sockets_SOURCE_DIR}/src
        ${web_sockets_SOURCE_DIR}/uSockets/src
        ${blake3_SOURCE_DIR}/c
)

target_link_libraries(main_library
    PUBLIC
        OpenSSL::Crypto
    PRIVATE
        BLAKE3::blake3
        simdjson
        fastbase64
        uWebSockets
        uSockets
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
