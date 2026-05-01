add_library(
        server
        include/storage_engine.h
        include/request_handler.h
        include/lexer.h
        include/command.h
        src/storage_engine.cpp
        src/request_handler.cpp
        src/lexer.cpp
        src/command.cpp
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
        nlohmann_json::nlohmann_json
        taocpp::pegtl
)
add_subdirectory(tests)

find_package(pegtl CONFIG REQUIRED)

