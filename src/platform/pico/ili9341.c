#include "ili9341.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/uart.h"

int dma_channel = -1;


static void ILI9341_DMA_Init() {
    dma_channel = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(ILI9341_SPI_PORT, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    dma_channel_configure(dma_channel, &c,
        &spi_get_hw(ILI9341_SPI_PORT)->dr,  
        NULL,                                 
        0,                                    
        false                                 
    );
}

void ILI9341_Select() {
    gpio_put(ILI9341_PIN_CS, 0);
}

void ILI9341_Unselect() {
    gpio_put(ILI9341_PIN_CS, 1);
}

static void ILI9341_Reset() {
    gpio_put(ILI9341_PIN_RST, 1);
    sleep_ms(5);
    gpio_put(ILI9341_PIN_RST, 0);
    sleep_ms(20);
    gpio_put(ILI9341_PIN_RST, 1);
    // The controller ignores commands for a while after reset is released;
    // without this the software reset below is dropped on the floor.
    sleep_ms(150);
}

static void ILI9341_WriteCommand(uint8_t cmd) {
    gpio_put(ILI9341_PIN_DC, 0);
    spi_write_blocking(ILI9341_SPI_PORT, &cmd, 1);
}

static void ILI9341_WriteData(const uint8_t* buff, size_t buff_size) {
    gpio_put(ILI9341_PIN_DC, 1);
    spi_write_blocking(ILI9341_SPI_PORT, buff, buff_size);
}

void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    ILI9341_WriteCommand(0x2A);
    uint8_t data[] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    ILI9341_WriteData(data, sizeof(data));

    ILI9341_WriteCommand(0x2B);
    uint8_t data2[] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };
    ILI9341_WriteData(data2, sizeof(data2));

    ILI9341_WriteCommand(0x2C);
}

void ILI9341_Init(void) {
#if TFT_PANEL == TFT_PANEL_NX4832T035_011
    ILI9341_InitNextion();
    return;
#else
    spi_init(ILI9341_SPI_PORT, ILI9341_SPI_BAUD);
    spi_set_format(ILI9341_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // The requested rate is rounded down to something the PL022 can divide to,
    // so print what we actually got rather than assuming. See ILI9341_SPI_BAUD.
    printf("ILI9341: requested %u Hz, actual %u Hz\n",
           (unsigned)ILI9341_SPI_BAUD, spi_get_baudrate(ILI9341_SPI_PORT));
    
    gpio_set_function(ILI9341_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(ILI9341_PIN_CS);  gpio_set_dir(ILI9341_PIN_CS,  GPIO_OUT);
    gpio_init(ILI9341_PIN_DC);  gpio_set_dir(ILI9341_PIN_DC,  GPIO_OUT);
    gpio_init(ILI9341_PIN_RST); gpio_set_dir(ILI9341_PIN_RST, GPIO_OUT);

    ILI9341_Select();
    ILI9341_Reset();

    ILI9341_WriteCommand(0x01); sleep_ms(100);
    ILI9341_WriteCommand(0xCB); { uint8_t d[] = {0x39,0x2C,0x00,0x34,0x02}; ILI9341_WriteData(d,5); }
    ILI9341_WriteCommand(0xCF); { uint8_t d[] = {0x00,0xC1,0x30}; ILI9341_WriteData(d,3); }
    ILI9341_WriteCommand(0xE8); { uint8_t d[] = {0x85,0x00,0x78}; ILI9341_WriteData(d,3); }
    ILI9341_WriteCommand(0xEA); { uint8_t d[] = {0x00,0x00}; ILI9341_WriteData(d,2); }
    ILI9341_WriteCommand(0xED); { uint8_t d[] = {0x64,0x03,0x12,0x81}; ILI9341_WriteData(d,4); }
    ILI9341_WriteCommand(0xF7); { uint8_t d[] = {0x20}; ILI9341_WriteData(d,1); }
    ILI9341_WriteCommand(0xC0); { uint8_t d[] = {0x23}; ILI9341_WriteData(d,1); }
    ILI9341_WriteCommand(0xC1); { uint8_t d[] = {0x10}; ILI9341_WriteData(d,1); }
    ILI9341_WriteCommand(0xC5); { uint8_t d[] = {0x3E,0x28}; ILI9341_WriteData(d,2); }
    ILI9341_WriteCommand(0xC7); { uint8_t d[] = {0x86}; ILI9341_WriteData(d,1); }
    ILI9341_WriteCommand(0x36); { uint8_t d[] = { (uint8_t)ILI9341_ROTATION }; ILI9341_WriteData(d,1); }
    ILI9341_WriteCommand(0x3A); { uint8_t d[] = {0x55}; ILI9341_WriteData(d,1); }
    ILI9341_WriteCommand(0xB1); { uint8_t d[] = {0x00,0x18}; ILI9341_WriteData(d,2); }
    ILI9341_WriteCommand(0xB6); { uint8_t d[] = {0x08,0x82,0x27}; ILI9341_WriteData(d,3); }
    ILI9341_WriteCommand(0xF2); { uint8_t d[] = {0x00}; ILI9341_WriteData(d,1); }
    ILI9341_WriteCommand(0x26); { uint8_t d[] = {0x01}; ILI9341_WriteData(d,1); }
    ILI9341_WriteCommand(0xE0); { uint8_t d[] = {0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00}; ILI9341_WriteData(d,15); }
    ILI9341_WriteCommand(0xE1); { uint8_t d[] = {0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F}; ILI9341_WriteData(d,15); }
    ILI9341_WriteCommand(0x11); sleep_ms(120); 
    ILI9341_WriteCommand(0x29); sleep_ms(50);               
    ILI9341_DMA_Init();
    ILI9341_Unselect();
#endif
}

void ILI9341_WaitDMA(void) {
#if TFT_PANEL == TFT_PANEL_NX4832T035_011
    return;
#else
    if (dma_channel >= 0) {
        dma_channel_wait_for_finish_blocking(dma_channel);
        while (spi_get_hw(ILI9341_SPI_PORT)->sr & SPI_SSPSR_BSY_BITS) {
            tight_loop_contents();
        }
    }
    
    ILI9341_WriteCommand(0x00);
    
    ILI9341_Unselect();
#endif
}

void ILI9341_StartFrameDMA(const void* framebuffer, size_t num_bytes) {
#if TFT_PANEL == TFT_PANEL_NX4832T035_011
    (void)framebuffer;
    (void)num_bytes;
    return;
#else
    ILI9341_Select();
    ILI9341_SetAddressWindow(0, 0, ILI9341_WIDTH - 1, ILI9341_HEIGHT - 1);
    gpio_put(ILI9341_PIN_DC, 1);
    ILI9341_StartDMATransfer(framebuffer, num_bytes);
#endif
}

void ILI9341_StartDMATransfer(const void* framebuffer, size_t num_bytes) {
#if TFT_PANEL == TFT_PANEL_NX4832T035_011
    (void)framebuffer;
    (void)num_bytes;
    return;
#else
    dma_channel_set_read_addr(dma_channel, framebuffer, false);
    dma_channel_set_trans_count(dma_channel, num_bytes, true);
#endif
}

void ILI9341_FillScreen(uint16_t color) {
#if TFT_PANEL == TFT_PANEL_NX4832T035_011
    (void)color;
    ILI9341_InitNextion();
    return;
#else
    ILI9341_WaitDMA();  
    ILI9341_Select();
    ILI9341_SetAddressWindow(0, 0, ILI9341_WIDTH-1, ILI9341_HEIGHT-1);
    gpio_put(ILI9341_PIN_DC, 1);
    uint8_t bytes[2] = { color >> 8, color & 0xFF };
    for (int i = 0; i < ILI9341_WIDTH * ILI9341_HEIGHT; i++) {
        spi_write_blocking(ILI9341_SPI_PORT, bytes, 2);
    }
    ILI9341_Unselect();
#endif
}