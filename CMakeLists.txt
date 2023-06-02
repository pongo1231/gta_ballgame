cmake_minimum_required(VERSION 3.12)

project(GolfGame CXX)

add_subdirectory(vendor/shv shv)
add_subdirectory(vendor/minhook minhook)

file(GLOB SRC ${PROJECT_SOURCE_DIR}/*.cpp)
add_library(GolfGame MODULE ${SRC})

set_target_properties(GolfGame PROPERTIES CXX_STANDARD 20)
set_target_properties(GolfGame PROPERTIES SUFFIX ".asi")

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(COMPILE_OPTIONS -DUNICODE -D_UNICODE -DNOMINMAX)
else()
    set(COMPILE_OPTIONS -municode -fpermissive)
endif()
target_compile_options(GolfGame PRIVATE ${COMPILE_OPTIONS})
target_include_directories(GolfGame PRIVATE ${PROJECT_SOURCE_DIR} vendor)

target_precompile_headers(GolfGame PRIVATE stdafx.cpp)

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_link_options(GolfGame PRIVATE -static)
endif()
target_link_libraries(GolfGame PRIVATE shv minhook)