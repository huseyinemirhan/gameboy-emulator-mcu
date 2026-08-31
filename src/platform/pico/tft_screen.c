#include <stdint.h>
#include <string.h>

#include "tft_screen.h"
#include "ili9341.h"
#include "../../ppu/ppu.h"

#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/sync.h"

extern void ILI9341_WaitDMA(void);

#define FRAME_LINE_BYTES 80
#define FRAME_HEIGHT 144
#define FRAME_WIDTH 160

// 4-byte aligned so the 2x scaler can write doubled pixels as 32-bit stores.
// A uint16_t array is only guaranteed 2-byte alignment on its own; every row
// offset is a multiple of ILI9341_WIDTH*2, which preserves this.
static uint16_t display_buffer[ILI9341_WIDTH * ILI9341_HEIGHT] __attribute__((aligned(4)));

static uint8_t frame_buffers[2][FRAME_LINE_BYTES * FRAME_HEIGHT];
static uint8_t frame_write_index = 0;

// Set by core 0 when it hands a frame over, cleared by core 1 once it has
// finished reading that frame out. Core 0 never waits on this: if core 1 is
// still busy the frame is dropped, so the emulated CPU keeps running at full
// speed no matter how slowly the panel can be fed.
static volatile uint8_t core1_busy = 0;

// Always compiled (unlike the EMU_PROFILE counters): the only way to tell
// from outside whether core 1 is rendering and starting transfers at all.
volatile uint32_t tft_frames_done = 0;

// Lets a diagnostic take the SPI bus without racing core 1's transfers.
volatile uint8_t tft_pause = 0;

// Colours are pre-byte-swapped: the DMA channel is configured for DMA_SIZE_8
// and pushes bytes in memory order, while the ILI9341 expects big-endian
// RGB565. Storing them swapped keeps the scaler free of per-pixel bswaps.
static const uint16_t GB_COLORS[4] = {
    __builtin_bswap16(0xFFFF),
    __builtin_bswap16(0xAD55),
    __builtin_bswap16(0x52AA),
    __builtin_bswap16(0x0000),
};

// Same four colours, each duplicated into both halves of a word, so the 2x
// horizontal case can emit a doubled pixel with one 32-bit store.
static uint32_t GB_COLORS32[4];

#if ILI9341_WIDTH != FRAME_WIDTH * 2
// General (non-integer) horizontal scale: destination span of each source
// pixel, computed once at init instead of once per source pixel per row.
static uint16_t dst_x_start[FRAME_WIDTH + 1];
#endif

#if EMU_PROFILE
volatile uint32_t prof_display_frames = 0;
volatile uint32_t prof_dropped_frames = 0;
volatile uint32_t prof_scale_us = 0;
volatile uint32_t prof_wait_us = 0;
#endif

static void ScaleTablesInit(void) {
    for (int i = 0; i < 4; i++) {
        GB_COLORS32[i] = (uint32_t)GB_COLORS[i] | ((uint32_t)GB_COLORS[i] << 16);
    }

#if ILI9341_WIDTH != FRAME_WIDTH * 2
    for (int i = 0; i <= FRAME_WIDTH; i++) {
        dst_x_start[i] = (uint16_t)((i * ILI9341_WIDTH) / FRAME_WIDTH);
    }
#endif
}

// Scales one packed-4bpp source line into a destination row.
static inline void ScaleLine(const uint8_t* src_line, uint16_t* dst_line) {
#if ILI9341_WIDTH == FRAME_WIDTH * 2
    // Exact 2x: consume one source byte (two pixels) per iteration and emit
    // four destination pixels as two 32-bit stores. Every destination row
    // starts on a 4-byte boundary because ILI9341_WIDTH is even.
    uint32_t* dst = (uint32_t*)dst_line;
    for (int i = 0; i < FRAME_LINE_BYTES; i++) {
        const uint8_t packed = src_line[i];
        dst[0] = GB_COLORS32[(packed >> 4) & 0x03];
        dst[1] = GB_COLORS32[packed & 0x03];
        dst += 2;
    }
#else
    for (int src_x = 0; src_x < FRAME_WIDTH; src_x++) {
        const int shift = (src_x & 1) ? 0 : 4;
        const uint16_t color = GB_COLORS[(src_line[src_x >> 1] >> shift) & 0x03];
        for (int dst_x = dst_x_start[src_x]; dst_x < dst_x_start[src_x + 1]; dst_x++) {
            dst_line[dst_x] = color;
        }
    }
#endif
}

static void RenderFrameToDisplayBuffer(const uint8_t* frame) {
    // No clear pass: the loop below covers every destination row and column
    // exactly once, so anything memset here would be overwritten anyway.
    for (int src_y = 0; src_y < FRAME_HEIGHT; src_y++) {
        const uint8_t* src_line = &frame[src_y * FRAME_LINE_BYTES];
        const int dst_y_start = (src_y * ILI9341_HEIGHT) / FRAME_HEIGHT;
        const int dst_y_end = ((src_y + 1) * ILI9341_HEIGHT) / FRAME_HEIGHT;

        if (dst_y_start >= dst_y_end) continue;

        // Scale the line once...
        uint16_t* dst_line = &display_buffer[dst_y_start * ILI9341_WIDTH];
        ScaleLine(src_line, dst_line);

        // ...then copy it into whatever further rows it covers.
        for (int dst_y = dst_y_start + 1; dst_y < dst_y_end; dst_y++) {
            memcpy(&display_buffer[dst_y * ILI9341_WIDTH], dst_line, ILI9341_WIDTH * 2);
        }
    }
}

void PPU_ILI9341_Init(){
    ScaleTablesInit();
    ILI9341_Init();
    ILI9341_FillScreen(0x0000);
}

// RUNS ON CORE 0
void PPU_Line_Complete(uint8_t line) {
    if (line >= FRAME_HEIGHT) return;

    memcpy(&frame_buffers[frame_write_index][line * FRAME_LINE_BYTES], ppu.line_buffer, FRAME_LINE_BYTES);

    if (line == FRAME_HEIGHT - 1) {
        // Non-blocking handoff. The busy flag guarantees at most one frame is
        // ever in flight, so the push below cannot block and the two frame
        // buffers are enough: core 1 always reads the buffer core 0 just
        // stopped writing.
        if (!core1_busy) {
            core1_busy = 1;
            __dmb();
            multicore_fifo_push_blocking(frame_write_index);
            frame_write_index ^= 1u;
        }
#if EMU_PROFILE
        else {
            prof_dropped_frames++;
        }
#endif
    }
}

// RUNS ON CORE 1
void Core1_Main(void) {
    while (1) {
        const uint8_t frame_index = (uint8_t)multicore_fifo_pop_blocking();
        const uint8_t* frame = frame_buffers[frame_index & 1u];

#if EMU_PROFILE
        const uint64_t t_wait = time_us_64();
#endif
        // The previous transfer reads out of display_buffer, so it has to
        // finish before the next frame is scaled into it.
        ILI9341_WaitDMA();

#if EMU_PROFILE
        const uint64_t t_scale = time_us_64();
        prof_wait_us = (uint32_t)(t_scale - t_wait);
#endif

        RenderFrameToDisplayBuffer(frame);

#if EMU_PROFILE
        prof_scale_us = (uint32_t)(time_us_64() - t_scale);
        prof_display_frames++;
#endif

        // The source buffer has been consumed, so core 0 is free to reuse it.
        // The transfer below only touches display_buffer.
        __dmb();
        core1_busy = 0;

        while (tft_pause) {
            tight_loop_contents();
        }
        ILI9341_StartFrameDMA(display_buffer, ILI9341_WIDTH * ILI9341_HEIGHT * 2);
        tft_frames_done++;
    }
}
