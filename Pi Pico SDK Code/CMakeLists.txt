# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_PATH "C:/Users/marti/PICO/pico-sdk")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(backlighting C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_subdirectory(./pico_neopixels build)
add_executable(backlighting rstp.c backlighting.cpp)

pico_enable_stdio_usb(backlighting 1)
pico_enable_stdio_uart(backlighting 1)

pico_add_extra_outputs(backlighting)

target_link_libraries(backlighting pico_stdlib hardware_irq pico_neopixel hardware_adc pico_multicore)
