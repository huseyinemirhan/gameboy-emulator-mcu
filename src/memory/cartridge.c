#include "cartridge.h"
#include "memory.h"
#include "sd_card.h"
#include <string.h>
#include <stdlib.h>
#include<stdint.h>


void Cartridge_Init(Cartridge *cart, const uint8_t *rom_header){
	memset(cart,0,sizeof(cart));

	switch (rom_header[CARTRIDGE_TYPE_ADDR])
	{
		case 0x00: cart->mbc_type = NO_MBC; break;

        case 0x01: case 0x02: case 0x03: 
            cart->mbc_type = MBC1; 
            break;
        case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
            cart->mbc_type = MBC3;
            break;
        case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
            cart->mbc_type = MBC5;
            break;
        default:
            cart->mbc_type = NO_MBC;  
    }
	cart->rom_bank0 = rom_header;
	cart->cur_rom_bank = rom_header + FIXED_ROM_END+1;

	cart->rom_size = 32768 << rom_header[ROM_SIZE_ADDR] ;
	cart->rom_bank_count = cart->rom_size / 16384; 

	switch (rom_header[RAM_SIZE_ADDR])
	{
	case 0x00: cart->ext_ram_size = 0; break;
	//case 0x01: cart->ext_ram_size = 2048; break; //UNUSED IN GB
	case 0x02: cart->ext_ram_size = 8192; break;
	case 0x03: cart->ext_ram_size = 32768; break;
	case 0x04: cart->ext_ram_size = 131072; break;
	case 0x05: cart->ext_ram_size = 65536; break;
	default: cart->ext_ram_size = 0; break;
	}

	if(cart->ext_ram_size > 0){
		//cart->ext_ram = (uint8_t *)malloc(cart->ext_ram_size); NO SPACE read from sd
		//memset(cart->ext_ram, 0x00 , cart->ext_ram_size);
	}
	else{
		cart->ext_ram = NULL;
	}
	cart -> rom_bank0 = rom_header;
	cart -> cur_rom_bank = &rom_header[16384]; //point to bank 1 initially
	cart-> rom_bank = 1;
	cart-> ram_bank = 0;
	cart-> ram_enabled = 0;
	cart-> banking_mode = 0;
	#ifdef DEBUG
    printf("=== Cartridge Info ===\n");
    printf("Title: ");
    for (int i = TITLE_START_ADDR; i <= TITLE_END_ADDR && rom_data[i]; i++) {
        printf("%c", rom_data[i]);
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
    printf("CGB Flag: 0x%02X\n", rom_data[CGB_FLAG_ADDR]);
    printf("======================\n");
    #endif
}
	

void Cartridge_Handle_MBC_Command(Cartridge *cart, uint16_t addr, uint8_t val){

	switch (cart->mbc_type)
	{
	case NO_MBC:
		break;

	case MBC1:
		if(addr >= MBC1_RAM_ENABLE_START && addr <= MBC1_RAM_ENABLE_END){

            cart->ram_enabled = (0b00001111 & val) == 0x0A ? 1:0;
        }

        else if( addr <= MBC1_ROM_BANK_NUM_START && addr <= MBC1_ROM_BANK_NUM_END){ // Which rom bank to use

            uint8_t bank = val & 0b00011111;

            cart->rom_bank = (cart->rom_bank & 0b01100000) | bank; 
        
            if ((cart->rom_bank & 0x1F) == 0) {
            cart->rom_bank |= 0x01;
            }

            cart->rom_bank %= cart->rom_bank_count;
            Switch_rom_bank(cart, cart->rom_bank);

        }

		else if( addr >= MBC1_RAM_BANK_NUM_START && addr <= MBC1_RAM_BANK_NUM_END){ // which ram bank to use

			if(cart->banking_mode == 0){
				//ROM Banking Mode

				uint8_t upper_bits = val & 0b00000011;

				cart->rom_bank = (cart->rom_bank & 0b00011111) | (upper_bits << 5); 
				
				cart->rom_bank %= cart->rom_bank_count;
				Switch_rom_bank(cart, cart->rom_bank);
				
			}
			else{
				//RAM Banking Mode
				cart->ram_bank = val & 0b00000011;
			}
		}
		else if( addr >= MBC1_BANKING_MODE_START && addr <= MBC1_BANKING_MODE_END){

			cart->banking_mode = val & 0x01;
		}
        
		break;
	case MBC3:
		// Handle MBC3 commands here
		break;
	case MBC5:
		// Handle MBC5 commands here
		break;
	default:
		break;
	}

}

void Switch_rom_bank(Cartridge *cart, uint8_t bank_num){

	uint32_t rom_offset = bank_num * 16384; // 16KB
	// add something to emulate reading from sd 
	cart->cur_rom_bank = &cart->rom_bank0[rom_offset];

}
