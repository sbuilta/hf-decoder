cmake_minimum_required(VERSION 3.13)
project(hfdecoder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RTLSDR REQUIRED librtlsdr)

add_executable(hfdecoder
    src/main.cpp
    src/rf_input.cpp
)

target_include_directories(hfdecoder PRIVATE include ${RTLSDR_INCLUDE_DIRS})
target_link_libraries(hfdecoder PRIVATE ${RTLSDR_LIBRARIES})
