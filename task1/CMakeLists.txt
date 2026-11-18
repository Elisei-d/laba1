cmake_minimum_required(VERSION 3.14)
project(Lab1_Task1_FileSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(filesystem
    src/main.cpp
    src/Name.cpp
    src/Path.cpp
    src/FsNode.cpp
    src/File.cpp
    src/Directory.cpp
    src/FileSystem.cpp
    tests/FileSystemTests.cpp
)
target_include_directories(filesystem PRIVATE include tests)

if(MSVC)
    target_compile_options(filesystem PRIVATE /W4 /utf-8)
else()
    target_compile_options(filesystem PRIVATE -Wall -Wextra -pedantic)
endif()
