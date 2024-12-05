#ifndef DEFERRED_EEPROM_WRITE_H
#define DEFERRED_EEPROM_WRITE_H

// [re]starts the timer for a deferred eeprom write to take place.
// Purpose: prolong Dexter EEPROM life by limiting EEPROM writes.
void deferred_eeprom_write_restart();

// checks to see if it's time to do the deferred eeprom write and does it if so
void deferred_eeprom_write_think();

#endif // DEFERRED_EEPROM_WRITE_H
