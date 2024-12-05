cmake_minimum_required(VERSION 3.15)

project(avr_main)

# define library version
set(AVR_MAIN_VERSION_MAJOR 1 CACHE STRING "major version" FORCE)
set(AVR_MAIN_VERSION_MINOR 0 CACHE STRING "minor version" FORCE)
set(AVR_MAIN_VERSION ${AVR_MAIN_VERSION_MAJOR}.${AVR_MAIN_VERSION_MINOR} CACHE STRING "version" FORCE)

# from https://foonathan.net/2016/03/cmake-install/
#set(include_dest "include/avr_main-${AVR_MAIN_VERSION}")
#set(main_lib_dest "lib/avr_main-${AVR_MAIN_VERSION}")

# for config.h file since it may be different depending on whether we are doing Debug/Release/etc
#set(lib_dest "${main_lib_dest}/${CMAKE_BUILD_TYPE}")

# don't look back
set(CMAKE_CXX_STANDARD 17)

add_subdirectory("src")

configure_file(avr_main-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/avr_main-config-version.cmake @ONLY)

#install(FILES avr_main-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/avr_main-config-version.cmake DESTINATION ${main_lib_dest})
#install(EXPORT avr_main FILE avr_main.cmake DESTINATION "${lib_dest}")
