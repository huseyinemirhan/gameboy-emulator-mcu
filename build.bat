@echo off
echo ========================================
echo Building Game Boy Emulator
echo ========================================

gcc -g -Wall -Isrc ^
    src/main.c ^
    src/cpu/cpu.c ^
    src/memory/memory.c ^
    src/memory/cartridge.c ^
    -o gb_emulator.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Run with: gb_emulator.exe your_rom.gb
) else (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
)

pause