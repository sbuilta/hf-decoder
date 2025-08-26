cmake_minimum_required(VERSION 3.13)
project(hfdecoder LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RTLSDR REQUIRED librtlsdr)
pkg_check_modules(FFTW3 REQUIRED fftw3f)

add_executable(hfdecoder
    src/main.cpp
    src/rf_input.cpp
    src/dsp/sync.cpp
    src/dsp/demod.cpp
    src/dsp/decode.cpp
    src/dsp/engine.cpp
    src/ft8/constants.c
    src/ft8/crc.c
    src/ft8/ldpc.c
)

target_include_directories(hfdecoder PRIVATE include ${RTLSDR_INCLUDE_DIRS} ${FFTW3_INCLUDE_DIRS})
target_link_libraries(hfdecoder PRIVATE ${RTLSDR_LIBRARIES} ${FFTW3_LIBRARIES})
