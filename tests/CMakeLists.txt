target_sources(encryption_test
    PRIVATE
        test_AES356ECB.cpp
        test_IndexedGlyphSet.cpp
        test_FF1Cipher.cpp
        test_GlyphFPECipher.cpp
        test_Performance.cpp
        test_UnicodeGlyphCipherIndex.cpp
        test_UnicodeFPECipher.cpp
)

target_link_libraries(encryption_test
    PRIVATE
        main_library
        Catch2::Catch2WithMain
)

target_include_directories(encryption_test
    PRIVATE
        ${CMAKE_BINARY_DIR}/_deps/fpe-src/src
)

target_sources(http_server_test
    PRIVATE
        test_WebServer.cpp
)

target_link_libraries(http_server_test
    PRIVATE
        main_library
        Catch2::Catch2WithMain
        CURL::libcurl
)
