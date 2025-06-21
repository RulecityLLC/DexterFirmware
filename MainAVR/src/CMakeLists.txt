# so that CMake will build assembly language files
enable_language(C ASM)

# source files to be built
set(AVR_MAIN_SRCS
	autodetect.c
	autodetect.h
	autodetect-deps.c
	autodetect-deps.h
	buttons.c
	buttons.h
	crc.c
	crc.h
	common-ldp.c
	common-ldp.h
	deferred_eeprom_write.c
	deferred_eeprom_write.h
	disc_switch.c
	disc_switch.h
	idle.c
	idle.h
	ldp_stub.c
	ldp_stub.h
	ld700-callbacks.c
	ld700-callbacks.h
	ld700-main.c
	ld700-main.h
	ld700-main-asm.s
	ld700-main-deps.c
	ld700-main-deps.h
	ld700-main-up-one-from-leaf.c
	ld700-main-up-one-from-leaf.h
	ldp1000_callbacks.c
	ldp1000_callbacks.h
	ldp1000-main.c
	ldp1000-main.h
	ldp1000-main-asm.s
	ldv1000_callbacks.c
	ldv1000_callbacks.h
	ldv1000-main.c
	ldv1000-main.h
	ldv1000-main-asm.s
	ldv1000super-main.c
	led_blinker.c
	led_blinker.h
	led_driver.c
	led_driver.h
	pr7820-callbacks.c
	pr7820-callbacks.h
	pr7820-main.c
	pr7820-main.h
	pr8210-callbacks.c
	pr8210-callbacks.h
	pr8210-main.c
	pr8210-main.h
	pr8210-main-asm.s
	protocol.c
	protocol.h
	serial.c
	serial.h
	serial2.c
	serial2.h
	settings.c
	settings.h
	dexter_strings.c
	dexter_strings.h
	timer1.c
	timer1.h
	timer1_asm.s
	timer-global.c
	timer-global.h
	util.c
	util.h
	vbi_inject.c
	vbi_inject.h
	vldp-avr.c
	vldp-avr.h
	vip9500sg-callbacks.c
	vip9500sg-callbacks.h
	vip9500sg-main.c
	vip9500sg-main.h
	vp931-callbacks.c
	vp931-callbacks.h
	vp931-main.c
	vp931-main.h
	vp931-main-asm.s
	vp932-callbacks.c
	vp932-callbacks.h
	vp932-main.c
	vp932-main.h
	vp932-main-asm.s
	vsync.c
	vsync.h
	vsync_asm.s
)

# add -x assembler-with-cpp when building assembly language files
SET(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")

add_executable(avr_main ${AVR_MAIN_SRCS} )

# ending in .elf helps emphasize what is different about this output file from the .hex and .bin files that we generate later
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "vldp-avr.elf")

# convert ELF file into binary format that Dexter's bootloader uses
add_custom_command(
	TARGET avr_main
	POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .lock -R .signature vldp-avr.elf vldp-avr.hex

	# vldp-avr.bin is the name of the file that Dexter's media server expects
	COMMAND ${CMAKE_OBJCOPY} -I ihex vldp-avr.hex -O binary vldp-avr.bin
)

# pull in dependencies
find_package(ldp_in 1.0 REQUIRED)
target_link_libraries(avr_main PUBLIC ldp_in)

find_package(ldp_abst 1.0 REQUIRED)
target_link_libraries(avr_main PUBLIC ldp_abst)

# to help debug this cmake file.  no harm leaving it here.
message("cmake src dir is ${CMAKE_SOURCE_DIR}")
message("cmake runtime out dir is ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message("cmake binary dir is ${CMAKE_BINARY_DIR}")
message("cmake current binary dir is ${CMAKE_CURRENT_BINARY_DIR}")

# we want the final binary to be installed in the default binary folder, because that's where a user would expect to find it
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vldp-avr.bin TYPE BIN)

# the .hex file may be useful for development so we'll install that too
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vldp-avr.hex TYPE BIN)

# the .elf file may be useful for development so we'll install that too
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vldp-avr.elf TYPE BIN)
