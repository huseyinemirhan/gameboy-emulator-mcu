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

MEMORY memory = {0};

void Memory_Init(){


}



uint8_t Memory_Read_Byte(uint16_t addr){

	if (addr >= ERAM_START && addr <= ERAM_END)  addr -= 0x2000; // Echo RAM
	       
	if(addr >= FIXED_ROM_START && addr <= FIXED_ROM_END){ // Bank 0

		return memory.cartridge.rom_bank0[addr - FIXED_ROM_START]; //If using SD card add reading here
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

	if (addr >= ERAM_START && addr <= ERAM_END) addr -= 0x2000; //Echo RAM


	if(addr >= FIXED_ROM_START && addr <= ROM_BANK_END){ // MBC  commands here
		Cartridge_Handle_MBC_Command(&memory.cartridge, addr, val);
	}

	else if(addr >= VRAM_START && addr <= VRAM_END){

		memory.vram[addr-VRAM_START] = val;
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

		memory.io_reg[addr-IO_REG_START] = val;
	}
	else if(addr >= HRAM_START && addr <= HRAM_END){

		memory.hram[addr-HRAM_START] = val;
	}
	else{
		memory.ie = val;
	}

}



