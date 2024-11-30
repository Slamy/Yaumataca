cmake_minimum_required(VERSION 3.17)

project(unittest C CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")

enable_testing()

# Code coverage -----

set(covname cov.info)
add_compile_options(-fprofile-arcs -ftest-coverage)
add_link_options(--coverage)
add_custom_target(cov
    COMMAND ${CMAKE_BINARY_DIR}/unittest
    COMMAND lcov -c -o ${covname} -d . -b .
    COMMAND lcov -r ${covname} '*/test/*' '/usr/include/*' -o ${covname}
    COMMAND genhtml ${covname} -output coverage
    DEPENDS unittest
)
set_directory_properties(PROPERTIES
    ADDITIONAL_CLEAN_FILES ${covname}
)

# ---------------------


add_executable(unittest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_field_extract.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_mouse_c1351.cpp
)

target_include_directories(unittest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/processors/
)

target_link_libraries(
    unittest
    gtest_main
    gtest
    gmock
)

include(GoogleTest)

gtest_discover_tests(unittest) # discovers tests by asking the compiled test executable to enumerate its tests


