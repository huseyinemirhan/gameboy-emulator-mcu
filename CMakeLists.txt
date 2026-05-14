cmake_minimum_required(VERSION 4.2)
project(gb_emulator_core C)

set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED)

include_directories(src/cpu)
include_directories(src/memory)
include_directories(src/ppu)

add_executable(gb_emulator_core
        src/cpu/cpu.c
        src/cpu/cpu.h
        src/memory/cartridge.c
        src/memory/cartridge.h
        src/memory/memory.c
        src/memory/memory.h
        src/ppu/ppu.c
        src/ppu/ppu.h
        src/main.c
        src/platform/windows/inputs.c
        src/platform/windows/inputs.h
        src/platform/windows/debug.c
        src/platform/windows/debug.h
        src/platform/stm32f103/memory_commands.c
        src/platform/stm32f103/memory_commands.h)

target_link_libraries(gb_emulator_core SDL2::SDL2)
