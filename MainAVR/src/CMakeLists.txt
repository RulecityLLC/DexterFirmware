# so that CMake will build assembly language files
enable_language(C ASM)

# source files to be built
set(AVR_MAIN_SRCS
	autodetect.c
	autodetect.h
	buttons.c
	buttons.h
	crc.c
	crc.h
	common-ldp.c
	common-ldp.h
	deferred_eeprom_write.c
	deferred_eeprom_write.h
	idle.c
	idle.h
	ldp_stub.c
	ldp_stub.h
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
	strings.c
	strings.h
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
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

# convert ELF file into binary format that Dexter's bootloader uses
add_custom_command(
    TARGET avr_main
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .lock -R .signature ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
	
	# vldp-avr.bin is the name of the file that Dexter's media server expects
	COMMAND ${CMAKE_OBJCOPY} -I ihex ${PROJECT_NAME}.hex -O binary vldp-avr.bin
)

find_package(ldp_in 1.0 REQUIRED)
target_link_libraries(avr_main PUBLIC ldp_in)

find_package(ldp_abst 1.0 REQUIRED)
target_link_libraries(avr_main PUBLIC ldp_abst)

message("cmake src dir is ${CMAKE_SOURCE_DIR}")
message("cmake runtime out dir is ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message("cmake binary dir is ${CMAKE_BINARY_DIR}")

#install(FILES ${AVR_MAIN_PUBLIC_INCLUDE} DESTINATION "${include_dest}/avr-main")

# goes into lib_dest so that Debug/Release is taken into account
#install(TARGETS avr_main EXPORT avr_main DESTINATION "${lib_dest}")