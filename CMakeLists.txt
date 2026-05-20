cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

set(VCPKG_MANIFEST_MODE ON)

project(MiniDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

find_package(nlohmann_json REQUIRED)
find_package(pegtl CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS asio uuid process)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

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
