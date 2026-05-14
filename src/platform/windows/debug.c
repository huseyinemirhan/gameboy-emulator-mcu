//
// Created by MSI Cyborg on 14.05.2026.
//
#include "stdio.h"

#include "debug.h"
#include "../../memory/memory.h"
#include "../../cpu/cpu.h"
#include "../../ppu/ppu.h"

void print_cpu_debug(){
    printf("PC: %d Cycles: %d\n",cpu.PC,cpu.cycles);
    printf("------REGISTERS-------\n");
    printf("A: %x B: %x C: %x D: %x E: %d H: %x L: %x\n",cpu.A,cpu.B,cpu.C,cpu.D,cpu.E, cpu.H, cpu.L);
    printf("-Flags-\n");
    printf("Z: %d N: %d H: %d C: %d\n",CPU_Get_Flag(Z_FLAG),CPU_Get_Flag(N_FLAG),CPU_Get_Flag(H_FLAG),CPU_Get_Flag(C_FLAG) );
}

void print_ppu_debug() {
    printf("\n=== PPU DEBUG ===\n");
    printf("LCDC (0xFF40): 0x%02X\n", Memory_Read_Byte(0xFF40));
    printf("  - LCD Enable: %d\n", (Memory_Read_Byte(0xFF40) >> 7) & 1);
    printf("  - BG Enable: %d\n", Memory_Read_Byte(0xFF40) & 1);
    printf("  - OBJ Enable: %d\n", (Memory_Read_Byte(0xFF40) >> 1) & 1);

    printf("STAT (0xFF41): 0x%02X\n", Memory_Read_Byte(0xFF41));
    printf("LY (0xFF44): %d\n", Memory_Read_Byte(0xFF44));
    printf("LYC (0xFF45): %d\n", Memory_Read_Byte(0xFF45));

    printf("SCX (0xFF43): %d\n", Memory_Read_Byte(0xFF43));
    printf("SCY (0xFF42): %d\n", Memory_Read_Byte(0xFF42));

    printf("BGP (0xFF47): 0x%02X\n", Memory_Read_Byte(0xFF47));

    printf("PPU Mode: %d\n", ppu.mode);
    printf("PPU Cycle Counter: %d\n", ppu.cycle_counter);
    printf("PPU Sprite Count: %d\n", ppu.sprite_count);

    // Check first few bytes of line buffer
    printf("Line Buffer (first 10 bytes): ");
    for (int i = 0; i < 10; i++) {
        printf("%02X ", ppu.line_buffer[i]);
    }
    printf("\n");
    printf("=================\n\n");
}

void print_cartridge_debug(Cartridge *cart) {
    printf("=== Cartridge Info ===\n");
    printf("Title: ");
    for (int i = TITLE_START_ADDR; i <= TITLE_END_ADDR && cart->rom_bank0[i]; i++) {
        printf("%c", cart->rom_bank0[i]);
    }
    printf("\n");

    printf("Type: ");
    switch(cart->mbc_type) {
        case NO_MBC: printf("ROM Only\n"); break;
        case MBC1:   printf("MBC1\n"); break;
        case MBC2:   printf("MBC2\n"); break;
        case MBC3:   printf("MBC3\n"); break;
        case MBC5:   printf("MBC5\n"); break;
    }

    printf("ROM Size: %d KB (%d banks)\n", cart->rom_size / 1024, cart->rom_bank_count);
    printf("RAM Size: %d KB\n", cart->ext_ram_size / 1024);
    printf("CGB Flag: 0x%02X\n", cart->rom_bank0[CGB_FLAG_ADDR]);
    printf("======================\n");

}

void print_debug(char *str) {
    printf("%s", str);
}


#include "debug.h"
