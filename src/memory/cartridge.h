#ifndef CARTRIDGE_H_
#define CARTRIDGE_H_

#include <stdint.h>

#define FIXED_ROM_START     0x0000
#define FIXED_ROM_END       0x3FFF

#define ROM_BANK_START    0x4000
#define ROM_BANK_END      0x7FFF

#define MBC1_RAM_ENABLE_START 0x0000
#define MBC1_RAM_ENABLE_END   0x1FFF

#define MBC1_ROM_BANK_NUM_START 0x2000
#define MBC1_ROM_BANK_NUM_END   0x3FFF

#define MBC1_RAM_BANK_NUM_START 0x4000
#define MBC1_RAM_BANK_NUM_END   0x5FFF

#define MBC1_BANKING_MODE_START 0x6000
#define MBC1_BANKING_MODE_END   0x7FFF

#define TITLE_START_ADDR       0x0134
#define TITLE_END_ADDR         0x0143

#define CGB_FLAG_ADDR          0x0143

#define CARTRIDGE_TYPE_ADDR    0x0147
#define ROM_SIZE_ADDR          0x0148
#define RAM_SIZE_ADDR          0x0149

#define HEADER_CHECKSUM_ADDR   0x014D
#define GLOBAL_CHECKSUM_ADDR   0x014E //2 bytes



typedef enum{
    NO_MBC,
    MBC1, // default
    MBC2, // is this used anywhere?
    MBC3, // pokemon uses this
    MBC5 // has rumble

}MBC_TYPE;

typedef struct{
    const uint8_t *rom_bank0;
    const uint8_t *cur_rom_bank;

    uint8_t *ext_ram;
    uint32_t ext_ram_size;

    MBC_TYPE mbc_type;
    
    uint8_t rom_bank;
    uint8_t ram_bank;

    uint8_t ram_enabled;
    uint8_t banking_mode;

    uint32_t rom_size;
    uint8_t rom_bank_count;

} Cartridge;


void Cartridge_Handle_MBC_Command(Cartridge *cart, uint16_t addr, uint8_t val);
void Cartridge_Init(Cartridge *cart, const uint8_t *rom_header);
void Switch_rom_bank(Cartridge *cart, uint8_t bank_num);


#endif /* SRC_MEMORY_CARTRIDGE_H_*/
