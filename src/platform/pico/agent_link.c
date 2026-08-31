#include "agent_link.h"

#if AGENT_LINK

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "ili9341.h"
#include "tft_screen.h"
#include "inputs.h"
#include "../../memory/memory.h"
#include "../../cpu/cpu.h"

// Agent-side button bits. Deliberately a flat 8-bit word rather than the two
// half-nibbles the joypad register uses, so a caller can express any chord in
// one byte without caring how the hardware splits them.
#define AGENT_A      0x01
#define AGENT_B      0x02
#define AGENT_SELECT 0x04
#define AGENT_START  0x08
#define AGENT_RIGHT  0x10
#define AGENT_LEFT   0x20
#define AGENT_UP     0x40
#define AGENT_DOWN   0x80

#define CMD_MAX 64
#define READ_MAX 192

static uint8_t  attached = 0;      // while 0, the hardware inputs are untouched
static uint8_t  held = 0;          // buttons held until changed
static uint8_t  tap = 0;           // buttons held for a bounded number of frames
static uint16_t tap_frames = 0;

// Repeated taps, played out here rather than one serial round trip per press.
// Shifting a piece five columns was costing ~1s of host latency, which is
// longer than a piece takes to fall a row once the level climbs.
#define REP_ON  2
#define REP_OFF 3
static uint8_t  rep_mask = 0;
static uint16_t rep_count = 0;
static uint8_t  rep_tick = 0;
static uint32_t frame_counter = 0;

static char    cmd[CMD_MAX];
static uint8_t cmd_len = 0;

static int HexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Parses up to `digits` hex characters starting at *p, advancing it.
static uint32_t ParseHex(const char** p, int digits) {
    uint32_t v = 0;
    int n = 0;
    while (n < digits) {
        const int d = HexVal(**p);
        if (d < 0) break;
        v = (v << 4) | (uint32_t)d;
        (*p)++;
        n++;
    }
    return v;
}

static uint32_t ParseDec(const char** p) {
    uint32_t v = 0;
    while (**p >= '0' && **p <= '9') {
        v = v * 10 + (uint32_t)(**p - '0');
        (*p)++;
    }
    return v;
}

// Reads guest memory through the normal bus so banking and IO mirroring behave
// exactly as the CPU sees them. Read-only: nothing here writes guest state.
static void DumpMemory(uint16_t addr, uint16_t len) {
    if (len > READ_MAX) len = READ_MAX;
    printf("R %04X %u ", addr, (unsigned)len);
    for (uint16_t i = 0; i < len; i++) {
        printf("%02X", Memory_Read_Byte((uint16_t)(addr + i)));
    }
    printf("\n");
}

// One atomic snapshot of everything a Tetris driver needs, so the host isn't
// stitching together four round trips while the piece keeps falling.
//
//   G <18 x 3hex row bitmasks> <8 x XXYY sprite positions> <6 score digits>
//
// Rows are the settled board only (columns 2..11 of the BG map); the active
// piece lives in OAM 4..7 and the next-piece preview in OAM 8..11, because
// Tetris draws the falling piece with sprites rather than background tiles.
static void DumpGame(void) {
    char out[160];
    int n = 0;

    const uint8_t lcdc = memory.io_reg[0x40];
    const uint16_t map = (lcdc & 0x08) ? 0x1C00 : 0x1800;

    out[n++] = 'G'; out[n++] = ' ';

    for (int row = 0; row < 18; row++) {
        uint16_t bits = 0;
        for (int col = 0; col < 10; col++) {
            if (memory.vram[map + row * 32 + (2 + col)] != 0x2F) {
                bits |= (uint16_t)(1u << col);
            }
        }
        n += sprintf(&out[n], "%03X", bits);
    }

    out[n++] = ' ';
    for (int i = 4; i < 12; i++) {
        n += sprintf(&out[n], "%02X%02X", memory.oam[i * 4 + 1], memory.oam[i * 4 + 0]);
    }

    out[n++] = ' ';
    for (int col = 13; col < 19; col++) {
        n += sprintf(&out[n], "%X", memory.vram[map + 3 * 32 + col] & 0x0F);
    }

    // Menu screens also park sprites on the field, so in-bounds cells alone
    // aren't proof we're playing. The word SCORE at row 1 only appears on the
    // gameplay screen, which makes it an unambiguous discriminator.
    static const uint8_t SCORE_TILES[5] = { 0x1C, 0x0C, 0x18, 0x1B, 0x0E };
    uint8_t playing = 1;
    for (int i = 0; i < 5; i++) {
        if (memory.vram[map + 1 * 32 + 14 + i] != SCORE_TILES[i]) {
            playing = 0;
            break;
        }
    }
    n += sprintf(&out[n], " P%u Q%u", (unsigned)playing, (unsigned)rep_count);

    out[n] = 0;
    printf("%s\n", out);
}

// Raw input pins, straight from the hardware, bypassing get_input()'s decoding.
// With nothing touched every button should read 1 (idle, held up by its
// internal pull-up) and each axis should sit near mid-scale (~2048).
// A button stuck at 0 is shorted to ground or on the wrong pin; an axis pinned
// near 0 or 4095 is wired to a rail; an axis drifting anywhere is floating.
static void ReportWiring(void) {
    adc_select_input(0);
    const uint16_t x = adc_read();
    adc_select_input(1);
    const uint16_t y = adc_read();

    printf("W A=%d B=%d START=%d SEL5=%d JOYSW=%d X=%u Y=%u\n",
           gpio_get(2), gpio_get(3), gpio_get(4), gpio_get(5), gpio_get(6),
           (unsigned)x, (unsigned)y);
}

// Everything measurable about the display path from this side of the cable.
// Distinguishes "core 1 never runs" from "core 1 runs but the panel shows
// nothing", which are completely different faults.
static void ReportDisplay(void) {
    printf("X baud=%u w=%d h=%d frames=%lu dma_busy=%d cs=%d dc=%d rst=%d sck=%d\n",
           (unsigned)spi_get_baudrate(ILI9341_SPI_PORT),
           ILI9341_WIDTH, ILI9341_HEIGHT,
           (unsigned long)tft_frames_done,
           dma_channel_is_busy(dma_channel),
           gpio_get(ILI9341_PIN_CS), gpio_get(ILI9341_PIN_DC),
           gpio_get(ILI9341_PIN_RST), gpio_get(ILI9341_PIN_SCK));
}

// Asks the panel to identify itself. This is the only test that gets evidence
// FROM the panel rather than about it: a real ILI9341 answers 0x04 with
// 00 93 41 after a dummy byte, and 0x0A with its power-mode byte.
//
//   all 00  -> nothing driving MISO: panel unpowered, dead, or SDO not wired
//   all FF  -> MISO floating high, same conclusion
//   93 41   -> the panel is alive and talking, so the fault is elsewhere
//
// Reads need a much slower clock than writes, so the baud is dropped for the
// duration and restored afterwards.
static void ReadPanelID(void) {
    const uint32_t saved = spi_get_baudrate(ILI9341_SPI_PORT);

    tft_pause = 1;                       // stop core 1 starting a new transfer
    ILI9341_WaitDMA();                   // let the in-flight one finish
    spi_set_baudrate(ILI9341_SPI_PORT, 2000000);

    uint8_t id[4] = {0}, pwr[2] = {0}, st[5] = {0};

    ILI9341_Select();
    gpio_put(ILI9341_PIN_DC, 0);
    uint8_t c = 0x04;                    // RDDID
    spi_write_blocking(ILI9341_SPI_PORT, &c, 1);
    gpio_put(ILI9341_PIN_DC, 1);
    spi_read_blocking(ILI9341_SPI_PORT, 0x00, id, 4);
    ILI9341_Unselect();

    ILI9341_Select();
    gpio_put(ILI9341_PIN_DC, 0);
    c = 0x0A;                            // RDDPM - power mode
    spi_write_blocking(ILI9341_SPI_PORT, &c, 1);
    gpio_put(ILI9341_PIN_DC, 1);
    spi_read_blocking(ILI9341_SPI_PORT, 0x00, pwr, 2);
    ILI9341_Unselect();

    ILI9341_Select();
    gpio_put(ILI9341_PIN_DC, 0);
    c = 0x09;                            // RDDST - display status
    spi_write_blocking(ILI9341_SPI_PORT, &c, 1);
    gpio_put(ILI9341_PIN_DC, 1);
    spi_read_blocking(ILI9341_SPI_PORT, 0x00, st, 5);
    ILI9341_Unselect();

    spi_set_baudrate(ILI9341_SPI_PORT, saved);
    tft_pause = 0;

    // Is SDO even connected? Drive the pin weakly both ways: a floating line
    // follows the pull (1 then 0) and proves nothing is wired to it, while a
    // line something else is holding stays put regardless.
    gpio_set_function(ILI9341_PIN_MISO, GPIO_FUNC_SIO);
    gpio_set_dir(ILI9341_PIN_MISO, GPIO_IN);
    gpio_pull_up(ILI9341_PIN_MISO);
    sleep_us(200);
    const int miso_pu = gpio_get(ILI9341_PIN_MISO);
    gpio_pull_down(ILI9341_PIN_MISO);
    sleep_us(200);
    const int miso_pd = gpio_get(ILI9341_PIN_MISO);
    gpio_disable_pulls(ILI9341_PIN_MISO);
    gpio_set_function(ILI9341_PIN_MISO, GPIO_FUNC_SPI);

    printf("D id=%02X%02X%02X%02X pwr=%02X%02X st=%02X%02X%02X%02X%02X miso_pu=%d miso_pd=%d\n",
           id[0], id[1], id[2], id[3], pwr[0], pwr[1],
           st[0], st[1], st[2], st[3], st[4], miso_pu, miso_pd);
}

static void ReportState(void) {
    printf("S frame=%lu attached=%u held=%02X tap=%02X tapf=%u "
           "LCDC=%02X LY=%02X PC=%04X P1=%02X sel=%02X dpad=%02X btn=%02X\n",
           (unsigned long)frame_counter, (unsigned)attached,
           (unsigned)held, (unsigned)tap, (unsigned)tap_frames,
           (unsigned)memory.io_reg[0x40], (unsigned)memory.io_reg[0x44],
           (unsigned)cpu.PC,
           (unsigned)Memory_Read_Byte(0xFF00), (unsigned)memory.io_reg[0x00],
           (unsigned)memory.d_pad_state, (unsigned)memory.buttons_state);
}

static void HandleCommand(const char* c) {
    switch (*c) {
        case 'v':   // version handshake
            printf("V agentlink 1\n");
            break;

        case 'a':   // attach: take over the joypad
            attached = 1;
            held = 0; tap = 0; tap_frames = 0; rep_count = 0; rep_tick = 0;
            printf("A attached\n");
            break;

        case 'd':   // detach: hand the joypad back to the hardware
            attached = 0;
            held = 0; tap = 0; tap_frames = 0; rep_count = 0; rep_tick = 0;
            printf("A detached\n");
            break;

        case 'k': { // k<hex2>  hold this button set until changed
            const char* p = c + 1;
            held = (uint8_t)ParseHex(&p, 2);
            printf("K %02X\n", (unsigned)held);
            break;
        }

        case 't': { // t<hex2>,<dec>  tap a button set for N frames
            const char* p = c + 1;
            tap = (uint8_t)ParseHex(&p, 2);
            if (*p == ',') p++;
            const uint32_t n = ParseDec(&p);
            tap_frames = (uint16_t)(n ? n : 1);
            printf("T %02X %u\n", (unsigned)tap, (unsigned)tap_frames);
            break;
        }

        case 'r': { // r<hex4>,<dec>  read guest memory
            const char* p = c + 1;
            const uint16_t addr = (uint16_t)ParseHex(&p, 4);
            if (*p == ',') p++;
            const uint32_t len = ParseDec(&p);
            DumpMemory(addr, (uint16_t)(len ? len : 16));
            break;
        }

        case 'n': { // n<hex2>,<dec>  tap a button set N times, queued here
            const char* p = c + 1;
            rep_mask = (uint8_t)ParseHex(&p, 2);
            if (*p == ',') p++;
            rep_count = (uint16_t)ParseDec(&p);
            rep_tick = 0;
            printf("N %02X %u\n", (unsigned)rep_mask, (unsigned)rep_count);
            break;
        }

        case 'w':   // raw input pin + ADC readout, for wiring checks
            ReportWiring();
            break;

        case 'x':   // display path: baud, geometry, frames, DMA, pin levels
            ReportDisplay();
            break;

        case 'i':   // ask the panel to identify itself over MISO
            ReadPanelID();
            break;

        case 's':
            ReportState();
            break;

        case 'g':   // atomic Tetris snapshot: board + active piece + next + score
            DumpGame();
            break;

        default:
            printf("E unknown %c\n", *c);
            break;
    }
}

// Drains whatever stdin has buffered without ever blocking, so a frame costs
// nothing when no one is driving the link.
static void PollCommands(void) {
    for (int guard = 0; guard < 256; guard++) {
        const int ch = getchar_timeout_us(0);
        if (ch < 0) return;

        if (ch == '\n' || ch == '\r') {
            if (cmd_len) {
                cmd[cmd_len] = 0;
                HandleCommand(cmd);
                cmd_len = 0;
            }
        } else if (cmd_len < CMD_MAX - 1) {
            cmd[cmd_len++] = (char)ch;
        }
    }
}

static void ApplyButtons(void) {
    uint8_t b = (uint8_t)(held | (tap_frames ? tap : 0));

    if (rep_count) {
        if (rep_tick < REP_ON) {
            b |= rep_mask;
        }
        if (++rep_tick >= REP_ON + REP_OFF) {
            rep_tick = 0;
            rep_count--;
        }
    }

    // Joypad lines read low when pressed.
    uint8_t buttons = 0x0F;
    uint8_t dpad = 0x0F;

    if (b & AGENT_A)      buttons &= (uint8_t)~A_BUTTON_MASK;
    if (b & AGENT_B)      buttons &= (uint8_t)~B_BUTTON_MASK;
    if (b & AGENT_SELECT) buttons &= (uint8_t)~SELECT_BUTTON_MASK;
    if (b & AGENT_START)  buttons &= (uint8_t)~START_BUTTON_MASK;

    if (b & AGENT_RIGHT)  dpad &= (uint8_t)~RIGHT_BUTTON_MASK;
    if (b & AGENT_LEFT)   dpad &= (uint8_t)~LEFT_BUTTON_MASK;
    if (b & AGENT_UP)     dpad &= (uint8_t)~UP_BUTTON_MASK;
    if (b & AGENT_DOWN)   dpad &= (uint8_t)~DOWN_BUTTON_MASK;

    memory.buttons_state = buttons;
    memory.d_pad_state = dpad;

    if (tap_frames) tap_frames--;
}

void AgentLink_Init(void) {
    attached = 0;
    held = 0;
    tap = 0;
    tap_frames = 0;
    frame_counter = 0;
    cmd_len = 0;
}

void AgentLink_Frame(void) {
    frame_counter++;

    // Apply before polling so a state report reflects what the guest will
    // actually see, rather than the hardware sample taken moments earlier.
    // While detached this touches nothing, leaving get_input()'s sample intact.
    if (attached) {
        ApplyButtons();
    }

    PollCommands();
}

#endif
