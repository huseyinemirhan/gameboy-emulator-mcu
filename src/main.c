	/*
 * main.c
 *
 *  Created on: Dec 30, 2025
 *      Author: MSI Cyborg
 */

#include "cpu/cpu.h"
#include "memory/memory.h"
#include "memory/sd_card.h"
#include "memory/cartridge.h"
#include <stdio.h>

void print_debug(){
	printf("PC: %d Cycles: %d\n",cpu.PC,cpu.cycles);
	printf("------REGISTERS-------\n");
	printf("A: %x B: %x C: %x D: %x E: %d H: %x L: %x\n",cpu.A,cpu.B,cpu.C,cpu.D,cpu.E, cpu.H, cpu.L);
	printf("-Flags-\n");
	printf("Z: %d N: %d H: %d C: %d\n",CPU_Get_Flag(Z_FLAG),CPU_Get_Flag(N_FLAG),CPU_Get_Flag(H_FLAG),CPU_Get_Flag(C_FLAG) );
}



int main(){
	Memory_Init();
	Cartridge_Init(&memory.cartridge, rom_data);
	CPU_Init();

	for(int i = 0; i<7; i++){
		CPU_Step();
		print_debug();
	}

	return 0;

}


