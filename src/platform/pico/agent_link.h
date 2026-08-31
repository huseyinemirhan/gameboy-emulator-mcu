#ifndef AGENT_LINK_H
#define AGENT_LINK_H

#include <stdint.h>

// Serial control link: lets an external driver inject joypad state and read
// guest memory over stdio, so the emulator can be driven programmatically.
//
// Entirely optional and entirely additive. Set AGENT_LINK to 0 (here, or with
// -DAGENT_LINK=0) and every entry point below collapses to a no-op: nothing is
// compiled, nothing is linked, and the emulator behaves exactly as it does
// without this file. Even with it compiled in, the link stays DETACHED until
// something sends the attach command, and while detached it never touches
// emulator state -- the physical buttons and joystick are the only input.
#ifndef AGENT_LINK
#define AGENT_LINK 1
#endif

#if AGENT_LINK

void AgentLink_Init(void);

// Call once per emulated frame, AFTER get_input() has sampled the hardware.
// Services pending commands and, only while attached, overrides the joypad
// state that get_input() just wrote.
void AgentLink_Frame(void);

#else

#define AgentLink_Init()  ((void)0)
#define AgentLink_Frame() ((void)0)

#endif

#endif
