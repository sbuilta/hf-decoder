cmake_minimum_required(VERSION 3.13)
project(hfdecoder LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_MAIN "Build main hfdecoder application" ON)

if(BUILD_MAIN)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(RTLSDR REQUIRED librtlsdr)
  pkg_check_modules(FFTW3 REQUIRED fftw3f)
  pkg_check_modules(SQLITE3 REQUIRED sqlite3)
  pkg_check_modules(CIVETWEBCPP REQUIRED civetweb-cpp)
  pkg_check_modules(CIVETWEB REQUIRED civetweb)
  find_package(Threads REQUIRED)

  add_executable(hfdecoder
      src/main.cpp
      src/rf_input.cpp
      src/dsp/sync.cpp
      src/dsp/demod.cpp
      src/dsp/decode.cpp
      src/dsp/engine.cpp
      src/data_store.cpp
      src/web_server.cpp
      src/ft8/constants.c
      src/ft8/crc.c
      src/ft8/ldpc.c
      src/logging.cpp
      src/config.cpp
  )

  target_include_directories(hfdecoder PRIVATE include ${RTLSDR_INCLUDE_DIRS} ${FFTW3_INCLUDE_DIRS} ${SQLITE3_INCLUDE_DIRS} ${CIVETWEBCPP_INCLUDE_DIRS} ${CIVETWEB_INCLUDE_DIRS})
  target_link_libraries(hfdecoder PRIVATE ${RTLSDR_LIBRARIES} ${FFTW3_LIBRARIES} ${SQLITE3_LIBRARIES} ${CIVETWEBCPP_LIBRARIES} ${CIVETWEB_LIBRARIES})
  target_link_libraries(hfdecoder PRIVATE Threads::Threads)
endif()

enable_testing()
add_subdirectory(tests)
