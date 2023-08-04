cmake_minimum_required(VERSION 3.17)

# RTT is blocking with tinyusb. Will halt software if no debugger is connected!
#set (LOGGER "RTT")
set (FAMILY "rp2040")
set (PROJECT "yaumataca")
set (PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/pico-sdk")

include(${CMAKE_SOURCE_DIR}/tinyusb/hw/bsp/family_support.cmake)

project(amigamouse C CXX ASM)

set(CMAKE_CXX_STANDARD 20)

# Checks this example is valid for the family and initializes the project
family_initialize_project(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

add_executable(${PROJECT})

target_sources(${PROJECT} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/hid_app.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/handlers/hid_impact.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/handlers/hid_mouse.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/handlers/hid_ps3.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  )

target_include_directories(${PROJECT} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Configure compilation flags and libraries for the example without RTOS.
# See the corresponding function in hw/bsp/FAMILY/family.cmake for details.
family_configure_host_example(${PROJECT} noos)

