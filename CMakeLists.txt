cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

set(VCPKG_MANIFEST_MODE ON)

project(MiniDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)

enable_testing()

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)

FetchContent_MakeAvailable(googletest)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)