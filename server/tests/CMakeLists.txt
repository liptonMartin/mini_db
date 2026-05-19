add_executable(
        test_lexer
        test_lexer.cpp
)
add_executable(
        test_storage_engine
        test_storage_engine.cpp
)
add_executable(
        test_command
        test_command.cpp
)
target_link_libraries(
        test_lexer
        PRIVATE
        server
        GTest::gtest_main
)
<<<<<<< HEAD

target_link_libraries(
        test_command
        PRIVATE
        server
        GTest::gtest_main
)
=======
>>>>>>> 13417979050204977d3242ad04254aad9de99237

add_executable(
        test_parser
        test_parser.cpp
)
target_link_libraries(
        test_parser
        PRIVATE
        server
        GTest::gtest_main
)
<<<<<<< HEAD

target_link_libraries(
        test_storage_engine
        PRIVATE
        server
        GTest::gtest_main
)
=======
>>>>>>> 13417979050204977d3242ad04254aad9de99237
