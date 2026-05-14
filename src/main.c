	/*
 * main.c
 *
 *  Created on: Dec 30, 2025
 *      Author: MSI Cyborg
 */

#include "cpu/cpu.h"
#include "memory/memory.h"
#include "memory/cartridge.h"
#include "ppu/ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Legally obtained roms, change them with your own roms(≖_≖ )
const char BOOT_ROM_ADDRESS[] = "../../roms/boot_rom.bin";
const char ROM_ADDRESS[] = "../../roms/mario.gb";

FILE *debug_file = NULL;

char serial_buffer[1024];
int serial_index = 0;

uint8_t *load_rom_from_file(const char *file_name){
	FILE *f = fopen(file_name, "rb");
	if(!f) return NULL;
	size_t size = 0;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *rom = malloc(size);
	fread(rom, 1, size, f);
	fclose(f);
	return rom;
}


int main(){
	Memory_Init();
	CPU_Init();
	PPU_Init();

	uint8_t *boot_rom = load_rom_from_file(BOOT_ROM_ADDRESS);
	uint8_t *rom = load_rom_from_file(ROM_ADDRESS);

	if (!rom)

		return 1;

	memory.boot_rom = boot_rom;
	Cartridge_Init(&memory.cartridge, rom);

	uint64_t total_cycles = 0;
	uint64_t max_cycles = 10000000000;

	while (total_cycles <= max_cycles) {
		int cycles = 0;

		if(!cpu.halted){
			cycles = CPU_Step();
		} else {
			cycles = 4;
		}

		PPU_Step(cycles);
		total_cycles += cycles;
		CPU_Handle_Interrupts();
		CPU_Run_Timer(cycles);
	}
	return 0;
}
