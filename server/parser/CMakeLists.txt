add_library(
        parser
        include/parser.h
        src/parser.cpp
)

target_include_directories(
        parser
        PUBLIC
        include
)
target_link_libraries(
        parser
        PUBLIC
)

#add_subdirectory(tests)