# TOKENIZER

target_sources(fpe_cpp
  PRIVATE
    AES256ECB.cpp
    Base64.cpp
    FF1Cipher.cpp
    UnicodeFPECipher.cpp
    PreconfiguredIndexedGlyphSet.cpp
    WebServer.cpp
  PUBLIC
    AES256ECB.hpp
    Base64.hpp
    CircularPool.hpp
    Curl.hpp
    IndexedGlyphSet.hpp
    FF1Cipher.hpp
    GlyphFPECipher.hpp
    PreconfiguredIndexedGlyphSet.hpp
    UnicodeGlyphCipherIndex.hpp
    WebServer.hpp
)

message(STATUS "main lib ${CMAKE_BINARY_DIR}/_deps/fpe_cpp-src/src")

target_include_directories(fpe_cpp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/_deps/fpe_cpp-src/src
    PRIVATE
        ${web_sockets_SOURCE_DIR}/src
        ${web_sockets_SOURCE_DIR}/uSockets/src
        ${fpe_SOURCE_DIR}/src
)

target_link_libraries(fpe_cpp
    PUBLIC
        bbhash_lib
        fastbase64
        fpe_lib
        OpenSSL::Crypto
        uWebSockets
        uSockets
    PRIVATE
        #BLAKE3::blake3
        #simdjson
)

target_compile_definitions(fpe_cpp PRIVATE OPENSSL_API_COMPAT=0x10100000L)

# SHARED LIB

target_sources(fpe
    PRIVATE
        libfpe.cpp
    PUBLIC
        libfpe.hpp
)

target_link_libraries(fpe PRIVATE fpe_cpp)

# SHARED LIB JNI

target_sources(fpe-jni
    PRIVATE
        libfpe-jni.cpp
    PUBLIC
        libfpe-jni.hpp
)

target_include_directories(fpe-jni PRIVATE ${JNI_INCLUDE_DIRS})

target_link_libraries(fpe-jni
    PRIVATE
        fpe
        fpe_cpp
        ${JNI_LIBRARIES}
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
        fpe_cpp
        uSockets
)
