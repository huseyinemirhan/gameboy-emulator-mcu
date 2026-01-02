#ifndef SRC_MEMORY_MEMORY_H_
#define SRC_MEMORY_MEMORY_H_

#include <stdint.h>
#include "cartridge.h"

struct Cartridge;

#define FIXED_ROM_START     0x0000
#define FIXED_ROM_END       0x3FFF

#define ROM_BANK_START    0x4000
#define ROM_BANK_END      0x7FFF

#define VRAM_START    0x8000
#define VRAM_END      0x9FFF

#define EXT_RAM_START 0xA000
#define EXT_RAM_END   0xBFFF

#define WRAM_START    0xC000
#define WRAM_END      0xDFFF

#define OAM_START     0xFE00
#define OAM_END       0xFE9F

#define IO_REG_START  0xFF00
#define IO_REG_END    0xFF7F

#define HRAM_START    0xFF80
#define HRAM_END      0xFFFE

#define IE_REG_ADDR   0xFFFF

#define	ERAM_START    0xE000
#define ERAM_END      0xFDFF



typedef struct {
    uint8_t vram[VRAM_END - VRAM_START + 1];
    uint8_t wram[WRAM_END - WRAM_START + 1];
    uint8_t oam[OAM_END - OAM_START + 1];
    uint8_t io_reg[IO_REG_END - IO_REG_START + 1];
    uint8_t hram[HRAM_END - HRAM_START + 1];
    uint8_t ie;
    Cartridge cartridge;
} MEMORY;

extern MEMORY memory;


uint8_t Memory_Read_Byte(uint16_t addr);

void Memory_Init();
void Memory_Write_Byte(uint16_t addr, uint8_t byte);
void Memory_Clear_Byte(uint16_t addr);

#endif /* SRC_MEMORY_MEMORY_H_ */
