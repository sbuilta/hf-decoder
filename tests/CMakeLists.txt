add_executable(decoder_tests
    test_decoder.cpp
    ../src/dsp/decode.cpp
    ../src/ft8/constants.c
    ../src/ft8/crc.c
    ../src/ft8/ldpc.c
)
target_include_directories(decoder_tests PRIVATE ../include)
add_test(NAME decoder_tests COMMAND decoder_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
