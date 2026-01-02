#ifndef SD_CARD_H_
#define SD_CARD_H_

#include <stdint.h>

extern const uint8_t rom_data[];



uint8_t sd_card_read(uint32_t addr);




#endif /* SD_CARD_H_ */
