cmake_minimum_required(VERSION 3.5)

project(mu-parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADER_FILES include/types.h)

set(SOURCES src/main.cpp src/types.cpp ${HEADER_FILES})

add_executable(mu-parser ${SOURCES})

target_include_directories(mu-parser PUBLIC include)
