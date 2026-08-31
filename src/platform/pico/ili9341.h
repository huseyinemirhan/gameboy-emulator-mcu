#ifndef __ILI9341_H__
#define __ILI9341_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// Pin definitions
// SPI clock. The PL022 divides clk_peri (= clk_sys = 266MHz) by
// CPSDVSR * (1 + SCR) with CPSDVSR even, and picks the largest rate <= this
// request -- so asking for a value it cannot hit silently rounds DOWN.
// Exactly achievable rates, fastest first:
//     66500000  266/4   18.5ms/frame  (~54fps)  aggressive, needs short wiring
//     53200000  266/5   23.1ms/frame  (~43fps)
//     44300000  266/6   27.7ms/frame  (~36fps)
//     33250000  266/8   36.9ms/frame  (~27fps)  conservative, known good
// If the panel shows a white screen, garbage, or tearing, step DOWN: at too
// high a clock the init sequence itself is corrupted and the panel never
// leaves its reset state. Frame rate here only affects display smoothness --
// the emulated CPU is paced independently and stays at full speed either way.
#define ILI9341_SPI_BAUD    33250000

#define ILI9341_SPI_PORT    spi0
#define ILI9341_PIN_SCK     18
#define ILI9341_PIN_MOSI    19
#define ILI9341_PIN_MISO    16
#define ILI9341_PIN_CS      17
#define ILI9341_PIN_DC      20
#define ILI9341_PIN_RST     21

#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_BGR 0x08

// Panel selection.
//   TFT_PANEL_ILI9341_320X240 - 2.8" 320x240 SPI module, driven over SPI + DMA.
//   TFT_PANEL_NX4832T035_011  - 480x320 Nextion smart display, driven over UART.
//                               This panel takes drawing commands, not a pixel
//                               stream, so every SPI/DMA entry point is a no-op.
#define TFT_PANEL_ILI9341_320X240 1
#define TFT_PANEL_NX4832T035_011  2

#ifndef TFT_PANEL
#define TFT_PANEL TFT_PANEL_ILI9341_320X240
#endif

#if TFT_PANEL == TFT_PANEL_ILI9341_320X240
#define ILI9341_WIDTH    320
#define ILI9341_HEIGHT   240
#define ILI9341_ROTATION (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)
#elif TFT_PANEL == TFT_PANEL_NX4832T035_011
#define ILI9341_WIDTH    480
#define ILI9341_HEIGHT   320
#define ILI9341_ROTATION (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)
#else
#error "TFT_PANEL must be set to one of the TFT_PANEL_* values above"
#endif

#define ILI9341_BLACK   0x0000
#define ILI9341_WHITE   0xFFFF
#define ILI9341_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

extern int dma_channel;   // claimed in ILI9341_DMA_Init

void ILI9341_Init(void);
void ILI9341_Select();
void ILI9341_Unselect(void);
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ILI9341_StartDMATransfer(const void* framebuffer, size_t num_bytes);
void ILI9341_DrawScanline(uint16_t x, uint16_t y, uint16_t* data, uint16_t len);
void ILI9341_DrawScanlineRaw(uint16_t y, uint16_t* data, uint16_t len);
void ILI9341_StartFrameDMA(const void* framebuffer, size_t num_bytes);
void ILI9341_WaitDMA(void);
void ILI9341_FillScreen(uint16_t color);

#endif