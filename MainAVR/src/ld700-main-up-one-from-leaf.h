#ifndef LD700_MAIN_UP_ONE_FROM_LEAF_H
#define LD700_MAIN_UP_ONE_FROM_LEAF_H

#include "ld700-callbacks.h"	// for LD700Status_t definition

void ld700_up_one_from_leaf_reset();	// resets all state, for testing
void ld700_button_think(LD700Status_t status);
void ld700_on_vblank(LD700Status_t status);

#endif // LD700_MAIN_UP_ONE_FROM_LEAF_H
