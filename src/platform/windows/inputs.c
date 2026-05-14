//
// Created by MSI Cyborg on 5.05.2026.
//

#include "inputs.h"

#include <windows.h>

#include "stdio.h"
#include "stdint.h"
#include "../memory/memory.h"
#include "../ppu/ppu.h"
#include <SDL2/SDL.h>


static SDL_Window   *sdl_window   = NULL;
static SDL_Renderer *sdl_renderer = NULL;
static SDL_Texture  *sdl_texture  = NULL;
static uint32_t      framebuffer[144][160];

// GB color palette
static const uint32_t GB_COLORS[4] = {
    0xFFFFFFFF,  // 0 - white
    0xFFAAAAAA,  // 1 - light gray
    0xFF555555,  // 2 - dark gray
    0xFF000000,  // 3 - black
};

void PPU_Line_Complete(uint8_t line) {
    if (sdl_window == NULL) {
        SDL_Init(SDL_INIT_VIDEO);
        sdl_window   = SDL_CreateWindow("GB Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 3, 144 * 3, 0);
        sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
        sdl_texture  = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    }

    if (line >= 144) return;

    for (int x = 0; x < 160; x++) {
        uint8_t byte = ppu.line_buffer[x / 2];
        uint8_t color_idx = (x % 2 == 0) ? (byte >> 4) & 0x0F : byte & 0x0F;
        if (color_idx > 3) color_idx = 3;
        framebuffer[line][x] = GB_COLORS[color_idx];
    }

    if (line == 143) {
        SDL_UpdateTexture(sdl_texture, NULL, framebuffer, 160 * sizeof(uint32_t));
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
        SDL_RenderPresent(sdl_renderer);

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:     memory.d_pad_state   &= ~UP_BUTTON_MASK;    break;
                    case SDLK_DOWN:   memory.d_pad_state   &= ~DOWN_BUTTON_MASK;  break;
                    case SDLK_LEFT:   memory.d_pad_state   &= ~LEFT_BUTTON_MASK;  break;
                    case SDLK_RIGHT:  memory.d_pad_state   &= ~RIGHT_BUTTON_MASK; break;
                    case SDLK_z:      memory.buttons_state &= ~A_BUTTON_MASK;     break;
                    case SDLK_x:      memory.buttons_state &= ~B_BUTTON_MASK;     break;
                    case SDLK_RETURN: memory.buttons_state &= ~START_BUTTON_MASK;  break;
                    case SDLK_RSHIFT: memory.buttons_state &= ~SELECT_BUTTON_MASK; break;
                }
            }
            if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:     memory.d_pad_state   |= UP_BUTTON_MASK;    break;
                    case SDLK_DOWN:   memory.d_pad_state   |= DOWN_BUTTON_MASK;  break;
                    case SDLK_LEFT:   memory.d_pad_state   |= LEFT_BUTTON_MASK;  break;
                    case SDLK_RIGHT:  memory.d_pad_state   |= RIGHT_BUTTON_MASK; break;
                    case SDLK_z:      memory.buttons_state |= A_BUTTON_MASK;     break;
                    case SDLK_x:      memory.buttons_state |= B_BUTTON_MASK;     break;
                    case SDLK_RETURN: memory.buttons_state |= START_BUTTON_MASK;  break;
                    case SDLK_RSHIFT: memory.buttons_state |= SELECT_BUTTON_MASK; break;
                }
            }
        }
        static uint32_t last_time = 0;
        uint32_t current_time = SDL_GetTicks();
        int delay = 16 - (int)(current_time - last_time);
        if (delay > 0) SDL_Delay(delay);
        last_time = SDL_GetTicks();
    }

}