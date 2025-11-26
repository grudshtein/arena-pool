cmake_minimum_required(VERSION 3.20)
project(arena-pool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- GoogleTest ---
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- Test executable ---
add_executable(test_arena_pool
    test/test_arena_pool.cpp
)
target_include_directories(test_arena_pool PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(test_arena_pool PRIVATE
    GTest::gtest_main
)

# --- CTest ---
enable_testing()
add_test(NAME arena_pool_tests COMMAND test_arena_pool)