
# source files to be built
set(BOOTLOADER_SRCS
	bootloader.c
	bootloader.h
	crc.c
	crc.h
	platform.h
	platform_aux.h
	platform_main.h
	program.c
	program.h
	protocol.c
	protocol.h
	serial.c
	serial.h
	strings.c
	strings.h
)

add_executable(bootloader ${BOOTLOADER_SRCS} )

# The same code is used to build a bootloader for main and aux AVRs, so we need to differentiate the output filename.
# The suffixes were chosen to make it clearer which .bin firmware files match up with the bootloader .hex files.
if (BUILD_AUX)
    set(PROJECT_NAME_SUFFIX "vbi-inject")
else()
	set(PROJECT_NAME_SUFFIX "vldp-avr")
endif()
set(PROJECT_NAME_FULL "${PROJECT_NAME}-${PROJECT_NAME_SUFFIX}")

# ending in .elf helps emphasize what is different about this output file from the .hex and .bin files that we generate later
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME_FULL}.elf")

# convert ELF file into .hex format that AVR programming tools like AVRDude use
add_custom_command(
    TARGET bootloader
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .lock -R .signature ${PROJECT_NAME_FULL}.elf ${PROJECT_NAME_FULL}.hex
)

# to help debug this cmake file.  no harm leaving it here.
message("cmake src dir is ${CMAKE_SOURCE_DIR}")
message("cmake runtime out dir is ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message("cmake binary dir is ${CMAKE_BINARY_DIR}")
message("cmake current binary dir is ${CMAKE_CURRENT_BINARY_DIR}")

# we want the final binary to be installed in the default binary folder, because that's where a user would expect to find it
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_FULL}.hex TYPE BIN)
