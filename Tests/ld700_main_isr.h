#ifndef LD700_MAIN_ISR_H
#define LD700_MAIN_ISR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define ENABLE_CTC_INT() (TIMSK1 |= (1 << OCIE1A))
#define DISABLE_CTC_INT() (TIMSK1 &= ~(1 << OCIE1A))
#define CYCLES_8MS_MIN 7800
#define CYCLES_TIL_8MS_TIMEOUT 8600
#define CYCLES_4MS_MIN 3900
#define CYCLES_TIL_4MS_TIMEOUT 4300
#define CYCLES_TIL_TIMEOUT 2150
#define CYCLES_TIL_ITS_A_1 1500

#define STAGE_WAITING_FOR_8MS 0
#define	STAGE_8MS_STARTED 1
#define	STAGE_4MS_STARTED 2
#define	STAGE_PULSES_STARTED 3

extern uint8_t LD700_EXT_CTRL;
extern uint8_t g_ld700_u8ReceivingStage;
extern uint8_t g_ld700_u8Message;
extern uint8_t g_ld700_u8ReceivedBitCount;
extern volatile uint8_t g_ld700_u8FinishedByte;
extern volatile uint8_t g_ld700_u8FinishedByteReady;
extern uint16_t OCR1A;

//////////////////////

void PCINT0_vect();

#ifdef __cplusplus
}
#endif

#endif //LD700_MAIN_ISR_H
