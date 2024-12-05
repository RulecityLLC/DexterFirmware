#ifndef PROGRAM_H
#define PROGRAM_H

#include <stdint.h>

void program_page(uint32_t u32Address, uint8_t *p8Buf);
uint8_t is_programming_done();

extern uint8_t g_u8ProgrammingActive;

#define PROGRAM_INIT() g_u8ProgrammingActive = 1
#define PROGRAM_END() g_u8ProgrammingActive = 0
#define IS_PROGRAMMING_ACTIVE() (g_u8ProgrammingActive != 0)

#endif // PROGRAM_H
