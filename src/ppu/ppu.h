#ifndef SRC_PPU_PPU_H_
#define SRC_PPU_PPU_H_

#include <stdint.h>

#define LCDC_REG            0xFF40
#define LCDC_ENABLE         0b10000000
#define LCDC_WINDOW_MAP     0b01000000
#define LCDC_WINDOW_EN      0b00100000
#define LCDC_TILE_SEL       0b00010000
#define LCDC_BG_MAP         0b00001000
#define LCDC_OBJ_SIZE       0b00000100
#define LCDC_OBJ_EN         0b00000010
#define LCDC_BG_ENABLE      0b00000001

#define STAT_REG            0xFF41
#define STAT_LYC_INT        0b01000000
#define STAT_OAM_INT        0b00100000
#define STAT_VBLANK_INT     0b00010000
#define STAT_HBLANK_INT     0b00001000
#define STAT_LYC_COMPARE    0b00000100
#define STAT_MODE_MASK      0b00000011

#define IF_REG 0xFF0F

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 144
#define TILE_SIZE     8



typedef enum {
    PPU_H_BLANK = 0,
    PPU_V_BLANK = 1,
    PPU_OAM_SEARCH = 2,
    PPU_PIXEL_TRANSFER = 3,
} PPU_Mode;

typedef struct {
    uint8_t y;
    uint8_t x;
    uint8_t tile;
    uint8_t flags;

} Sprite;

typedef struct {
    uint8_t line_buffer[SCREEN_WIDTH/2];
    PPU_Mode mode;
    uint8_t sprite_count;
    Sprite sprites[10];
    uint16_t cycle_counter;

}PPU;

extern PPU ppu;
void PPU_Init();
void PPU_Step(uint8_t cycles);

void PPU_Render_Scanline();
void PPU_Fetch_Sprite();

void PPU_Render_Sprite_Line();
void PPU_Render_Background_Line();

void PPU_Line_Complete(uint8_t line);
#endif /* SRC_PPU_PPU_H_ */