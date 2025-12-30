/*
 * cpu.h
 *
 *  Created on: Dec 30, 2025
 *      Author: MSI Cyborg
 */

#ifndef SRC_CPU_CPU_H_
#define SRC_CPU_CPU_H_

#include <stdint.h>

#define Z_FLAG  0b10000000
#define N_FLAG  0b01000000
#define H_FLAG  0b00100000
#define C_FLAG  0b00010000


typedef struct{

	//Registers
	uint8_t A, B, C, D, E, H, L;
	uint16_t PC, SP;
	//Flag Register
	uint8_t F;
	uint32_t cycles;

}CPU;

extern CPU cpu;

void CPU_Init();
int CPU_Step();

void CPU_Set_Flag(uint8_t flag);
void CPU_Clear_Flag(uint8_t flag);
uint8_t CPU_Get_Flag(uint8_t flag);



#endif /* SRC_CPU_CPU_H_ */
