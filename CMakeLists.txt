cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

set(VCPKG_MANIFEST_MODE ON)

project(sys_prog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)

add_subdirectory(common)
add_subdirectory(server)