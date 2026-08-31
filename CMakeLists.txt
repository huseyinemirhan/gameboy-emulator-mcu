# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.20)
include(pico_sdk_import.cmake)
project(gb_emulator_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
pico_sdk_init()

add_executable(gb_emulator_pico
    src/cpu/cpu.c
    src/memory/cartridge.c
    src/memory/memory.c
    src/ppu/ppu.c
    src/main.c
    src/platform/pico/ili9341.c
    src/platform/pico/tft_screen.c
    src/platform/pico/inputs.c
    src/platform/pico/agent_link.c
    rom.s
)

target_include_directories(gb_emulator_pico PRIVATE
    src
    src/cpu
    src/memory
    src/ppu
    src/platform/pico
)

target_link_libraries(gb_emulator_pico
    pico_stdlib
    pico_multicore
    hardware_spi
    hardware_gpio
    hardware_adc
    hardware_dma
)

# Diagnostics: expose stdio over USB CDC as well as UART0 so the board can
# be read without wiring up a UART adapter.
pico_enable_stdio_usb(gb_emulator_pico 1)
# UART mirror off: it runs at a real 115200 baud, so every diagnostic line
# blocked the emulator for tens of ms. USB CDC has no such limit.
# Set back to 1 if you want output on GP0/GP1.
pico_enable_stdio_uart(gb_emulator_pico 0)

pico_add_extra_outputs(gb_emulator_pico)