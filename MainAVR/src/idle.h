#ifndef IDLE_H
#define IDLE_H

// Call this regularly when you are just waiting for something else (like vsync)
// It services all low priority tasks
void idle_think();

// Call this regularly when you are just waiting for something else while inside a critical section
// It services all low priority tasks except IO
void idle_think_critical_section();

#endif // IDLE_H
