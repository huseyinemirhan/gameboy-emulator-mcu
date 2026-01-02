#include "sd_card.h"

const uint8_t rom_data[] = { 0

};

uint8_t sd_card_read(uint32_t addr){

    return rom_data[addr];

}
