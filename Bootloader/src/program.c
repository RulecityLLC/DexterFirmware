#include "program.h"
#include <avr/boot.h>
#include <avr/interrupt.h>
#include "platform.h"

uint8_t g_u8ProgrammingActive = 0;

void program_page(uint32_t u32Address, uint8_t *p8Buf)
{
        uint16_t i;

        // Disable interrupts.
        cli();
    
        eeprom_busy_wait();

        boot_page_erase(u32Address);
        boot_spm_busy_wait();      // Wait until the memory is erased.

        for (i=0; i<PAGE_SIZE_BYTES; i+=2)
        {
            // Set up little-endian word.

            uint16_t w = *p8Buf++;
            w += (*p8Buf++) << 8;
        
            boot_page_fill (u32Address + i, w);
        }

        boot_page_write (u32Address);     // Store buffer in flash page.
        boot_spm_busy_wait();       // Wait until the memory is written.

        // Reenable RWW-section again. We need this if we want to jump back
        // to the application after bootloading.

        boot_rww_enable ();

        // Re-enable interrupts.
       sei();
}
