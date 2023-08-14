cmake_minimum_required(VERSION 3.17)

project(unittest C CXX)

set(CMAKE_CXX_STANDARD 20)


enable_testing()

add_executable(unittest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_pipeline.cpp
)

target_include_directories(unittest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/
)

target_link_libraries(
    unittest
    gtest_main
    gtest
    gmock
)

include(GoogleTest)

gtest_discover_tests(unittest) # discovers tests by asking the compiled test executable to enumerate its tests

