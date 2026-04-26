add_library(
        main_server
        include/server.h
        src/server.cpp
)

target_include_directories(
        main_server
        PUBLIC
        ./include
)
target_link_libraries(
        main_server
        PUBLIC
        nlohmann_json::nlohmann_json
)

add_subdirectory(tests)
add_subdirectory(parser)