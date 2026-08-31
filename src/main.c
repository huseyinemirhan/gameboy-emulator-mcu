#include "cpu/cpu.h"
#include "memory/memory.h"
#include "memory/cartridge.h"
#include "ppu/ppu.h"
#include "platform/pico/inputs.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include <stdint.h>
#include "hardware/clocks.h"
#include "pico/multicore.h"
#include "platform/pico/tft_screen.h"
#include "platform/pico/agent_link.h"

extern const uint8_t rom[];
extern const uint8_t boot_rom_data[];

#define GB_CYCLES_PER_FRAME 70224
#define GB_FRAME_US 16743

#define GB_MAX_LAG_US (GB_FRAME_US * 4)

int main(){

    set_sys_clock_khz(266000, true);
    stdio_init_all();

    Input_Init();
    AgentLink_Init();
    Memory_Init();

    memory.boot_rom = (uint8_t *)boot_rom_data;
    memory.boot_rom_en = 1;

    CPU_Init();
    PPU_Init();

    Cartridge_Init(&memory.cartridge, rom);

    multicore_launch_core1(Core1_Main);

    uint64_t next_frame_us = time_us_64();
    uint32_t frame_cycles = 0;
#if EMU_PROFILE
    uint64_t next_report_us = next_frame_us + 1000000;
    uint32_t emulated_frames = 0;
    uint32_t last_display_frames = 0;
    uint32_t last_dropped_frames = 0;
#endif

    while (1) {
        int cycles = 0;

        if(!cpu.halted){
            cycles = CPU_Step();
        } else {
            cycles = 4;
        }

        PPU_Step(cycles);
        CPU_Handle_Interrupts();
        CPU_Run_Timer(cycles);

        frame_cycles += (uint32_t)cycles;
        if (frame_cycles < GB_CYCLES_PER_FRAME) {
            continue;
        }
        frame_cycles -= GB_CYCLES_PER_FRAME;

        get_input();
        AgentLink_Frame();

#if EMU_PROFILE
        emulated_frames++;
#endif

        next_frame_us += GB_FRAME_US;
        const uint64_t now = time_us_64();

        if (now < next_frame_us) {
            busy_wait_until(from_us_since_boot(next_frame_us));
        } else if (now - next_frame_us > GB_MAX_LAG_US) {
            next_frame_us = now;
        }

#if EMU_PROFILE
        if (time_us_64() >= next_report_us) {
            const uint32_t display_frames = prof_display_frames;
            const uint32_t dropped_frames = prof_dropped_frames;

            uint32_t vram_nonzero = 0;
            for (int i = 0; i < 8192; i++) {
                if (memory.vram[i]) vram_nonzero++;
            }

            printf("emu %lu | tft %lu | drop %lu | PC=%04X SP=%04X A=%02X F=%02X B=%02X C=%02X HL=%04X "
                   "LCDC=%02X BGP=%02X IE=%02X IF=%02X halt=%u vram=%lu | scale %lu wait %lu\n",
                   (unsigned long)emulated_frames,
                   (unsigned long)(display_frames - last_display_frames),
                   (unsigned long)(dropped_frames - last_dropped_frames),
                   (unsigned)cpu.PC, (unsigned)cpu.SP,
                   (unsigned)cpu.A, (unsigned)cpu.F, (unsigned)cpu.B, (unsigned)cpu.C,
                   (unsigned)((cpu.H << 8) | cpu.L),
                   (unsigned)memory.io_reg[0x40], (unsigned)memory.io_reg[0x47],
                   (unsigned)memory.ie, (unsigned)memory.io_reg[0x0F],
                   (unsigned)cpu.halted,
                   (unsigned long)vram_nonzero,
                   (unsigned long)prof_scale_us,
                   (unsigned long)prof_wait_us);

            emulated_frames = 0;
            last_display_frames = display_frames;
            last_dropped_frames = dropped_frames;
            next_report_us += 1000000;
            
            next_frame_us = time_us_64();
        }
#endif
    }
    return 0;
}
