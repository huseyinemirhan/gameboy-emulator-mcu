/*
 * memory.c
 *
 *  Created on: Dec 30, 2025
 *      Author: MSI Cyborg
 */

#include "memory.h"
#include "cartridge.h"
#include <stdio.h>

#include "../cpu/cpu.h"
#include "../ppu/ppu.h"

MEMORY memory = {0};


void Memory_Init(){
	// Post-boot IO register values
	// These are the values the boot ROM leaves behind before jumping to 0x0100
	// PPU registers - CRITICAL for rendering
	memory.boot_rom_en = 1;
	memory.buttons_state = 0x0F;
	memory.d_pad_state = 0x0F;
	memory.io_reg[0x40] = 0x91;  // LCDC: LCD on, BG on, tile data at 0x8000
	memory.io_reg[0x41] = 0x85;  // STAT
	memory.io_reg[0x42] = 0x00;  // SCY
	memory.io_reg[0x43] = 0x00;  // SCX
	memory.io_reg[0x44] = 0x00;  // LY (current scanline)
	memory.io_reg[0x45] = 0x00;  // LYC
	memory.io_reg[0x47] = 0xFC;  // BGP: palette 3=black, 2=dark, 1=light, 0=white
	memory.io_reg[0x48] = 0xFF;  // OBP0
	memory.io_reg[0x49] = 0xFF;  // OBP1
	memory.io_reg[0x4A] = 0x00;  // WY
	memory.io_reg[0x4B] = 0x00;  // WX

	// Joypad
	memory.io_reg[0x00] = 0xFF;  // P1/JOYP
	// Serial
	memory.io_reg[0x01] = 0x00;  // SB
	memory.io_reg[0x02] = 0x7E;  // SC
	// Timer
	memory.io_reg[0x04] = 0xAB;  // DIV
	memory.io_reg[0x05] = 0x00;  // TIMA
	memory.io_reg[0x06] = 0x00;  // TMA
	memory.io_reg[0x07] = 0x00;  // TAC
	// Interrupt flag
	memory.io_reg[0x0F] = 0xE1;  // IF
	// Sound registers (basic init to prevent issues)
	memory.io_reg[0x10] = 0x80;  // NR10
	memory.io_reg[0x11] = 0xBF;  // NR11
	memory.io_reg[0x12] = 0xF3;  // NR12
	memory.io_reg[0x13] = 0xFF;  // NR13
	memory.io_reg[0x14] = 0xBF;  // NR14
	memory.io_reg[0x16] = 0x3F;  // NR21
	memory.io_reg[0x17] = 0x00;  // NR22
	memory.io_reg[0x18] = 0xFF;  // NR23
	memory.io_reg[0x19] = 0xBF;  // NR24
	memory.io_reg[0x1A] = 0x7F;  // NR30
	memory.io_reg[0x1B] = 0xFF;  // NR31
	memory.io_reg[0x1C] = 0x9F;  // NR32
	memory.io_reg[0x1D] = 0xFF;  // NR33
	memory.io_reg[0x1E] = 0xBF;  // NR34
	memory.io_reg[0x20] = 0xFF;  // NR41
	memory.io_reg[0x21] = 0x00;  // NR42
	memory.io_reg[0x22] = 0x00;  // NR43
	memory.io_reg[0x23] = 0xBF;  // NR44
	memory.io_reg[0x24] = 0x77;  // NR50
	memory.io_reg[0x25] = 0xF3;  // NR51
	memory.io_reg[0x26] = 0xF1;

	memory.io_reg[0x4c] = 0xFF;
	memory.io_reg[0x4d] = 0xFF;
}



uint8_t Memory_Read_Byte(uint16_t addr){

	if (addr >= ERAM_START && addr <= ERAM_END)  addr -= 0x2000; // Echo RAM

	if (addr < 0x0100 && memory.boot_rom_en) {
		return memory.boot_rom[addr];
	}
	       
	if(addr >= FIXED_ROM_START && addr <= FIXED_ROM_END){ // Bank 0

		return memory.cartridge.rom_bank0[addr - FIXED_ROM_START];
	}

	else if(addr >= ROM_BANK_START && addr <= ROM_BANK_END){

		return memory.cartridge.cur_rom_bank[addr - ROM_BANK_START]; //If using 4Kb banks add offset managing here
	}

	else if(addr >= VRAM_START && addr <= VRAM_END){

		return memory.vram[addr-VRAM_START];
	}

	else if(addr >= EXT_RAM_START && addr <= EXT_RAM_END){

		if(!memory.cartridge.ram_enabled || memory.cartridge.ext_ram == NULL){
			return 0xFF;
		}
		else{

			uint32_t abs_addr = addr - EXT_RAM_START + (memory.cartridge.ram_bank * 8192);	// 8KB banks

			if(abs_addr < memory.cartridge.ext_ram_size){
				return memory.cartridge.ext_ram[abs_addr];
			}

			return 0xFF;

		}
	}
	else if(addr >= WRAM_START && addr <= WRAM_END){

		return memory.wram[addr-WRAM_START];
	}
	else if(addr >= OAM_START && addr <= OAM_END){

		return memory.oam[addr-OAM_START];
	}
	else if(addr >= IO_REG_START && addr <= IO_REG_END){

		if (addr == 0xFF00) {
			uint8_t result = 0xC0 | memory.io_reg[0];
			if (!(memory.io_reg[0] & 0x10))
				result |= memory.d_pad_state;

			if (!(memory.io_reg[0] & 0x20))
				result |= memory.buttons_state;

			return result;

		}

		return memory.io_reg[addr-IO_REG_START];

	}
	else if(addr >= HRAM_START && addr <= HRAM_END){

		return memory.hram[addr-HRAM_START];
	}
	else{
		return memory.ie;
	}
}

void Memory_Write_Byte(uint16_t addr, uint8_t val){

	if (addr == 0xFF50) {
		memory.boot_rom_en = 0;
		return;
	}

	if (addr < 0x0100 && memory.boot_rom_en) {
		memory.boot_rom[addr] = val;
	}

	if (addr >= ERAM_START && addr <= ERAM_END) addr -= 0x2000; //Echo RAM


	if(addr >= FIXED_ROM_START && addr <= ROM_BANK_END){ // MBC  commands here
		Cartridge_Handle_MBC_Command(&memory.cartridge, addr, val);
	}

	else if(addr >= VRAM_START && addr <= VRAM_END){
		memory.vram[addr - VRAM_START] = val;
	}

	else if(addr >= EXT_RAM_START && addr <= EXT_RAM_END){

		if (!memory.cartridge.ram_enabled || !memory.cartridge.ext_ram) {
        	return;
    	}

    	uint32_t abs_addr = (memory.cartridge.ram_bank * 8192) + addr - EXT_RAM_START;

    	if (abs_addr < memory.cartridge.ext_ram_size) {

        	memory.cartridge.ext_ram[abs_addr] = val;

    	}
	}
	else if(addr >= WRAM_START && addr <= WRAM_END){

		memory.wram[addr-WRAM_START] = val;
	}
	else if(addr >= OAM_START && addr <= OAM_END){

		memory.oam[addr-OAM_START] = val;;
	}
	else if(addr >= IO_REG_START && addr <= IO_REG_END){

		if(addr == 0xFF04){ //DIV
			memory.io_reg[0x04] = 0;
			cpu.div_counter = 0;
			return;
		}

		if (addr == 0xFF46) { // OAM Transfer
				memory.io_reg[0x46] = val;
				uint16_t source_addr = val << 8;
				for (int i = 0; i < 160; i++) {
					memory.oam[i] = Memory_Read_Byte(source_addr + i);
				}
				return;
			}
		if (addr == 0xFF00) {
			memory.io_reg[0] = val & 0x30;
			return;
		}

			memory.io_reg[addr-IO_REG_START] = val;
		}
	else if(addr >= HRAM_START && addr <= HRAM_END){

		memory.hram[addr-HRAM_START] = val;
	}
	else{
		memory.ie = val;
	}

}



