add_library(
        server
        include/storage_engine.h
        include/lexer.h
        include/command.h
        include/entrypoint.h
        src/storage_engine.cpp
        src/lexer.cpp
        src/command.cpp
        src/entrypoint.cpp
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
add_executable(
        main_server main.cpp
)
add_executable(
        main_storage_node main_storage_node.cpp
)
target_link_libraries(
        main_server
        PUBLIC
        server
)
target_link_libraries(
        main_storage_node
        PUBLIC
        server
)
add_subdirectory(tests)

find_package(pegtl CONFIG REQUIRED)

