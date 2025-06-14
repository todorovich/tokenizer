target_sources(encryption_test
    PRIVATE
        test_aes356ecb.cpp
)

target_link_libraries(encryption_test
    PRIVATE
        main_library
        Catch2::Catch2WithMain
)

target_sources(http_server_test
    PRIVATE
        test_http_server.cpp
)

target_include_directories(http_server_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${web_sockets_SOURCE_DIR}/src
        ${web_sockets_SOURCE_DIR}/uSockets/src
)

target_link_libraries(http_server_test
    PRIVATE
        main_library
        Catch2::Catch2WithMain
        CURL::libcurl
)
