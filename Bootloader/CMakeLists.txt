cmake_minimum_required(VERSION 3.15)

# for the bootloader, this check fails due to memory constraints, and we don't need it anyway because we aren't using C++
set(CMAKE_CXX_COMPILER_FORCED TRUE) 

project(bootloader)

# don't look back
set(CMAKE_CXX_STANDARD 17)

add_subdirectory("src")

install(DIRECTORY scripts TYPE BIN)
