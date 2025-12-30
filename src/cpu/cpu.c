/*
 * cpu.c
 *
 *  Created on: Dec 30, 2025
 *      Author: MSI Cyborg
 */

#include <stdint.h>
#include <stdio.h>
#include "cpu.h"

CPU cpu = {0};
uint8_t rom[] = {0x00, 0x3E, 0x13, 0x3E, 0x11, 0X01}; //test

typedef int (*opcode_func)(void);

//opcodes
static int op_00(){
	return 4;
}
static int op_3E(){
	cpu.A = rom[cpu.PC++];
	return 8;
}


static int op_undefined(){

	printf("Undefined OpCode");
	return 0;
}


opcode_func opcode_func_table[256] = {
		[0 ... 255] = op_undefined,
		[0x00] = op_00,
		[0x3E] = op_3E

};

void CPU_Init(){

	cpu.PC = 0x00;
	cpu.SP = 0xFFFE;
	cpu.F = 0x00;

}

int CPU_Step(){

	uint8_t op_code = rom[cpu.PC++];
	int cycles = opcode_func_table[op_code]();
	cpu.cycles += cycles;

	return cycles;

}

void CPU_Set_Flag(uint8_t flag){

	cpu.F |= flag;
	cpu.F &= 0b11110000;
}

void CPU_Clear_Flag(uint8_t flag){

	cpu.F &= ~flag;
	cpu.F &= 0b11110000;

}

uint8_t CPU_Get_Flag(uint8_t flag){

	cpu.F &= 0b11110000;
	return cpu.F & flag; // non zero =  flag is set

}

