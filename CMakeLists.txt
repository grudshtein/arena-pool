cmake_minimum_required(VERSION 3.20)
project(arena-pool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Warning policy ---
function(enable_strict_warnings TARGET)
    if (MSVC)
        target_compile_options(${TARGET} PRIVATE 
            /W4 /WX
        )
    else()
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wconversion -Wsign-conversion -Wshadow
            -Werror
        )
    endif()
endfunction()

# --- GoogleTest ---
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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
enable_strict_warnings(test_arena_pool)
target_link_libraries(test_arena_pool PRIVATE
    GTest::gtest_main
)

# --- CTest ---
enable_testing()
add_test(NAME arena_pool_tests COMMAND test_arena_pool)

# --- Bench executable
add_executable(bench_arena_pool
    bench/main.cpp
    bench/bench_arena_pool.cpp
    bench/bench_arena_pool.hpp
)
target_include_directories(bench_arena_pool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(bench_arena_pool
    PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
enable_strict_warnings(bench_arena_pool)