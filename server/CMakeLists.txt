add_library(
        main_server
        include/server.h
        src/server.cpp
        include/lexer.h
        src/lexer.cpp
)

target_include_directories(
        main_server
        PUBLIC
        ./include
)
target_link_libraries(
        main_server
        PUBLIC
        common
        nlohmann_json::nlohmann_json
        taocpp::pegtl
        gtest
)

add_subdirectory(tests)

find_package(pegtl CONFIG REQUIRED)

