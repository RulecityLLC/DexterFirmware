cmake_minimum_required(VERSION 3.15)

project(avr_main)

# define library version
set(AVR_MAIN_VERSION_MAJOR 1 CACHE STRING "major version" FORCE)
set(AVR_MAIN_VERSION_MINOR 0 CACHE STRING "minor version" FORCE)
set(AVR_MAIN_VERSION ${AVR_MAIN_VERSION_MAJOR}.${AVR_MAIN_VERSION_MINOR} CACHE STRING "version" FORCE)

# don't look back
set(CMAKE_CXX_STANDARD 17)

add_subdirectory("src")

configure_file(avr_main-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/avr_main-config-version.cmake @ONLY)
