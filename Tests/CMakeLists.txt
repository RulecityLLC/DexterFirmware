cmake_minimum_required(VERSION 3.15)

project(firmware_tests)

# don't look back
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# to help me figure out what these vars are set to (handy!)
message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message("CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_MAIN_FIRMWARE_SRCS
		ld700_firmware_tests.cpp
		ld700_main_isr.cpp
		ld700_main_isr.h
		stdafx.h
		mocks.h
)

add_executable(test_main_firmware ${TEST_MAIN_FIRMWARE_SRCS})

# force C files to be built as C++ files so that they can depend upon our mocks
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/ld700_main_isr.cpp
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/ld700_main_isr.c ${CMAKE_SOURCE_DIR}/ld700_main_isr.cpp
					DEPENDS ${CMAKE_SOURCE_DIR}/ld700_main_isr.c)

# should help improve compile time for test project
target_precompile_headers(test_main_firmware PRIVATE stdafx.h)

# this will automatically give the indicated targets access to the headers/libs of the indicated dependencies
target_link_libraries(test_main_firmware GTest::gtest_main GTest::gmock_main)

# so we can include <avr/io.h> without errors.  If we do 'avr' here, then gtest won't build.
target_include_directories(test_main_firmware PRIVATE .)

include(GoogleTest)
gtest_discover_tests(test_main_firmware)
