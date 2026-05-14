#include <stdint.h>
#include "ppu.h"

#include <string.h>
#include <stdio.h>

#include "../memory/memory.h"
#include "../cpu/cpu.h"
#include <windows.h>

#include "../platform/windows/inputs.h"


#define COLOR_WHITE   15
#define COLOR_LIGHT   8
#define COLOR_DARK    8
#define COLOR_BLACK   0

// GB color palette (white, light gray, dark gray, black)
static const uint32_t GB_COLORS[4] = {
    0xFFFFFFFF,  // 0 - white
    0xFFAAAAAA,  // 1 - light gray
    0xFF555555,  // 2 - dark gray
    0xFF000000,  // 3 - black
};


PPU ppu = {0};

void PPU_Init() {
    ppu.mode = PPU_OAM_SEARCH;
    ppu.cycle_counter = 0;
    ppu.sprite_count = 0x00;
    memset(ppu.line_buffer, 0, 80);
}

void PPU_Step(uint8_t cycles) {
    uint8_t lcdc = Memory_Read_Byte(LCDC_REG);
    if (!(lcdc & LCDC_ENABLE)) {
        return;
    }
    ppu.cycle_counter += cycles;
    uint8_t stat = Memory_Read_Byte(STAT_REG);

    switch (ppu.mode) {
        case PPU_OAM_SEARCH:
            if (ppu.cycle_counter >= 80) {
                ppu.cycle_counter -= 80;
                ppu.mode = PPU_PIXEL_TRANSFER;

                PPU_Fetch_Sprite();

            }
            break;

        case PPU_PIXEL_TRANSFER:
            if (ppu.cycle_counter >= 172) {
                ppu.cycle_counter -= 172;
                ppu.mode = PPU_H_BLANK;

                PPU_Render_Scanline();

                uint8_t ly = Memory_Read_Byte(0xFF44);
                PPU_Line_Complete(ly);

                if (stat & STAT_HBLANK_INT) {
                    uint8_t if_reg = Memory_Read_Byte(IF_REG);
                    Memory_Write_Byte(IF_REG, if_reg | 0x02);
                }
            }
            break;

        case PPU_H_BLANK:
            if (ppu.cycle_counter >= 204) {
                ppu.cycle_counter -= 204;

                uint8_t ly = Memory_Read_Byte(0xFF44);
                ly++;
                Memory_Write_Byte(0xFF44, ly);

                if (ly >= 144) {
                    ppu.mode = PPU_V_BLANK;
                    uint8_t if_reg = Memory_Read_Byte(IF_REG);
                    Memory_Write_Byte(IF_REG, if_reg | 0x01);

                    if (stat & STAT_VBLANK_INT) {
                        if_reg = Memory_Read_Byte(IF_REG);
                        Memory_Write_Byte(IF_REG, if_reg | 0x02);
                    }
                } else {
                    ppu.mode = PPU_OAM_SEARCH;
                    if (stat & STAT_OAM_INT) {
                        uint8_t if_reg = Memory_Read_Byte(IF_REG);
                        Memory_Write_Byte(IF_REG, if_reg | 0x02);
                    }
                }

                uint8_t lyc = Memory_Read_Byte(0xFF45);
                if (ly == lyc) {
                    stat |= STAT_LYC_COMPARE;
                    if (stat & STAT_LYC_INT) {
                        uint8_t if_reg = Memory_Read_Byte(IF_REG);
                        Memory_Write_Byte(IF_REG, if_reg | 0x02);
                    }
                } else {
                    stat &= ~STAT_LYC_COMPARE;
                }
            }
            break;

        case PPU_V_BLANK:
            if (ppu.cycle_counter >= 456) {
                ppu.cycle_counter -= 456;

                uint8_t ly = Memory_Read_Byte(0xFF44);
                ly++;
                Memory_Write_Byte(0xFF44, ly);

                if (ly >= 154) {
                    ly = 0;
                    Memory_Write_Byte(0xFF44, ly);
                    ppu.mode = PPU_OAM_SEARCH;
                    if (stat & STAT_OAM_INT) {
                        uint8_t if_reg = Memory_Read_Byte(IF_REG);
                        Memory_Write_Byte(IF_REG, if_reg | 0x02);
                    }
                }

                uint8_t lyc = Memory_Read_Byte(0xFF45);
                if (ly == lyc) {
                    stat |= STAT_LYC_COMPARE;
                    if (stat & STAT_LYC_INT) {
                        uint8_t if_reg = Memory_Read_Byte(IF_REG);
                        Memory_Write_Byte(IF_REG, if_reg | 0x02);
                    }
                } else {
                    stat &= ~STAT_LYC_COMPARE;
                }
            }
            break;
    }

    stat = (stat & ~STAT_MODE_MASK) | (ppu.mode & STAT_MODE_MASK);
    Memory_Write_Byte(STAT_REG, stat);
}

void PPU_Fetch_Sprite() {
    uint8_t lcdc = Memory_Read_Byte(LCDC_REG);

    if (!(lcdc & LCDC_OBJ_EN)) {
        ppu.sprite_count = 0x00;
        return;
    }

    uint8_t sprite_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;
    uint8_t ly = Memory_Read_Byte(0xFF44);
    ppu.sprite_count = 0;

    for (uint8_t i = 0; i < 40 && ppu.sprite_count < 10; i++) {
        uint16_t oam_addr = 0xFE00 + (i * 4);
        uint8_t sprite_y = Memory_Read_Byte(oam_addr);
        uint8_t sprite_x = Memory_Read_Byte(oam_addr + 1);
        uint8_t tile_num = Memory_Read_Byte(oam_addr + 2);
        uint8_t flags = Memory_Read_Byte(oam_addr + 3);

        sprite_y -= 16;
        sprite_x -= 8;

        if (ly >= sprite_y && ly < sprite_y + sprite_height) {
            ppu.sprites[ppu.sprite_count].y = sprite_y;
            ppu.sprites[ppu.sprite_count].x = sprite_x;
            ppu.sprites[ppu.sprite_count].tile = tile_num;
            ppu.sprites[ppu.sprite_count].flags = flags;
            ppu.sprite_count++;
        }
    }
}

static uint8_t PPU_Apply_Palette(uint8_t palette_reg, uint8_t color_idx) {
    return (palette_reg >> (color_idx * 2)) & 0x03;
}

void PPU_Render_Scanline() {
    uint8_t lcdc = Memory_Read_Byte(LCDC_REG);

    memset(ppu.line_buffer, 0, sizeof(ppu.line_buffer));

    if (lcdc & LCDC_BG_ENABLE) {
        PPU_Render_Background_Line();
    }

    if (lcdc & LCDC_OBJ_EN) {
        PPU_Render_Sprite_Line();
    }
}

void PPU_Render_Background_Line() {
    uint8_t lcdc = Memory_Read_Byte(LCDC_REG);
    uint8_t ly = Memory_Read_Byte(0xFF44);
    uint8_t scy = Memory_Read_Byte(0xFF42);
    uint8_t scx = Memory_Read_Byte(0xFF43);
    uint8_t bgp = Memory_Read_Byte(0xFF47);

    uint16_t bg_map_base = (lcdc & LCDC_BG_MAP) ? 0x9C00 : 0x9800;
    uint16_t tile_data_base = (lcdc & LCDC_TILE_SEL) ? 0x8000 : 0x8800;

    uint8_t y_in_map = (ly + scy);
    uint8_t map_row = (y_in_map / 8) & 0x1F;
    uint8_t y_in_tile = y_in_map % 8;

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        uint8_t x_in_map = (x + scx);
        uint8_t map_col = (x_in_map / 8) & 0x1F;
        uint8_t x_in_tile = x_in_map % 8;

        uint16_t map_addr = bg_map_base + (map_row * 32) + map_col;
        uint8_t tile_num = Memory_Read_Byte(map_addr);

        uint16_t tile_addr;
        if (lcdc & LCDC_TILE_SEL) {
            tile_addr = 0x8000 + (tile_num * 16) + (y_in_tile * 2);
        } else {
            tile_addr = 0x9000 + ((int8_t)tile_num * 16) + (y_in_tile * 2);
        }
        uint8_t tile_byte1 = Memory_Read_Byte(tile_addr);
        uint8_t tile_byte2 = Memory_Read_Byte(tile_addr + 1);

        uint8_t bit_pos = 7 - x_in_tile;
        uint8_t palette_idx = ((tile_byte2 >> bit_pos) & 1) << 1 | ((tile_byte1 >> bit_pos) & 1);

        uint8_t color_idx = PPU_Apply_Palette(bgp, palette_idx);

        uint8_t byte_idx = x / 2;
        if (x % 2 == 0) {
            ppu.line_buffer[byte_idx] = (ppu.line_buffer[byte_idx] & 0x0F) | (color_idx << 4);
        } else {
            ppu.line_buffer[byte_idx] = (ppu.line_buffer[byte_idx] & 0xF0) | color_idx;
        }
    }
}

    void PPU_Render_Sprite_Line(void) {
    uint8_t lcdc = Memory_Read_Byte(LCDC_REG);
    uint8_t ly = Memory_Read_Byte(0xFF44);
    uint8_t sprite_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

    for (int i = 0; i < ppu.sprite_count; i++) {
        uint8_t sprite_x = ppu.sprites[i].x;
        uint8_t sprite_y = ppu.sprites[i].y;
        uint8_t tile_num = ppu.sprites[i].tile;
        uint8_t flags = ppu.sprites[i].flags;

        uint8_t palette = (flags & 0x10) ? Memory_Read_Byte(0xFF49) : Memory_Read_Byte(0xFF48);

        uint8_t flip_x = flags & 0x20;
        uint8_t flip_y = flags & 0x40;
        uint8_t bg_priority = flags & 0x80;

        uint8_t y_in_sprite = ly - sprite_y;
        if (flip_y) {
            y_in_sprite = sprite_height - 1 - y_in_sprite;
        }

        const uint16_t tile_addr = 0x8000 + (tile_num * 16) + (y_in_sprite * 2);
        const uint8_t tile_byte1 = Memory_Read_Byte(tile_addr);
        const uint8_t tile_byte2 = Memory_Read_Byte(tile_addr + 1);

        for (int px = 0; px < 8; px++) {
            int screen_x = sprite_x + px;

            if (screen_x < 0 || screen_x >= SCREEN_WIDTH) continue;

            uint8_t bit_pos = flip_x ? px : (7 - px);
            uint8_t palette_idx = ((tile_byte2 >> bit_pos) & 1) << 1 | ((tile_byte1 >> bit_pos) & 1);

            if (palette_idx == 0) continue;

            const uint8_t byte_idx = screen_x / 2;

            if (bg_priority) {
                uint8_t cur_byte = ppu.line_buffer[byte_idx];
                uint8_t cur_color = (screen_x % 2 == 0) ? (cur_byte >> 4) & 0x0F : cur_byte & 0x0F;
                if (cur_color != 0) continue;
            }

            const uint8_t color_idx = PPU_Apply_Palette(palette, palette_idx);

            if (screen_x % 2 == 0) {
                ppu.line_buffer[byte_idx] = (ppu.line_buffer[byte_idx] & 0x0F) | (color_idx << 4);
            } else {
                ppu.line_buffer[byte_idx] = (ppu.line_buffer[byte_idx] & 0xF0) | color_idx;
            }
        }
    }
}


// Redundant helper functions

// void SetConsoleColor(int fg, int bg) {
//     HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//     SetConsoleTextAttribute(hConsole, (bg << 4) | fg);
// }
//
// void ResetConsoleColor() {
//     SetConsoleColor(COLOR_WHITE, 0);
// }
//
// void ClearConsole() {
//     HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//     COORD coord = {0, 0};
//     DWORD count;
//
//     SetConsoleCursorPosition(hConsole, coord);
//
//     // Fill screen with spaces (clears it)
//     FillConsoleOutputCharacter(hConsole, ' ', 80 * 30, coord, &count);
//
//     // Move cursor back to top
//     SetConsoleCursorPosition(hConsole, coord);
// }


// Render the window in terminal
// void PPU_Line_Complete(uint8_t line) {
//     static uint8_t screen_buffer[144][80];
//     static int frame_count = 0;
//
//     if (line >= 144) {
//         return;
//     }
//
//     memcpy(screen_buffer[line], ppu.line_buffer, 80);
//
//     if (line == 143) {
//         frame_count++;
//
//
//         if (!memory.boot_rom_en && frame_count % 10 == 0) {
//             // ClearConsole();
//
//             printf("FRAME %5d\n", frame_count);
//
//             for (int line_idx = 0; line_idx < 144; line_idx++) {
//                 for (int i = 0; i < 80; i++) {
//                     uint8_t byte = screen_buffer[line_idx][i];
//                     uint8_t left = (byte >> 4) & 0x0F;
//                     uint8_t right = byte & 0x0F;
//
//                     char left_c = (left == 0) ? ' ' : (left == 1) ? '.' : (left == 2) ? '*' : '#';
//                     char right_c = (right == 0) ? ' ' : (right == 1) ? '.' : (right == 2) ? '*' : '#';
//
//                     printf("%c%c", left_c, right_c);
//                 }
//                 printf("\n");
//             }
//             get_input();
//         }
//     }
// }

