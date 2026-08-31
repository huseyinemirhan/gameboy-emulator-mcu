#ifndef INPUTS_H
#define INPUTS_H

#define A_BUTTON_MASK       0x01
#define B_BUTTON_MASK       0x02
#define SELECT_BUTTON_MASK  0x04
#define START_BUTTON_MASK   0x08

#define RIGHT_BUTTON_MASK   0x01
#define LEFT_BUTTON_MASK    0x02
#define UP_BUTTON_MASK      0x04
#define DOWN_BUTTON_MASK    0x08

void Input_Init(void);
void get_input(void);

#endif