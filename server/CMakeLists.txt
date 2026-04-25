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
