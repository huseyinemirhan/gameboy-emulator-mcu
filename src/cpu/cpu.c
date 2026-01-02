/*
 * cpu.c
 *
 *  Created on: Dec 30, 2025
 *      Author: MSI Cyborg
 */

#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "../memory/memory.h"

#define LD_R_R(name, dest, src) \
static int op_##name(){			\
	dest = src;					\
	return 4;					\
}								\


#define LD_R_R16(name, dest, src1, src2)\
static int op_##name(){					\
	uint16_t addr = (src1 << 8) | src2 ;	\
	dest = Memory_Read_Byte(addr);		\
	return 8;							\
}

#define LD_R16_R(name, src, dest1, dest2) \
static int op_##name(){					  \
	uint16_t addr = (dest1 << 8) | dest2 ;\
	Memory_Write_Byte(addr, src);	  \
	return 8;							  \
}


// LD B, r
LD_R_R(40, cpu.B, cpu.B)
LD_R_R(41, cpu.B, cpu.C)
LD_R_R(42, cpu.B, cpu.D)
LD_R_R(43, cpu.B, cpu.E)
LD_R_R(44, cpu.B, cpu.H)
LD_R_R(45, cpu.B, cpu.L)
LD_R_R16(46, cpu.B, cpu.H, cpu.L);
LD_R_R(47, cpu.B, cpu.A)

// LD C, r
LD_R_R(48, cpu.C, cpu.B)
LD_R_R(49, cpu.C, cpu.C)
LD_R_R(4A, cpu.C, cpu.D)
LD_R_R(4B, cpu.C, cpu.E)
LD_R_R(4C, cpu.C, cpu.H)
LD_R_R(4D, cpu.C, cpu.L)
LD_R_R16(4E, cpu.C, cpu.H, cpu.L);
LD_R_R(4F, cpu.C, cpu.A)

// LD D, r
LD_R_R(50, cpu.D, cpu.B)
LD_R_R(51, cpu.D, cpu.C)
LD_R_R(52, cpu.D, cpu.D)
LD_R_R(53, cpu.D, cpu.E)
LD_R_R(54, cpu.D, cpu.H)
LD_R_R(55, cpu.D, cpu.L)
LD_R_R16(56, cpu.D, cpu.H, cpu.L);
LD_R_R(57, cpu.D, cpu.A)

// LD E, r
LD_R_R(58, cpu.E, cpu.B)
LD_R_R(59, cpu.E, cpu.C)
LD_R_R(5A, cpu.E, cpu.D)
LD_R_R(5B, cpu.E, cpu.E)
LD_R_R(5C, cpu.E, cpu.H)
LD_R_R(5D, cpu.E, cpu.L)
LD_R_R16(5E, cpu.E, cpu.H, cpu.L);
LD_R_R(5F, cpu.E, cpu.A)

// LD H, r
LD_R_R(60, cpu.H, cpu.B)
LD_R_R(61, cpu.H, cpu.C)
LD_R_R(62, cpu.H, cpu.D)
LD_R_R(63, cpu.H, cpu.E)
LD_R_R(64, cpu.H, cpu.H)
LD_R_R(65, cpu.H, cpu.L)
LD_R_R16(66, cpu.H, cpu.H, cpu.L);
LD_R_R(67, cpu.H, cpu.A)

// LD L, r
LD_R_R(68, cpu.L, cpu.B)
LD_R_R(69, cpu.L, cpu.C)
LD_R_R(6A, cpu.L, cpu.D)
LD_R_R(6B, cpu.L, cpu.E)
LD_R_R(6C, cpu.L, cpu.H)
LD_R_R(6D, cpu.L, cpu.L)
LD_R_R16(6E, cpu.L, cpu.H, cpu.L);
LD_R_R(6F, cpu.L, cpu.A)

// LD HL, r
LD_R16_R(70,cpu.B, cpu.H, cpu.L);
LD_R16_R(71,cpu.C, cpu.H, cpu.L);
LD_R16_R(72,cpu.D, cpu.H, cpu.L);
LD_R16_R(73,cpu.E, cpu.H, cpu.L);
LD_R16_R(74,cpu.H, cpu.H, cpu.L);
LD_R16_R(75,cpu.L, cpu.H, cpu.L);
LD_R16_R(77,cpu.A, cpu.H, cpu.L);

// LD A, r
LD_R_R(78, cpu.A, cpu.B)
LD_R_R(79, cpu.A, cpu.C)
LD_R_R(7A, cpu.A, cpu.D)
LD_R_R(7B, cpu.A, cpu.E)
LD_R_R(7C, cpu.A, cpu.H)
LD_R_R(7D, cpu.A, cpu.L)
LD_R_R16(7E, cpu.A, cpu.H, cpu.L);
LD_R_R(7F, cpu.A, cpu.A)


CPU cpu = {0};



typedef int (*opcode_func)(void);

//opcodes
static int op_00(){
	return 4;
}

static int op_06(){
	cpu.B = Memory_Read_Byte(cpu.PC++);
	return 8;
}

static int op_16(){
	cpu.D = Memory_Read_Byte(cpu.PC++);
	return 8;
}

static int op_26(){
	cpu.H = Memory_Read_Byte(cpu.PC++);
	return 8;
}
static int op_0E(){
	cpu.C = Memory_Read_Byte(cpu.PC++);
	return 8;
}
static int op_1E(){
	cpu.E = Memory_Read_Byte(cpu.PC++);
	return 8;
}

static int op_2E(){
	cpu.L = Memory_Read_Byte(cpu.PC++);
	return 8;
}

static int op_3E(){
	cpu.A = Memory_Read_Byte(cpu.PC++);
	return 8;
}

static int op_undefined(){

	printf("Undefined OpCode\n");
	return 0;
}


const opcode_func opcode_func_table[256] = {
    [0x00 ... 0xFF] = op_undefined,

    [0x00] = op_00,

    // LD r, n
    [0x06] = op_06, [0x0E] = op_0E, [0x16] = op_16, [0x1E] = op_1E,
    [0x26] = op_26, [0x2E] = op_2E, [0x3E] = op_3E,

    // LD B, r
    [0x40] = op_40, [0x41] = op_41, [0x42] = op_42, [0x43] = op_43,
    [0x44] = op_44, [0x45] = op_45, [0x46] = op_46, [0x47] = op_47,

    // LD C, r
    [0x48] = op_48, [0x49] = op_49, [0x4A] = op_4A, [0x4B] = op_4B,
    [0x4C] = op_4C, [0x4D] = op_4D, [0x4E] = op_4E,  [0x4F] = op_4F,

    // LD D, r
    [0x50] = op_50, [0x51] = op_51, [0x52] = op_52, [0x53] = op_53,
    [0x54] = op_54, [0x55] = op_55, [0x56] = op_56, [0x57] = op_57,

    // LD E, r
    [0x58] = op_58, [0x59] = op_59, [0x5A] = op_5A, [0x5B] = op_5B,
    [0x5C] = op_5C, [0x5D] = op_5D, [0x5E] = op_5E, [0x5F] = op_5F,

    // LD H, r
    [0x60] = op_60, [0x61] = op_61, [0x62] = op_62, [0x63] = op_63,
    [0x64] = op_64, [0x65] = op_65, [0x66] = op_66,  [0x67] = op_67,

    // LD L, r
    [0x68] = op_68, [0x69] = op_69, [0x6A] = op_6A, [0x6B] = op_6B,
    [0x6C] = op_6C, [0x6D] = op_6D, [0x6E] = op_6E,  [0x6F] = op_6F,

	// LD (HL), r
	[0x70] = op_70, [0x71] = op_71, [0x72] = op_72, [0x73] = op_73,
	[0x74] = op_74, [0x75] = op_75, [0x77] = op_77,

    // LD A, r
    [0x78] = op_78, [0x79] = op_79, [0x7A] = op_7A, [0x7B] = op_7B,
    [0x7C] = op_7C, [0x7D] = op_7D, [0x7F] = op_7F, [0x7E] = op_7E

};

void CPU_Init(){

	cpu.PC = 0x00;
	cpu.SP = 0xFFFE;
	cpu.F = 0x00;

}

int CPU_Step(){

	uint8_t op_code = Memory_Read_Byte(cpu.PC++);
	int cycles = opcode_func_table[op_code]();
	cpu.cycles += cycles;

	return cycles;

}

void CPU_Set_Flag(uint8_t flag){

	cpu.F |= flag;

}

void CPU_Clear_Flag(uint8_t flag){

	cpu.F &= ~flag;

}

uint8_t CPU_Get_Flag(uint8_t flag){

	return (cpu.F & flag) ? 1 : 0;

}

