cmake_minimum_required(VERSION 3.15)

project(avr_aux)

# define library version
set(AVR_AUX_VERSION_MAJOR 1 CACHE STRING "major version" FORCE)
set(AVR_AUX_VERSION_MINOR 0 CACHE STRING "minor version" FORCE)
set(AVR_AUX_VERSION ${AVR_AUX_VERSION_MAJOR}.${AVR_AUX_VERSION_MINOR} CACHE STRING "version" FORCE)

# don't look back
set(CMAKE_CXX_STANDARD 17)

add_subdirectory("src")

configure_file(avr_aux-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/avr_aux-config-version.cmake @ONLY)
