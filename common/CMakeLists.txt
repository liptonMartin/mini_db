add_library(
        common
        include/datatypes.h
        src/datatypes.cpp
)

target_include_directories(
        common
        PUBLIC
        ./include
)
add_subdirectory(tests)