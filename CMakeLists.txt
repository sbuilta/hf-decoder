cmake_minimum_required(VERSION 3.13)
project(hfdecoder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hfdecoder src/main.cpp)

target_include_directories(hfdecoder PRIVATE include)
