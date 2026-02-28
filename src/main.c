	/*
 * main.c
 *
 *  Created on: Dec 30, 2025
 *      Author: MSI Cyborg
 */

#include "cpu/cpu.h"
#include "memory/memory.h"
#include "memory/cartridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


char serial_buffer[1024];
int serial_index = 0;

void print_debug(){
	printf("PC: %d Cycles: %d\n",cpu.PC,cpu.cycles);
	printf("------REGISTERS-------\n");
	printf("A: %x B: %x C: %x D: %x E: %d H: %x L: %x\n",cpu.A,cpu.B,cpu.C,cpu.D,cpu.E, cpu.H, cpu.L);
	printf("-Flags-\n");
	printf("Z: %d N: %d H: %d C: %d\n",CPU_Get_Flag(Z_FLAG),CPU_Get_Flag(N_FLAG),CPU_Get_Flag(H_FLAG),CPU_Get_Flag(C_FLAG) );
}

uint8_t *load_test_rom(const char *file_name, size_t *size){
	FILE *f = fopen(file_name, "rb");

	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *rom = malloc(*size);
	fread(rom, 1, *size, f);
	fclose(f);
	return rom;
}

void check_serial_output(uint16_t addr, uint8_t value){
	if(addr == 0xFF02 && value == 0x81){
		char c = Memory_Read_Byte(0xFF01);
		serial_buffer[serial_index++] = c;
		serial_buffer[serial_index] = '\0';
		putchar(c);
		fflush(stdout);

	}
}


int main(int argc, char* argv[]){
	if (argc < 2) {
    printf("Usage: %s <rom_file>\n", argv[0]);
    return 1;
    }

	size_t rom_size;
	uint8_t *rom_data = load_test_rom(argv[1], &rom_size);

	Memory_Init();
	Cartridge_Init(&memory.cartridge, rom_data);
	CPU_Init();
	printf("\n=== Starting CPU Test ===\n");
    printf("Serial output:\n");

	uint64_t total_cycles = 0;
	uint64_t max_cycles = 100000000000;
	while (total_cycles <= max_cycles)
	{
		int cycles = 0; 
		if(!cpu.halted){
			cycles = CPU_Step();
		}
		else{
			cycles = 4;
		}
		total_cycles += cycles;
		CPU_Handle_Interrupts();
		CPU_Run_Timer(cycles);
		if(total_cycles % 50000000 == 0){
    printf("\n[DEBUG] PC=0x%04X Serial so far: %s\n", cpu.PC, serial_buffer);
	}
	
	}

	printf("\n\nReached cycle limit (%llu cycles)\n", total_cycles);
    printf("Final serial output:\n%s\n", serial_buffer);
    
    free(rom_data);
    return 0;
}


