#ifndef TFT_SCREEN_H
#define TFT_SCREEN_H

#include <stdint.h>

// Set to 1 to print per-core frame-rate and timing counters once a second.
// Leave off in normal use: stdio here is a blocking UART write and a ~60
// character line at 115200 baud costs about 5ms, enough to disturb pacing.
#ifndef EMU_PROFILE
#define EMU_PROFILE 0
#endif

#if EMU_PROFILE
extern volatile uint32_t prof_display_frames;
extern volatile uint32_t prof_dropped_frames;
extern volatile uint32_t prof_scale_us;
extern volatile uint32_t prof_wait_us;
#endif

extern volatile uint32_t tft_frames_done;
extern volatile uint8_t tft_pause;

void PPU_ILI9341_Init(void);
void PPU_Line_Complete(uint8_t line);
void Core1_Main(void);

#endif
