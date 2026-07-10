cmake_minimum_required(VERSION 3.14)
project(resize_image_service CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build unit tests" OFF)

# This service is plain HTTP with no database (CLAUDE.md §7). Note that
# add_subdirectory(libasyik) also builds libasyik's own Catch2 test suite —
# expected upstream behavior, there is no flag to skip it.
SET(LIBASYIK_ENABLE_SOCI OFF CACHE BOOL "" FORCE)
SET(LIBASYIK_ENABLE_SSL_SERVER OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/libasyik)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

# nlohmann/json is header-only; fetched pinned instead of vendored
include(FetchContent)
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(nlohmann_json)

# Core logic (no HTTP) as a library so tests can link it without a server
add_library(resize_core
    src/image_ops.cpp
    src/resize_handler.cpp
)
target_include_directories(resize_core PUBLIC src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(resize_core PUBLIC
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
)

add_executable(resize_image_server src/main.cpp)
# libasyik's public headers include aixlog.hpp and cppcodec/*, but libasyik
# only exposes those include paths to installed consumers
# add them explicitly for the add_subdirectory() case
target_include_directories(resize_image_server PRIVATE
    third_party/libasyik/external/aixlog/include
    third_party/libasyik/external/cppcodec)
target_link_libraries(resize_image_server PRIVATE resize_core libasyik)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
