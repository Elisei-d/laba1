cmake_minimum_required(VERSION 3.14)
project(Lab1_Task2_VersionHistory LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(history
    src/main.cpp
    src/VersionHistory.cpp
    tests/HistoryTests.cpp
)
target_include_directories(history PRIVATE include tests)

if(MSVC)
    target_compile_options(history PRIVATE /W4 /utf-8)
else()
    target_compile_options(history PRIVATE -Wall -Wextra -pedantic)
endif()
