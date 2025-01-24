#ifndef LD700_INTERPRETER_H
#define LD700_INTERPRETER_H

#include <cstdint>

typedef enum
{
	LD700_FALSE = 0,
	LD700_TRUE = 1
} LD700_BOOL;

typedef enum
{
	LD700_ERROR, LD700_SEARCHING, LD700_STOPPED, LD700_PLAYING, LD700_PAUSED, LD700_SPINNING_UP, LD700_TRAY_EJECTED
 }
LD700Status_t;

typedef enum
{
	LD700_ERR_UNKNOWN_CMD_BYTE,
	LD700_ERR_UNSUPPORTED_CMD_BYTE,
	LD700_ERR_TOO_MANY_DIGITS,
	LD700_ERR_UNHANDLED_SITUATION,
	LD700_ERR_CORRUPT_INPUT
} LD700ErrCode_t;

// resets all globals to default state
void ld700i_reset();

// sends control byte (for example, 0xA8)
void ld700i_write(uint8_t u8Cmd, LD700Status_t status);

// call for every vblank (helps us with timing)
void ld700i_on_vblank(LD700Status_t status);

#endif //LD700_INTERPRETER_H
