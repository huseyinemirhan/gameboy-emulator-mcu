# Nintendo Game Boy Emulator on STM32F103 & Raspberry Pi Pico 2

A hardware-based Nintendo Game Boy (DMG-01) emulator targeting resource-constrained microcontrollers, starting with the STM32F103 (Blue Pill) and evolving to the Raspberry Pi Pico 2.

![Game Boy DMG-01](https://upload.wikimedia.org/wikipedia/commons/f/f4/Game-Boy-FL.jpg)

*Original Nintendo Game Boy (DMG-01)*

### Current Status

**Work in Progress** - This emulator is under active development

## Version Roadmap

### Version 1: STM32F103 (Blue Pill) - Basic Emulation/Testing

**Status:** In Development...

![STM32F103 Blue Pill](https://stm32-base.org/assets/img/boards/STM32F103C8T6_Blue_Pill-1.jpg)

*STM32F103C8T6 Blue Pill Development Board*

**Features:**
- Core CPU emulation (Sharp LR35902)
- Basic PPU (Pixel Processing Unit) implementation
- Simple memory management
- Works with games that fit in internal memory

**Limitations:**
- No external RAM support
- No ROM banking (MBC1, MBC3, etc.)
- Limited to small, simple games (Tetris, Dr. Mario, etc.)

### Version 2: STM32F103 with SD Card - Enhanced Compatibility

**Status:** Planned

**Features:**
- SD card integration for external ROM storage
- External RAM support via SD card(Should be tested further)
- Memory Bank Controller (MBC) support
- Significantly expanded game compatibility

**Challenges:**
- SD card I/O performance optimization
- Real-time RAM persistence

### Version 3: Raspberry Pi Pico 2 Port - Full-Featured Emulator

**Status:** Planned

![Raspberry Pi Pico 2](https://www.raspberrypi.com/products/raspberry-pi-pico-2/)

*Raspberry Pi Pico 2 Board*

**Features:**
- Port of Version 2 to RP2350 architecture
- Improved performance with dual ARM Cortex-M33 cores
- Better memory management (520KB SRAM vs 20KB on STM32)
- Enhanced peripheral support
- Goal: Comprehensive Game Boy game compatibility

**Migration Focus:**
- Peripheral communication layer adaptation
- Leveraging additional CPU power and memory
- Potential for advanced features (save states, fast-forward)

## Hardware Requirements

### Version 1 & 2
- **Microcontroller:** STM32F103C8T6 (Blue Pill)
- **Display:** Compatible LCD/OLED display (TBD)
- **Storage:** SD card module (Version 2)
- **Input:** Button interface for Game Boy controls
- **Power:** 3.3V regulated supply

### Version 3
- **Microcontroller:** Raspberry Pi Pico 2 (RP2350)
- **Display:** Compatible LCD/OLED display
- **Storage:** SD card module
- **Input:** Button interface for Game Boy controls
- **Power:** 3.3V via USB or battery

## Development Tools

- **IDE:** STM32CubeIDE / Raspberry Pi Pico SDK-VS Code
- **Toolchain:** ARM GCC 
- **Language:** C

## Technical Specifications

### Game Boy DMG-01 Hardware
- **CPU:** 8-bit Sharp LR35902 (similar to Z80) @ 4.19 MHz
- **RAM:** 8KB work RAM, 8KB video RAM
- **Display:** 160×144 pixels, 4 shades of gray
- **Audio:** 4 channels (2 square waves, 1 programmable wave, 1 noise)
- **Cartridge:** Various MBC types with ROM/RAM banking

### Emulation Components
- CPU instruction cycle emulation
- Memory management and banking
- Pixel Processing Unit (PPU)
- Timer and interrupt handling
- Input handling
- Audio processing (future)

## Project Structure

```
Nintendo-Game-Boy-Emulation-On-STM32F103/
├── src/                    # Source code files
│   ├── cpu/               # CPU emulation
│   ├── ppu/               # Graphics processing
│   ├── memory/            # Memory management
│   ├── peripherals/       # Hardware interfaces
│   └── main.c             # Main program
├── gb_emulator.exe        # Test emulator (desktop)
└── README.md
```

## Hardware Comparison

| Feature | STM32F103C8T6 | Raspberry Pi Pico 2 |
|---------|---------------|---------------------|
| CPU Core | ARM Cortex-M3 @ 72 MHz | Dual ARM Cortex-M33 @ 150 MHz |
| RAM | 20 KB | 520 KB |
| Flash | 64 KB | 4 MB on-board |
| Architecture | Single-core | Dual-core |
| Additional Features | - | RISC-V option, TrustZone security |

## Resources & References

### Game Boy Technical Documentation
- [Pan Docs](https://gbdev.io/pandocs/)  
- [Game Boy: Complete Technical Reference](https://gekkio.fi/files/gb-docs/gbctr.pdf) 
- [gbdev.io](https://gbdev.io/) 

### Hardware Documentation
- [STM32F103C8T6 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [Raspberry Pi Pico 2 Datasheet](https://datasheets.raspberrypi.com/pico/pico-2-datasheet.pdf)
- [RP2350 Datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)

## Author

**Hüseyin Emirhan**
- GitHub: [@huseyinemirhan](https://github.com/huseyinemirhan)


**Note:** This emulator is a work in progress. Features and compatibility will "hopefully" improve with each version release.
