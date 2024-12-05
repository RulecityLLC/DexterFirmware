# so that CMake will build assembly language files
enable_language(C ASM)

# source files to be built
set(AVR_AUX_SRCS
	asm-macros.h
	build.h
	crc.c
	crc.h
	csync_isr.s
	datatypes.h
	protocol.c
	protocol.h
	serial.c
	serial.h
	serial_packet.c
	serial_packet.h
	strings.c
	strings.h
	util.s
	v1.h
	v2.h
	v3.h
	vbi_inject.c
	vbi_inject.h
)

# add -x assembler-with-cpp when building assembly language files
SET(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")

add_executable(avr_aux ${AVR_AUX_SRCS} )

# ending in .elf helps emphasize what is different about this output file from the .hex and .bin files that we generate later
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

# convert ELF file into binary format that Dexter's bootloader uses
add_custom_command(
    TARGET avr_aux
    POST_BUILD
	COMMAND ${CMAKE_STRIP} --strip-unneeded ${PROJECT_NAME}.elf
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .lock -R .signature ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
	COMMAND ${CMAKE_OBJCOPY} -I ihex ${PROJECT_NAME}.hex -O binary vbi-inject.bin
)

# to help debug this cmake file.  no harm leaving it here.
message("cmake src dir is ${CMAKE_SOURCE_DIR}")
message("cmake runtime out dir is ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message("cmake binary dir is ${CMAKE_BINARY_DIR}")
message("cmake current binary dir is ${CMAKE_CURRENT_BINARY_DIR}")

# we want the final binary to be installed in the default binary folder, because that's where a user would expect to find it
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vbi-inject.bin TYPE BIN)
