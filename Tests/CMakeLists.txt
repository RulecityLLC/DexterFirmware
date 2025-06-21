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

# -----------------------------------------------------------------------------------------

set(DEPS_SRCS
		autodetect-deps.cpp
		ld700_main_isr.cpp
		ld700_main_isr.h
		ld700_main_deps.cpp
)

add_library(deps ${DEPS_SRCS})

set(TEST_DEPS_SRCS
		autodetect_deps_tests.cpp
		ld700_isr_tests.cpp
		ld700_deps_tests.cpp
		stdafx.h
		mocks.h
		mockImplementations/mockLdpc.cpp
		mockImplementations/mockLdpc.h
		mockImplementations/mockLD700Callbacks.cpp
		mockImplementations/mockLD700Callbacks.h
		mockImplementations/mockDiscSwitch.cpp
		mockImplementations/mockDiscSwitch.h
		mockImplementations/mockSettings.cpp
		mockImplementations/mockSettings.h
		mockImplementations/mockStrings.cpp
		mockImplementations/mockStrings.h
		mockImplementations/mockTimerGlobal.cpp
		mockImplementations/mockTimerGlobal.h
		avr/io.cpp
		mockImplementations/mockCommonLdp.cpp
		mockImplementations/mockCommonLdp.h
		mockImplementations/mockIdle.cpp
		mockImplementations/mockIdle.h
		mockImplementations/mockProtocol.cpp
		mockImplementations/mockProtocol.h
)

add_executable(test_deps ${TEST_DEPS_SRCS})

# force C files to be built as C++ files so that they can depend upon our mocks

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/autodetect-deps.cpp
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../MainAVR/src/autodetect-deps.c ${CMAKE_SOURCE_DIR}/autodetect-deps.cpp
					DEPENDS ${CMAKE_SOURCE_DIR}/../MainAVR/src/autodetect-deps.c)

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/ld700_main_isr.cpp
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/ld700_main_isr.c ${CMAKE_SOURCE_DIR}/ld700_main_isr.cpp
					DEPENDS ${CMAKE_SOURCE_DIR}/ld700_main_isr.c)

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/ld700_main_deps.cpp
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../MainAVR/src/ld700-main-deps.c ${CMAKE_SOURCE_DIR}/ld700_main_deps.cpp
					DEPENDS ${CMAKE_SOURCE_DIR}/../MainAVR/src/ld700-main-deps.c)

# should help improve compile time for test project
target_precompile_headers(test_deps PRIVATE stdafx.h)

# so we can include <avr/io.h> without errors.  If we do 'avr' here, then gtest won't build.
target_include_directories(deps PRIVATE . PRIVATE ../MainAVR/src/)
target_include_directories(test_deps PRIVATE . PRIVATE ../MainAVR/src/)

# this will automatically give the indicated targets access to the headers/libs of the indicated dependencies
target_link_libraries(test_deps deps GTest::gtest_main GTest::gmock_main)

# common.h requires this
target_compile_definitions(deps PRIVATE F_CPU=18432000UL REV3)

# for mutation testing (we don't want to change all CXX flags because we don't want to mutate test runners/frameworks)
if(DEFINED TEST_COMPILE_OPTIONS)
    target_compile_options(deps PRIVATE SHELL:${TEST_COMPILE_OPTIONS})
endif()

# -----------------------------------------------------------------------------------------

set(LIB_ONE_UP_FROM_LEAF_SRCS
		ld700_main_up_one_from_leaf.cpp
)

add_library(up_one_from_leaf ${LIB_ONE_UP_FROM_LEAF_SRCS})

set(TEST_ONE_UP_FROM_LEAF_SRCS
		ld700_main_up_one_from_leaf_tests.cpp
		mocks.h
		mockImplementations/mockLdpc.cpp
		mockImplementations/mockLdpc.h
		mockImplementations/mockLD700Callbacks.cpp
		mockImplementations/mockLD700Callbacks.h
		mockImplementations/mockDiscSwitch.cpp
		mockImplementations/mockDiscSwitch.h
		mockImplementations/mockSettings.cpp
		mockImplementations/mockSettings.h
		mockImplementations/mockStrings.cpp
		mockImplementations/mockStrings.h
		mockImplementations/mockTimerGlobal.cpp
		mockImplementations/mockTimerGlobal.h
		avr/io.cpp
		mockImplementations/mockCommonLdp.cpp
		mockImplementations/mockCommonLdp.h
		mockImplementations/mockLD700Deps.cpp
		mockImplementations/mockLD700Deps.h
)

add_executable(test_up_one_from_leaf ${TEST_ONE_UP_FROM_LEAF_SRCS})

# force C files to be built as C++ files so that they can depend upon our mocks
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/ld700_main_up_one_from_leaf.cpp
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../MainAVR/src/ld700-main-up-one-from-leaf.c ${CMAKE_SOURCE_DIR}/ld700_main_up_one_from_leaf.cpp
					DEPENDS ${CMAKE_SOURCE_DIR}/../MainAVR/src/ld700-main-up-one-from-leaf.c)

# so we can include <avr/io.h> without errors.  If we do 'avr' here, then gtest won't build.
target_include_directories(up_one_from_leaf PRIVATE . PRIVATE ../MainAVR/src/)
target_include_directories(test_up_one_from_leaf PRIVATE . PRIVATE ../MainAVR/src/)

# this will automatically give the indicated targets access to the headers/libs of the indicated dependencies
target_link_libraries(test_up_one_from_leaf up_one_from_leaf GTest::gtest_main GTest::gmock_main)

# common.h requires this
target_compile_definitions(up_one_from_leaf PRIVATE F_CPU=18432000UL REV3)

# for mutation testing (we don't want to change all CXX flags because we don't want to mutate test runners/frameworks)
if(DEFINED TEST_COMPILE_OPTIONS)
	# we don't want to mutate the tests, only the code-under-test
    target_compile_options(up_one_from_leaf PRIVATE SHELL:${TEST_COMPILE_OPTIONS})
endif()

# -----------------------------------------------------------------------------------------

include(GoogleTest)
gtest_discover_tests(test_up_one_from_leaf)
gtest_discover_tests(test_deps)
