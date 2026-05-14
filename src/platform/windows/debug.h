//
// Created by MSI Cyborg on 14.05.2026.
//

#ifndef GB_EMULATOR_CORE_DEBUG_H
#define GB_EMULATOR_CORE_DEBUG_H

#include <stdint.h>
#include "../memory/memory.h"

void cpu_debug();
void ppu_debug();
void print_debug(char *str);
void print_cartridge_debug(Cartridge *cart);

#endif //GB_EMULATOR_CORE_DEBUG_H
