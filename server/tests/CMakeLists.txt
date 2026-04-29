enable_testing()

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)

FetchContent_MakeAvailable(googletest)


add_executable(
        test_main_server
        test_main_server.cpp

)

add_executable(test_lexer
        test_lexer.cpp
)

target_link_libraries(
        test_lexer
        PRIVATE
        main_server
        GTest::gtest_main
)

target_link_libraries(
        test_main_server
        PRIVATE
        main_server
        GTest::gtest_main
)

target_include_directories(
        test_main_server
        PRIVATE
        ../include
)

include(GoogleTest)
gtest_discover_tests(test_main_server)