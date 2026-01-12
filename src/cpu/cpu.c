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
}								

#define LD_R_N(name, dest) \
static int op_##name(){ \
    dest = Memory_Read_Byte(cpu.PC++); \
    return 8; \
}

#define LD_R_R16(name, dest, src1, src2)\
static int op_##name(){					\
	uint16_t addr = (src1 << 8) | src2 ;\
	dest = Memory_Read_Byte(addr);		\
	return 8;							\
}										

#define LD_R16_R(name, src, dest1, dest2) \
static int op_##name(){					  \
	uint16_t addr = (dest1 << 8) | dest2 ;\
	Memory_Write_Byte(addr, src);	  \
	return 8;							  \
}											

#define LD_R16_NN(name, dest1, dest2)\
static int op_##name(){\
	dest2 = Memory_Read_Byte(cpu.PC++);\
	dest1 = Memory_Read_Byte(cpu.PC++);\
	return 12;\
}

#define INC_R(name, reg)\
static int op_##name(){\
	uint8_t result = reg + 1;\
	CPU_Clear_Flag(N_FLAG);		\
	(result == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG); \
	((reg & 0x0F) == 0x0F) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG); \
	reg = result; \
	return 4;\
}

#define DEC_R(name, reg) \
static int op_##name(){ \
	uint8_t result = reg - 1;\
	CPU_Set_Flag(N_FLAG);\
	(result == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	((reg & 0x0F) == 0) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);\
	reg = result;\
	return 4;\
}

#define INC_R16(name, reg1, reg2)\
static int op_##name(){\
	uint16_t r16 = (reg1 << 8) | reg2;\
	r16++;			\
	reg1 = (r16 >> 8);\
	reg2 = (r16 & 0b0000000011111111);\
	return 8;\
}

#define DEC_R16(name, reg1, reg2)\
static int op_##name(){\
	uint16_t r16 = (reg1 << 8) | reg2;\
	r16--;			\
	reg1 = (r16 >> 8);\
	reg2 = (r16 & 0b0000000011111111);\
	return 8;\
}

#define ADD_R(name, reg)\
static int op_##name(){\
	uint8_t res = cpu.A + reg; \
	CPU_Clear_Flag(N_FLAG);	\
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	((cpu.A & 0x0F) + (reg & 0x0F) > 0x0F) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);\
	(res < cpu.A)	? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);\
	cpu.A = res;	\
	return 4; \
}

#define SUB_R(name, reg)\
static int op_##name(){\
	uint8_t res = cpu.A - reg; \
	CPU_Set_Flag(N_FLAG);	\
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	((cpu.A & 0x0F) < (reg & 0x0F)) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);\
	(reg > cpu.A) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);\
	cpu.A = res;	\
	return 4; \
}

#define ADC_R(name, reg)\
static int op_##name(){\
	uint8_t Cy = CPU_Get_Flag(C_FLAG);\
	uint16_t full_sum = cpu.A + reg + Cy;\
	uint8_t res = (uint8_t)full_sum; \
	CPU_Clear_Flag(N_FLAG);	\
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	((cpu.A & 0x0F) + (reg & 0x0F) + Cy > 0x0F) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);\
	(full_sum > 0xFF)	? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);\
	cpu.A = res;	\
	return 4; \
}

#define SBC_R(name, reg)\
static int op_##name(){\
	uint8_t Cy = CPU_Get_Flag(C_FLAG);\
	int res = cpu.A - reg - Cy; \
	CPU_Set_Flag(N_FLAG);	\
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	((cpu.A & 0x0F) < (reg & 0x0F) + Cy) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);\
	((uint16_t)reg + (uint16_t)Cy > (uint16_t)cpu.A)	? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);\
    cpu.A = res; \
    return 4; \
}

#define CP_R(name,reg)\
static int op_##name(){\
	uint8_t res = cpu.A - reg; \
	CPU_Set_Flag(N_FLAG);	\
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	((cpu.A & 0x0F) < (reg & 0x0F)) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);\
	(reg > cpu.A) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);\
	return 4; \
}

#define AND_R(name, reg)\
static int op_##name(){\
	cpu.A = cpu.A & reg;\
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	CPU_Clear_Flag(N_FLAG);\
	CPU_Set_Flag(H_FLAG);\
	CPU_Clear_Flag(C_FLAG);\
	return 4;\
}

#define OR_R(name, reg)\
static int op_##name(){\
	cpu.A = cpu.A | reg;\
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	CPU_Clear_Flag(N_FLAG);\
	CPU_Clear_Flag(H_FLAG);\
	CPU_Clear_Flag(C_FLAG);\
	return 4;\
}

#define XOR_R(name, reg)\
static int op_##name(){\
	cpu.A = cpu.A ^ reg;\
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);\
	CPU_Clear_Flag(N_FLAG);\
	CPU_Clear_Flag(H_FLAG);\
	CPU_Clear_Flag(C_FLAG);\
	return 4;\
}

#define ADD_HL_R16(name, reg1, reg2)\
static int op_##name(){\
	uint16_t hl = Combine_Registers(cpu.H, cpu.L);\
	uint16_t reg16 = Combine_Registers(reg1, reg2);\
	uint16_t res = reg16 + hl;\
	cpu.H = (res >> 8);\
	cpu.L = res & 0b0000000011111111;\
	CPU_Clear_Flag(N_FLAG);\
	(((reg16 & 0x0FFF) + (hl & 0x0FFF)) > 0x0FFF) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);\
	(hl > res) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);\
	return 8;\
}

#define RST_N(name, n)\
static int op_##name(){\
	cpu.SP--;\
	Memory_Write_Byte(cpu.SP, (cpu.PC >> 8) & 0xFF);\
	cpu.SP--;\
	Memory_Write_Byte(cpu.SP, cpu.PC & 0xFF);\
	cpu.PC = n;\
	return 16;\
}

#define PUSH_R_R(name, reg1, reg2)\
static int op_##name(){\
	cpu.SP--;\
	Memory_Write_Byte(cpu.SP, reg1);\
	cpu.SP--;\
	Memory_Write_Byte(cpu.SP, reg2);\
	return 16;\
}

#define POP_R_R(name, reg1, reg2)\
static int op_##name(){\
	uint8_t lower = Memory_Read_Byte(cpu.SP++);\
	uint8_t higher = Memory_Read_Byte(cpu.SP++);\
	reg1 = higher;\
	reg2 = lower;\
	return 12;\
}

//LD r,n
LD_R_N(06, cpu.B)
LD_R_N(0E, cpu.C)
LD_R_N(16, cpu.D)
LD_R_N(1E, cpu.E)
LD_R_N(26, cpu.H)
LD_R_N(2E, cpu.L)
LD_R_N(3E, cpu.A)


// LD B, r
LD_R_R(40, cpu.B, cpu.B)
LD_R_R(41, cpu.B, cpu.C)
LD_R_R(42, cpu.B, cpu.D)
LD_R_R(43, cpu.B, cpu.E)
LD_R_R(44, cpu.B, cpu.H)
LD_R_R(45, cpu.B, cpu.L)
LD_R_R16(46, cpu.B, cpu.H, cpu.L)
LD_R_R(47, cpu.B, cpu.A)

// LD C, r
LD_R_R(48, cpu.C, cpu.B)
LD_R_R(49, cpu.C, cpu.C)
LD_R_R(4A, cpu.C, cpu.D)
LD_R_R(4B, cpu.C, cpu.E)
LD_R_R(4C, cpu.C, cpu.H)
LD_R_R(4D, cpu.C, cpu.L)
LD_R_R16(4E, cpu.C, cpu.H, cpu.L)
LD_R_R(4F, cpu.C, cpu.A)

// LD D, r
LD_R_R(50, cpu.D, cpu.B)
LD_R_R(51, cpu.D, cpu.C)
LD_R_R(52, cpu.D, cpu.D)
LD_R_R(53, cpu.D, cpu.E)
LD_R_R(54, cpu.D, cpu.H)
LD_R_R(55, cpu.D, cpu.L)
LD_R_R16(56, cpu.D, cpu.H, cpu.L)
LD_R_R(57, cpu.D, cpu.A)

// LD E, r
LD_R_R(58, cpu.E, cpu.B)
LD_R_R(59, cpu.E, cpu.C)
LD_R_R(5A, cpu.E, cpu.D)
LD_R_R(5B, cpu.E, cpu.E)
LD_R_R(5C, cpu.E, cpu.H)
LD_R_R(5D, cpu.E, cpu.L)
LD_R_R16(5E, cpu.E, cpu.H, cpu.L)
LD_R_R(5F, cpu.E, cpu.A)

// LD H, r
LD_R_R(60, cpu.H, cpu.B)
LD_R_R(61, cpu.H, cpu.C)
LD_R_R(62, cpu.H, cpu.D)
LD_R_R(63, cpu.H, cpu.E)
LD_R_R(64, cpu.H, cpu.H)
LD_R_R(65, cpu.H, cpu.L)
LD_R_R16(66, cpu.H, cpu.H, cpu.L)
LD_R_R(67, cpu.H, cpu.A)

// LD L, r
LD_R_R(68, cpu.L, cpu.B)
LD_R_R(69, cpu.L, cpu.C)
LD_R_R(6A, cpu.L, cpu.D)
LD_R_R(6B, cpu.L, cpu.E)
LD_R_R(6C, cpu.L, cpu.H)
LD_R_R(6D, cpu.L, cpu.L)
LD_R_R16(6E, cpu.L, cpu.H, cpu.L)
LD_R_R(6F, cpu.L, cpu.A)

// LD HL, r
LD_R16_R(70,cpu.B, cpu.H, cpu.L)
LD_R16_R(71,cpu.C, cpu.H, cpu.L)
LD_R16_R(72,cpu.D, cpu.H, cpu.L)
LD_R16_R(73,cpu.E, cpu.H, cpu.L)
LD_R16_R(74,cpu.H, cpu.H, cpu.L)
LD_R16_R(75,cpu.L, cpu.H, cpu.L)
LD_R16_R(77,cpu.A, cpu.H, cpu.L)

// LD R16, A
LD_R16_R(02, cpu.A, cpu.B, cpu.C)
LD_R16_R(12, cpu.A, cpu.D, cpu.E)

//LD A, R16
LD_R_R16(0A, cpu.A, cpu.B, cpu.C)
LD_R_R16(1A, cpu.A, cpu.D, cpu.E)

// LD A, r
LD_R_R(78, cpu.A, cpu.B)
LD_R_R(79, cpu.A, cpu.C)
LD_R_R(7A, cpu.A, cpu.D)
LD_R_R(7B, cpu.A, cpu.E)
LD_R_R(7C, cpu.A, cpu.H)
LD_R_R(7D, cpu.A, cpu.L)
LD_R_R16(7E, cpu.A, cpu.H, cpu.L)
LD_R_R(7F, cpu.A, cpu.A)

//LD R16, nn
LD_R16_NN(01, cpu.B, cpu.C)
LD_R16_NN(11, cpu.D, cpu.E)
LD_R16_NN(21, cpu.H, cpu.L)

// --- INC/DEC 8-bit ---
INC_R(04, cpu.B) INC_R(0C, cpu.C) INC_R(14, cpu.D) INC_R(1C, cpu.E) 
INC_R(24, cpu.H) INC_R(2C, cpu.L) INC_R(3C, cpu.A)
DEC_R(05, cpu.B) DEC_R(0D, cpu.C) DEC_R(15, cpu.D) DEC_R(1D, cpu.E) 
DEC_R(25, cpu.H) DEC_R(2D, cpu.L) DEC_R(3D, cpu.A)

// --- Arithmetic A, r ---
ADD_R(80, cpu.B) ADD_R(81, cpu.C) ADD_R(82, cpu.D) ADD_R(83, cpu.E) 
ADD_R(84, cpu.H) ADD_R(85, cpu.L) ADD_R(87, cpu.A)

ADC_R(88, cpu.B) ADC_R(89, cpu.C) ADC_R(8A, cpu.D) ADC_R(8B, cpu.E) 
ADC_R(8C, cpu.H) ADC_R(8D, cpu.L) ADC_R(8F, cpu.A)

SUB_R(90, cpu.B) SUB_R(91, cpu.C) SUB_R(92, cpu.D) SUB_R(93, cpu.E) 
SUB_R(94, cpu.H) SUB_R(95, cpu.L) SUB_R(97, cpu.A)

SBC_R(98, cpu.B) SBC_R(99, cpu.C) SBC_R(9A, cpu.D) SBC_R(9B, cpu.E) 
SBC_R(9C, cpu.H) SBC_R(9D, cpu.L) SBC_R(9F, cpu.A)

// --- Logic A, r ---
AND_R(A0, cpu.B) AND_R(A1, cpu.C) AND_R(A2, cpu.D) AND_R(A3, cpu.E) 
AND_R(A4, cpu.H) AND_R(A5, cpu.L) AND_R(A7, cpu.A)

XOR_R(A8, cpu.B) XOR_R(A9, cpu.C) XOR_R(AA, cpu.D) XOR_R(AB, cpu.E) 
XOR_R(AC, cpu.H) XOR_R(AD, cpu.L) XOR_R(AF, cpu.A)

OR_R(B0, cpu.B)  OR_R(B1, cpu.C)  OR_R(B2, cpu.D)  OR_R(B3, cpu.E)  
OR_R(B4, cpu.H)  OR_R(B5, cpu.L)  OR_R(B7, cpu.A)

CP_R(B8, cpu.B)  CP_R(B9, cpu.C)  CP_R(BA, cpu.D)  CP_R(BB, cpu.E)  
CP_R(BC, cpu.H)  CP_R(BD, cpu.L)  CP_R(BF, cpu.A)

// --- 16-bit ADD HL, rr ---
ADD_HL_R16(09, cpu.B, cpu.C)
ADD_HL_R16(19, cpu.D, cpu.E)
ADD_HL_R16(29, cpu.H, cpu.L)

// --- 16-bit INC DEC ---
INC_R16(03, cpu.B, cpu.C) 
INC_R16(13, cpu.D, cpu.E) 
INC_R16(23, cpu.H, cpu.L) 
DEC_R16(0B, cpu.B, cpu.C) 
DEC_R16(1B, cpu.D, cpu.E) 
DEC_R16(2B, cpu.H, cpu.L) 

// --- PUSH rr ---
PUSH_R_R(C5, cpu.B, cpu.C)
PUSH_R_R(D5, cpu.D, cpu.E)
PUSH_R_R(E5, cpu.H, cpu.L)
PUSH_R_R(F5, cpu.A, cpu.F)

// --- POP rr ---
POP_R_R(C1, cpu.B, cpu.C)
POP_R_R(D1, cpu.D, cpu.E)
POP_R_R(E1, cpu.H, cpu.L)
POP_R_R(F1, cpu.A, cpu.F)

// ---RST n ---
RST_N(C7, 0x0000) RST_N(CF, 0x0008) RST_N(D7, 0x0010) RST_N(DF, 0x0018)
RST_N(E7, 0x0020) RST_N(EF, 0x0028) RST_N(F7, 0x0030) RST_N(FF, 0x0038)

CPU cpu = {0};



typedef int (*opcode_func)(void);

//opcodes
static int op_00(){
	return 4;
}

//+-HL
static int op_22(){
	uint16_t addr = (cpu.H << 8) | cpu.L;
	Memory_Write_Byte(addr, cpu.A);
	addr++;
	cpu.H = (addr >> 8);
	cpu.L =	addr & 0b0000000011111111;

	return 8;
}

static int op_32(){
	uint16_t addr = (cpu.H << 8) | cpu.L;
	Memory_Write_Byte(addr--, cpu.A);
	cpu.H = (addr >> 8);
	cpu.L =	addr & 0b0000000011111111;

	return 8;
}

static int op_2A(){
	uint16_t addr = (cpu.H << 8) | cpu.L;
	cpu.A = Memory_Read_Byte(addr++);
	cpu.H = (addr >> 8);
	cpu.L =	addr & 0b0000000011111111;
	return 8;
}

static int op_3A(){
	uint16_t addr = (cpu.H << 8) | cpu.L;
	cpu.A = Memory_Read_Byte(addr--);
	cpu.H = (addr >> 8);
	cpu.L =	addr & 0b0000000011111111;
	return 8;
}
// LD (a16), A and LD A, (a16)
static int op_FA(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	cpu.A = Memory_Read_Byte((higher << 8) | lower);
	return 16;
}

static int op_EA(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	Memory_Write_Byte(higher << 8 | lower, cpu.A);
	return 16;
}

// LD HL n
static int op_36(){
	uint16_t addr = (cpu.H << 8) | cpu.L;
	Memory_Write_Byte(addr, Memory_Read_Byte(cpu.PC++));
	return 12;
}

//LD SP ??
static int op_31(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	cpu.SP = (higher << 8) | lower;
	return 12;
}

static int op_08(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	uint16_t addr = ((higher << 8) | lower);
	Memory_Write_Byte(addr, cpu.SP & 0xFF);
	Memory_Write_Byte(addr+1, cpu.SP >> 8);
	return 20;
}

static int op_F9(){
	cpu.SP = Combine_Registers(cpu.H, cpu.L);
	return 8;
}

//LD HL, SP+s8
static int op_F8() {
    int8_t e = (int8_t)Memory_Read_Byte(cpu.PC++);
    uint16_t sp = cpu.SP;
    uint32_t res = sp + e;

    cpu.H = (res >> 8) & 0xFF;
    cpu.L = res & 0xFF;

    CPU_Clear_Flag(Z_FLAG); 
    CPU_Clear_Flag(N_FLAG); 

    ((sp & 0x0F) + (e & 0x0F) > 0x0F) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
    ((sp & 0xFF) + (uint8_t)e > 0xFF) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);

    return 12; 
}

//LDH -> Acces to HRAM

static int op_F2() {
	cpu.A = Memory_Read_Byte(cpu.C + 0xFF00);
	return 8;
}

static int op_E2() {
	Memory_Write_Byte(cpu.C + 0XFF00, cpu.A);
	return 8;
}

static int op_F0() {
	uint8_t addr = Memory_Read_Byte(cpu.PC++);
	cpu.A = Memory_Read_Byte(addr + 0xFF00);
	return 12;
}

static int op_E0() {
	uint8_t addr = Memory_Read_Byte(cpu.PC++);
	Memory_Write_Byte(addr + 0XFF00, cpu.A);
	return 12;
}

//ADD n/ SUB n
static int op_C6(){
	uint8_t val = Memory_Read_Byte(cpu.PC++);
	uint8_t res = cpu.A + val;
	CPU_Clear_Flag(N_FLAG);
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) + (val & 0xF)) > 0x0F ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(cpu.A > res) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	cpu.A = res;
	return 8;
}

static int op_D6(){
	uint8_t val = Memory_Read_Byte(cpu.PC++);
	uint8_t res = cpu.A - val;
	CPU_Set_Flag(N_FLAG);
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) < (val & 0x0F)) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(val > cpu.A) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	cpu.A = res;
	return 8;
} 


// ADD/SUB A, (HL)
static int op_86(){
	uint16_t addr = Combine_Registers(cpu.H, cpu.L);
	uint8_t val = Memory_Read_Byte(addr);
	uint8_t res = val + cpu.A;
	CPU_Clear_Flag(N_FLAG);
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) + (val & 0xF)) > 0x0F ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(cpu.A > res) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	cpu.A = res;
	return 8;
}

static int op_96(){
	uint16_t addr = Combine_Registers(cpu.H, cpu.L);
	uint8_t val = Memory_Read_Byte(addr);
	uint8_t res = cpu.A - val;
	CPU_Set_Flag(N_FLAG);
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) < (val & 0x0F)) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(val > cpu.A) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	cpu.A = res;
	return 8;
}
// ADC/SBC A, (HL)
static int op_8E(){
	uint8_t Cy = CPU_Get_Flag(C_FLAG);
	uint8_t val = Memory_Read_Byte(Combine_Registers(cpu.H, cpu.L));
	uint16_t full_sum = cpu.A + val + Cy;
	uint8_t res = (uint8_t)full_sum; 
	CPU_Clear_Flag(N_FLAG);	
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) + (val & 0x0F) + Cy > 0x0F) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(full_sum > 0xFF) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	cpu.A = res;	
	return 8; 
}

static int op_9E(){
	uint8_t Cy = CPU_Get_Flag(C_FLAG);
	uint8_t val = Memory_Read_Byte(Combine_Registers(cpu.H, cpu.L));
	int res = cpu.A - val - Cy; 
	CPU_Set_Flag(N_FLAG);
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) < (val & 0x0F) + Cy) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	((uint16_t)val + (uint16_t)Cy > (uint16_t)cpu.A)	? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
    cpu.A = res; 
    return 8; 
}

//ADC/SBC A, N
static int op_CE(){
	uint8_t Cy = CPU_Get_Flag(C_FLAG);
	uint8_t val = Memory_Read_Byte(cpu.PC++);
	uint16_t full_sum = cpu.A + val + Cy;
	uint8_t res = (uint8_t)full_sum; 
	CPU_Clear_Flag(N_FLAG);	
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) + (val & 0x0F) + Cy > 0x0F) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(full_sum > 0xFF) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	cpu.A = res;	
	return 8; 
}

static int op_DE(){
	uint8_t Cy = CPU_Get_Flag(C_FLAG);
	uint8_t val = Memory_Read_Byte(cpu.PC++);
	int res = cpu.A - val - Cy; 
	CPU_Set_Flag(N_FLAG);
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) < (val & 0x0F) + Cy) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	((uint16_t)val + (uint16_t)Cy > (uint16_t)cpu.A)	? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
    cpu.A = res; 
    return 8; 
}

//CP (HL)
static int op_BE(){
	uint16_t addr = Combine_Registers(cpu.H, cpu.L);
	uint8_t val = Memory_Read_Byte(addr);
	uint8_t res = cpu.A - val;
	CPU_Set_Flag(N_FLAG);
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) < (val & 0x0F)) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(val > cpu.A) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	return 8;
}
//CP N
static int op_FE(){
	uint8_t val = Memory_Read_Byte(cpu.PC++);
	uint8_t res = cpu.A - val;
	CPU_Set_Flag(N_FLAG);
	(res == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((cpu.A & 0x0F) < (val & 0x0F)) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(val > cpu.A) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	return 8;
}

//INC/DEC (HL)
static int op_34(){
	uint16_t addr = Combine_Registers(cpu.H, cpu.L);
	uint8_t val = Memory_Read_Byte(addr);
	uint8_t result = val + 1;
	CPU_Clear_Flag(N_FLAG);	
	(result == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG); 
	((val & 0x0F) == 0x0F) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG); 
	Memory_Write_Byte(addr, result);
	return 12;

}

static int op_35(){
	uint16_t addr = Combine_Registers(cpu.H, cpu.L);
	uint8_t val = Memory_Read_Byte(addr); 
	uint8_t result = val - 1;
	CPU_Set_Flag(N_FLAG);
	(result == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	((val & 0x0F) == 0) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	Memory_Write_Byte(addr, result);
	return 12;
}

//ADD HL, SP
static int op_39(){
	uint16_t hl = Combine_Registers(cpu.H, cpu.L);
	uint16_t res = cpu.SP + hl;
	cpu.H = (res >> 8);
	cpu.L = res & 0b0000000011111111;
	CPU_Clear_Flag(N_FLAG);
	(((cpu.SP & 0x0FFF) + (hl & 0x0FFF)) > 0x0FFF) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(hl > res) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	return 8;
}

//AND (HL)
static int op_A6(){
	uint8_t val = Memory_Read_Byte(Combine_Registers(cpu.H, cpu.L));
	cpu.A = cpu.A & val;
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Set_Flag(H_FLAG);
	CPU_Clear_Flag(C_FLAG);
	return 8;
}

//AND N
static int op_E6(){
	uint8_t val = Memory_Read_Byte(cpu.PC++);
	cpu.A = cpu.A & val;
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Set_Flag(H_FLAG);
	CPU_Clear_Flag(C_FLAG);
	return 8;
}

//OR (HL)
static int op_B6(){
	uint8_t val = Memory_Read_Byte(Combine_Registers(cpu.H, cpu.L));
	cpu.A = cpu.A | val;
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	CPU_Clear_Flag(C_FLAG);
	return 8;
}

//OR N
static int op_F6(){
	uint8_t val = Memory_Read_Byte(cpu.PC++);
	cpu.A = cpu.A | val;
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	CPU_Clear_Flag(C_FLAG);
	return 8;
}

//XOR (HL)
static int op_AE(){
	uint8_t val = Memory_Read_Byte(Combine_Registers(cpu.H, cpu.L));
	cpu.A = cpu.A ^ val;
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	CPU_Clear_Flag(C_FLAG);
	return 8;
}

//XOR N
static int op_EE(){
	uint8_t val = Memory_Read_Byte(cpu.PC++);
	cpu.A = cpu.A ^ val;
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	CPU_Clear_Flag(C_FLAG);
	return 8;
}

//CCF
static int op_3F(){
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	CPU_Flip_Flag(C_FLAG);
	return 4;
}

//SCF
static int op_37(){
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	CPU_Set_Flag(C_FLAG);
	return 4;
}

//DAA
static int op_27(){
	if(!CPU_Get_Flag(N_FLAG)){
		if(CPU_Get_Flag(H_FLAG) || ( (cpu.A & 0b00001111) > 0x09)){
			cpu.A += 0x06;
		}
		if(CPU_Get_Flag(C_FLAG) || (cpu.A > 0x99)){
			cpu.A += 0x60;
			CPU_Set_Flag(C_FLAG);
		}
	}
	else{
		if(CPU_Get_Flag(H_FLAG)){
			cpu.A -= 0x06;
		}
		if(CPU_Get_Flag(C_FLAG)){
			cpu.A -= 0x60;
		}
	}
	cpu.A = cpu.A & 0xFF;
	(cpu.A == 0) ? CPU_Set_Flag(Z_FLAG) : CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(H_FLAG);
	return 4;
}
//CPL
static int op_2F(){
	cpu.A = ~cpu.A;
	CPU_Set_Flag(N_FLAG);
	CPU_Set_Flag(H_FLAG);
	return 4;
}

//INC/DEC SP
static int op_33() { 
	cpu.SP++; 
	return 8; 
}

static int op_3B() { 
	cpu.SP--; 
	return 8; 

}

//ADD SP,e
static int op_E8(){
	uint16_t old_SP = cpu.SP;
	int8_t val = Memory_Read_Byte(cpu.PC++);
	cpu.SP += val;
	CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	((old_SP & 0x0F) + (val & 0x0F) > 0x0F) ? CPU_Set_Flag(H_FLAG) : CPU_Clear_Flag(H_FLAG);
	(((old_SP & 0xFF) + (uint8_t)val) > 0xFF) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);
	return 16;
}

//JP nn
static int op_C3(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	cpu.PC = Combine_Registers(higher, lower);
	return 16;
}

// JP HL
static int op_E9(){
	uint16_t hl = Combine_Registers(cpu.H, cpu.L);
	cpu.PC = hl;
	return 4;
}

//JP cc nn
static int op_C2(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	if(!CPU_Get_Flag(Z_FLAG)){
		cpu.PC = Combine_Registers(higher, lower);
		return 16;
	}
	else{
		return 12;
	}
}

static int op_D2(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	if(!CPU_Get_Flag(C_FLAG)){
		cpu.PC = Combine_Registers(higher, lower);
		return 16;
	}
	else{
		return 12;
	}
}

static int op_CA(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	if(CPU_Get_Flag(Z_FLAG)){
		cpu.PC = Combine_Registers(higher, lower);
		return 16;
	}
	else{
		return 12;
	}
}

static int op_DA(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	if(CPU_Get_Flag(C_FLAG)){
		cpu.PC = Combine_Registers(higher, lower);
		return 16;
	}
	else{
		return 12;
	}
}

//JR e
static int op_18(){
	int8_t e = (int8_t)Memory_Read_Byte(cpu.PC++);
	cpu.PC+=e;
	return 12;
}

//JR cc, e
static int op_28(){
	int8_t e = (int8_t)Memory_Read_Byte(cpu.PC++);
	if(CPU_Get_Flag(Z_FLAG)){
		cpu.PC+=e;
		return 12;
	}
	else{
		return 8;
	}
}

static int op_38(){
	int8_t e = (int8_t)Memory_Read_Byte(cpu.PC++);
	if(CPU_Get_Flag(C_FLAG)){
		cpu.PC+=e;
		return 12;
	}
	else{
		return 8;
	}
}

static int op_20(){
	int8_t e = (int8_t)Memory_Read_Byte(cpu.PC++);
	if(!CPU_Get_Flag(Z_FLAG)){
		cpu.PC+=e;
		return 12;
	}
	else{
		return 8;
	}
}

static int op_30(){
	int8_t e = (int8_t)Memory_Read_Byte(cpu.PC++);
	if(!CPU_Get_Flag(C_FLAG)){
		cpu.PC+=e;
		return 12;
	}
	else{
		return 8;
	}
}

//CALL nn
static int op_CD(){
	uint16_t val = Read_nn_From_Rom();
	cpu.SP--;
	Memory_Write_Byte(cpu.SP, (cpu.PC >> 8) & 0xFF);
	cpu.SP--;
	Memory_Write_Byte(cpu.SP, cpu.PC & 0xFF);
	cpu.PC = val;

	return 24;

}

static int op_CC(){
	uint16_t val = Read_nn_From_Rom();
	if(CPU_Get_Flag(Z_FLAG)){
			cpu.SP--;
		Memory_Write_Byte(cpu.SP, (cpu.PC >> 8) & 0xFF);
		cpu.SP--;
		Memory_Write_Byte(cpu.SP, cpu.PC & 0xFF);
		cpu.PC = val;
		return 24;
	}
	else{
		return 12;
	}
}

static int op_DC(){
	uint16_t val = Read_nn_From_Rom();
	if(CPU_Get_Flag(C_FLAG)){
			cpu.SP--;
		Memory_Write_Byte(cpu.SP, (cpu.PC >> 8) & 0xFF);
		cpu.SP--;
		Memory_Write_Byte(cpu.SP, cpu.PC & 0xFF);
		cpu.PC = val;
		return 24;
	}
	else{
		return 12;
	}
}

static int op_C4(){
	uint16_t val = Read_nn_From_Rom();
	if(!CPU_Get_Flag(Z_FLAG)){
			cpu.SP--;
		Memory_Write_Byte(cpu.SP, (cpu.PC >> 8) & 0xFF);
		cpu.SP--;
		Memory_Write_Byte(cpu.SP, cpu.PC & 0xFF);
		cpu.PC = val;
		return 24;
	}
	else{
		return 12;
	}
}

static int op_D4(){
	uint16_t val = Read_nn_From_Rom();
	if(!CPU_Get_Flag(C_FLAG)){
		cpu.SP--;
		Memory_Write_Byte(cpu.SP, (cpu.PC >> 8) & 0xFF);
		cpu.SP--;
		Memory_Write_Byte(cpu.SP, cpu.PC & 0xFF);
		cpu.PC = val;
		return 24;
	}
	else{
		return 12;
	}
}

//RET
static int op_C9(){
	uint8_t lower = Memory_Read_Byte(cpu.SP++);
	uint8_t higher = Memory_Read_Byte(cpu.SP++);
	cpu.PC = Combine_Registers(higher, lower);
	return 16;
}

static int op_C8(){

	if(CPU_Get_Flag(Z_FLAG)){
	uint8_t lower = Memory_Read_Byte(cpu.SP++);
	uint8_t higher = Memory_Read_Byte(cpu.SP++);
	cpu.PC = Combine_Registers(higher, lower);
	return 20;
	}
	else{
		return 8;
	}
}

static int op_D8(){

	if(CPU_Get_Flag(C_FLAG)){
	uint8_t lower = Memory_Read_Byte(cpu.SP++);
	uint8_t higher = Memory_Read_Byte(cpu.SP++);
	cpu.PC = Combine_Registers(higher, lower);
	return 20;
	}
	else{
		return 8;
	}
}

static int op_C0(){

	if(!CPU_Get_Flag(Z_FLAG)){
	uint8_t lower = Memory_Read_Byte(cpu.SP++);
	uint8_t higher = Memory_Read_Byte(cpu.SP++);
	cpu.PC = Combine_Registers(higher, lower);
	return 20;
	}
	else{
		return 8;
	}
}

static int op_D0(){

	if(!CPU_Get_Flag(C_FLAG)){
	uint8_t lower = Memory_Read_Byte(cpu.SP++);
	uint8_t higher = Memory_Read_Byte(cpu.SP++);
	cpu.PC = Combine_Registers(higher, lower);
	return 20;
	}
	else{
		return 8;
	}
}

//POP AF
static int op_F1() {
    uint8_t f_val = Memory_Read_Byte(cpu.SP++);
    cpu.A = Memory_Read_Byte(cpu.SP++);
    cpu.F = f_val & 0xF0; 
    return 12;
}

//RETI

static int op_D9(){
	uint8_t lower = Memory_Read_Byte(cpu.SP++);
	uint8_t higher = Memory_Read_Byte(cpu.SP++);
	cpu.PC = Combine_Registers(higher, lower);
	memory.ie = 1;
	return 16;
}

//HALT

static int op_76(){
	cpu.halted = 1;
	return 4;
}

//EI

static int op_FB(){
	memory.ie = 1;
	return 4;
}

//DI
static int op_F3(){
	memory.ie = 0;
	return 4;
}

//STOP
static int op_10(){
	cpu.PC++;
	cpu.halted = 1;
	return 4;
}

//RLCA
static int op_07(){
	uint8_t c = (cpu.A >> 7);
	cpu.A = (cpu.A << 1) | c;
	
	CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	(c == 1) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);;
	
	return 4;
}

//RRCA
static int op_0F(){
	uint8_t c = (cpu.A & 0b00000001);
	cpu.A = (cpu.A >> 1) | (c << 7);
	
	CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	(c == 1) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);;
	
	return 4;
}

//RLA
static int op_17(){
	uint8_t c = CPU_Get_Flag(C_FLAG);
	uint8_t b7 = (cpu.A >> 7);

	cpu.A = (cpu.A << 1) | c;
	
	CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	(b7 == 1) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);;
	
	return 4;
}

//RRA
static int op_1F(){
	uint8_t c = CPU_Get_Flag(C_FLAG);
	uint8_t b0 = (cpu.A & 0b00000001);
	cpu.A = (cpu.A >> 1) | (c << 7);
	
	CPU_Clear_Flag(Z_FLAG);
	CPU_Clear_Flag(N_FLAG);
	CPU_Clear_Flag(H_FLAG);
	(b0 == 1) ? CPU_Set_Flag(C_FLAG) : CPU_Clear_Flag(C_FLAG);;
	
	return 4;
}

//CB PREFIX OPCODES IMPLEMENTATION
static int CB_Execute(){
	uint8_t opcode = Memory_Read_Byte(cpu.PC++);

	uint8_t type = (opcode >> 6) & 0b00000011;
	uint8_t id = (opcode >> 3) & 0b00000111;
	uint8_t reg_id = (opcode & 0b00000111);

	uint8_t *reg;
	if(reg_id == 6){
		return handle_cb_hl(opcode, type, id);
	}

	switch (reg_id)
	{
	    case 0: reg = &cpu.B;
        case 1: reg = &cpu.C;
        case 2: reg = &cpu.D;
        case 3: reg = &cpu.E;
        case 4: reg = &cpu.H;
        case 5: reg = &cpu.L;
        case 7: reg = &cpu.A;
        default: reg = NULL;
	}
 	
	switch(type){
        case 0:  // Rotates/Shifts (0x00-0x3F)
            return handle_rotate_shift(opcode, reg);
        
        case 1:  // BIT (0x40-0x7F)
            return handle_bit(bit_num, reg);
        
        case 2:  // RES (0x80-0xBF)
            return handle_res(bit_num, reg);
        
        case 3:  // SET (0xC0-0xFF)
            return handle_set(bit_num, reg);
    }
    return 0;
	}

static int op_undefined(){

	printf("Undefined OpCode\n");
	return 0;
}


const opcode_func opcode_func_table[256] = {
    [0x00 ... 0xFF] = op_undefined,
    [0x00] = op_00, // NOP

    // --- 8-bit Arithmetic/Logic (Reg) ---
    [0x80]=op_80, [0x81]=op_81, [0x82]=op_82, [0x83]=op_83, [0x84]=op_84, [0x85]=op_85, [0x86]=op_86, [0x87]=op_87,
    [0x88]=op_88, [0x89]=op_89, [0x8A]=op_8A, [0x8B]=op_8B, [0x8C]=op_8C, [0x8D]=op_8D, [0x8E]=op_8E, [0x8F]=op_8F,
    [0x90]=op_90, [0x91]=op_91, [0x92]=op_92, [0x93]=op_93, [0x94]=op_94, [0x95]=op_95, [0x96]=op_96, [0x97]=op_97,
    [0x98]=op_98, [0x99]=op_99, [0x9A]=op_9A, [0x9B]=op_9B, [0x9C]=op_9C, [0x9D]=op_9D, [0x9E]=op_9E, [0x9F]=op_9F,
    [0xA0]=op_A0, [0xA1]=op_A1, [0xA2]=op_A2, [0xA3]=op_A3, [0xA4]=op_A4, [0xA5]=op_A5, [0xA6]=op_A6, [0xA7]=op_A7,
    [0xA8]=op_A8, [0xA9]=op_A9, [0xAA]=op_AA, [0xAB]=op_AB, [0xAC]=op_AC, [0xAD]=op_AD, [0xAE]=op_AE, [0xAF]=op_AF,
    [0xB0]=op_B0, [0xB1]=op_B1, [0xB2]=op_B2, [0xB3]=op_B3, [0xB4]=op_B4, [0xB5]=op_B5, [0xB6]=op_B6, [0xB7]=op_B7,
    [0xB8]=op_B8, [0xB9]=op_B9, [0xBA]=op_BA, [0xBB]=op_BB, [0xBC]=op_BC, [0xBD]=op_BD, [0xBE]=op_BE, [0xBF]=op_BF,

    // --- 8-bit Arithmetic/Logic (Immediate) ---
    [0xC6]=op_C6, [0xCE]=op_CE, [0xD6]=op_D6, [0xDE]=op_DE, [0xE6]=op_E6, [0xEE]=op_EE, [0xF6]=op_F6, [0xFE]=op_FE,

    // --- INC/DEC ---
    [0x04]=op_04, [0x0C]=op_0C, [0x14]=op_14, [0x1C]=op_1C, [0x24]=op_24, [0x2C]=op_2C, [0x34]=op_34, [0x3C]=op_3C,
    [0x05]=op_05, [0x0D]=op_0D, [0x15]=op_15, [0x1D]=op_1D, [0x25]=op_25, [0x2D]=op_2D, [0x35]=op_35, [0x3D]=op_3D,

    // --- 16-bit Arithmetic ---
    [0x09]=op_09, [0x19]=op_19, [0x29]=op_29, [0x39]=op_39, [0xE8]=op_E8,
    [0x03]=op_03, [0x13]=op_13, [0x23]=op_23, [0x33]=op_33,
    [0x0B]=op_0B, [0x1B]=op_1B, [0x2B]=op_2B, [0x3B]=op_3B,

    // --- PUSH/POP ---
    [0xC5]=op_C5, [0xD5]=op_D5, [0xE5]=op_E5, [0xF5]=op_F5,
    [0xC1]=op_C1, [0xD1]=op_D1, [0xE1]=op_E1, [0xF1]=op_F1,

    // --- Accumulator Rotates ---
    [0x07]=op_07, [0x0F]=op_0F, [0x17]=op_17, [0x1F]=op_1F,

    // --- Miscellaneous ---
    [0x27]=op_27, [0x2F]=op_2F, [0x37]=op_37, [0x3F]=op_3F,
    [0x10]=op_10, [0x76]=op_76, [0xFB]=op_FB, [0xF3]=op_F3,

    // --- Jumps ---
    [0xC3]=op_C3, [0xC2]=op_C2, [0xCA]=op_CA, [0xD2]=op_D2, [0xDA]=op_DA, [0xE9]=op_E9,
    [0x18]=op_18, [0x20]=op_20, [0x28]=op_28, [0x30]=op_30, [0x38]=op_38,

    // --- Calls/Returns/RST ---
    [0xCD]=op_CD, [0xC4]=op_C4, [0xCC]=op_CC, [0xD4]=op_D4, [0xDC]=op_DC,
    [0xC9]=op_C9, [0xC0]=op_C0, [0xC8]=op_C8, [0xD0]=op_D0, [0xD8]=op_D8,
    [0xD9]=op_D9,
    [0xC7]=op_C7, [0xCF]=op_CF, [0xD7]=op_D7, [0xDF]=op_DF,
    [0xE7]=op_E7, [0xEF]=op_EF, [0xF7]=op_F7, [0xFF]=op_FF,

    // --- Loads & High RAM ---
    [0x01]=op_01, [0x11]=op_11, [0x21]=op_21, [0x31]=op_31,
    [0x06]=op_06, [0x0E]=op_0E, [0x16]=op_16, [0x1E]=op_1E, [0x26]=op_26, [0x2E]=op_2E, [0x3E]=op_3E,
    [0x22]=op_22, [0x32]=op_32, [0x2A]=op_2A, [0x3A]=op_3A, [0x36]=op_36,
    [0xEA]=op_EA, [0xFA]=op_FA, [0x08]=op_08, [0xF9]=op_F9, [0xF8]=op_F8,
    [0x02]=op_02, [0x12]=op_12, [0x0A]=op_0A, [0x1A]=op_1A,
    [0xE0]=op_E0, [0xF0]=op_F0, [0xE2]=op_E2, [0xF2]=op_F2,
    
    // --- LD r, r Block ---
    [0x40]=op_40, [0x41]=op_41, [0x42]=op_42, [0x43]=op_43, [0x44]=op_44, [0x45]=op_45, [0x46]=op_46, [0x47]=op_47,
    [0x48]=op_48, [0x49]=op_49, [0x4A]=op_4A, [0x4B]=op_4B, [0x4C]=op_4C, [0x4D]=op_4D, [0x4E]=op_4E, [0x4F]=op_4F,
    [0x50]=op_50, [0x51]=op_51, [0x52]=op_52, [0x53]=op_53, [0x54]=op_54, [0x55]=op_55, [0x56]=op_56, [0x57]=op_57,
    [0x58]=op_58, [0x59]=op_59, [0x5A]=op_5A, [0x5B]=op_5B, [0x5C]=op_5C, [0x5D]=op_5D, [0x5E]=op_5E, [0x5F]=op_5F,
    [0x60]=op_60, [0x61]=op_61, [0x62]=op_62, [0x63]=op_63, [0x64]=op_64, [0x65]=op_65, [0x66]=op_66, [0x67]=op_67,
    [0x68]=op_68, [0x69]=op_69, [0x6A]=op_6A, [0x6B]=op_6B, [0x6C]=op_6C, [0x6D]=op_6D, [0x6E]=op_6E, [0x6F]=op_6F,
    [0x70]=op_70, [0x71]=op_71, [0x72]=op_72, [0x73]=op_73, [0x74]=op_74, [0x75]=op_75, [0x76]=op_76, [0x77]=op_77,
    [0x78]=op_78, [0x79]=op_79, [0x7A]=op_7A, [0x7B]=op_7B, [0x7C]=op_7C, [0x7D]=op_7D, [0x7E]=op_7E, [0x7F]=op_7F,
	// --- CB PREFIX ---
	[0xCB]=CB_Execute
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


// Some Helpers

void CPU_Set_Flag(uint8_t flag){

	cpu.F |= flag;

}

void CPU_Clear_Flag(uint8_t flag){

	cpu.F &= ~flag;

}

uint8_t CPU_Get_Flag(uint8_t flag){

	return (cpu.F & flag) ? 1 : 0;

}

void CPU_Flip_Flag(uint8_t flag){ 
	
	cpu.F ^= flag;

}

static uint16_t Combine_Registers(uint8_t reg1,uint8_t reg2){
	return ((reg1 << 8) | reg2);
}


uint16_t Read_nn_From_Rom(){
	uint8_t lower = Memory_Read_Byte(cpu.PC++);
	uint8_t higher = Memory_Read_Byte(cpu.PC++);
	return Combine_Registers(higher, lower);

}

