/* STM32F103C8Tx_FLASH.ld - Modified for Game Boy Emulator */

/* Entry Point */
ENTRY(Reset_Handler)

/* Memory layout */
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

/* Memory areas - STM32F103C8T6 Blue Pill */
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000, LENGTH = 20K
  FLASH (rx)   : ORIGIN = 0x08000000, LENGTH = 64K
}

SECTIONS
{
  /* Interrupt vector table */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* Program code */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* Read-only data - THIS IS WHERE YOUR ROM GOES */
  .rodata :
  {
    . = ALIGN(4);
    
    /* Game Boy boot ROM (optional, 256 bytes) */
    _boot_rom_start = .;
    *(.boot_rom)
    _boot_rom_end = .;
    
    /* Game Boy game ROM (32KB for Tetris) */
    _game_rom_start = .;
    *(.rodata)         /* tetris_rom goes here */
    *(.rodata*)
    _game_rom_end = .;
    
    . = ALIGN(4);
  } >FLASH

  /* Save data section (for Phase 2 - Flash saves) */
  /* Uncomment when you implement Flash saves */
  /*
  .save_data :
  {
    . = ALIGN(1024);   /* Align to page boundary */
    _save_data_start = .;
    . = . + 32K;       /* Reserve 32KB for saves */
    _save_data_end = .;
  } >FLASH
  */

  /* ARM exception handling */
  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  /* C++ constructors/destructors */
  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* Data initialization */
  _sidata = LOADADDR(.data);

  /* Initialized data */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  /* Zero-initialized data */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* Heap and stack */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}