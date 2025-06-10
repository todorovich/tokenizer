add_executable(test_tokenizer)

target_sources(test_tokenizer
    PRIVATE
        test_tokenizer.cpp
)

target_link_libraries(test_tokenizer
    PRIVATE
        tokenizer
        Catch2::Catch2WithMain
)

add_executable(performance_test)

target_sources(performance_test
    PRIVATE
        test_tokenizer_performance.cpp
)

target_link_libraries(performance_test
    PRIVATE
        tokenizer
        Catch2::Catch2WithMain
)