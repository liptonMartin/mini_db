add_library(
        server
        include/storage_engine.h
        include/lexer.h
        src/storage_engine.cpp
        src/lexer.cpp
)

target_include_directories(
        server
        PUBLIC
        ./include
        ../common/include
)
target_link_libraries(
        server
        PUBLIC
        common
)
target_link_libraries(
        server
        PUBLIC
        common
        nlohmann_json::nlohmann_json
        taocpp::pegtl
)
add_subdirectory(tests)

find_package(pegtl CONFIG REQUIRED)

