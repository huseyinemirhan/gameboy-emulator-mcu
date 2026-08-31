#include "inputs.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include "hardware/adc.h"
#include "../../memory/memory.h"


// Button pins
#define BTN_A       2
#define BTN_B       3
#define BTN_START   4
#define BTN_SELECT  5

// Joystick pins
#define JOY_X_PIN   26  
#define JOY_Y_PIN   27  
#define JOY_SW_PIN  6   

#define JOY_THRESHOLD 1000 

void Input_Init() {

    gpio_init(BTN_A);      gpio_set_dir(BTN_A,      GPIO_IN); gpio_pull_up(BTN_A);
    gpio_init(BTN_B);      gpio_set_dir(BTN_B,      GPIO_IN); gpio_pull_up(BTN_B);
    gpio_init(BTN_START);  gpio_set_dir(BTN_START,  GPIO_IN); gpio_pull_up(BTN_START);
    gpio_init(BTN_SELECT); gpio_set_dir(BTN_SELECT, GPIO_IN); gpio_pull_up(BTN_SELECT);
    gpio_init(JOY_SW_PIN); gpio_set_dir(JOY_SW_PIN, GPIO_IN); gpio_pull_up(JOY_SW_PIN);


    adc_init();
    adc_gpio_init(JOY_X_PIN);
    adc_gpio_init(JOY_Y_PIN);
}

void get_input() {
    memory.d_pad_state   = 0x0F;
    memory.buttons_state = 0x0F;

    if (!gpio_get(BTN_A))      memory.buttons_state &= ~A_BUTTON_MASK;
    if (!gpio_get(BTN_B))      memory.buttons_state &= ~B_BUTTON_MASK;
    if (!gpio_get(BTN_START))  memory.buttons_state &= ~START_BUTTON_MASK;
    if (!gpio_get(JOY_SW_PIN)) memory.buttons_state &= ~SELECT_BUTTON_MASK;

    adc_select_input(0);
    uint16_t x = adc_read();

    adc_select_input(1);
    uint16_t y = adc_read();

    if (x < 2048 - JOY_THRESHOLD) memory.d_pad_state &= ~LEFT_BUTTON_MASK;
    if (x > 2048 + JOY_THRESHOLD) memory.d_pad_state &= ~RIGHT_BUTTON_MASK;
    if (y < 2048 - JOY_THRESHOLD) memory.d_pad_state &= ~UP_BUTTON_MASK;
    if (y > 2048 + JOY_THRESHOLD) memory.d_pad_state &= ~DOWN_BUTTON_MASK;
}