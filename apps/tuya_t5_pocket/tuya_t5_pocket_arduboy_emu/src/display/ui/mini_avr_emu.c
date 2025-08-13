/**
 * @file mini_avr_emu.c
 * @brief Minimal AVR emulation for ARM Cortex-M33
 * @version 0.1
 * @date 2025-03-25
 */

#include "mini_avr_emu.h"
#include <stdlib.h>
#include <string.h>

/***********************************************************
************************macro define************************
***********************************************************/

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/

static mini_avr_t g_avr;
static bool g_button_states[BTN_COUNT] = {false};

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Initialize minimal AVR emulator
 * @param firmware_data Pointer to firmware data
 * @param firmware_size Size of firmware data
 * @return 0 on success, -1 on failure
 */
int mini_avr_init(const uint8_t *firmware_data, uint32_t firmware_size)
{
    /* Allocate memory for AVR */
    g_avr.flash = (uint8_t*)malloc(AVR_FLASH_SIZE);
    g_avr.ram = (uint8_t*)malloc(AVR_RAM_SIZE);
    g_avr.eeprom = (uint8_t*)malloc(AVR_EEPROM_SIZE);
    
    if (!g_avr.flash || !g_avr.ram || !g_avr.eeprom) {
        mini_avr_cleanup();
        return -1;
    }
    
    /* Initialize memory */
    memset(g_avr.flash, 0xFF, AVR_FLASH_SIZE);
    memset(g_avr.ram, 0x00, AVR_RAM_SIZE);
    memset(g_avr.eeprom, 0xFF, AVR_EEPROM_SIZE);
    
    /* Copy firmware to flash */
    if (firmware_data && firmware_size > 0) {
        uint32_t copy_size = (firmware_size < AVR_FLASH_SIZE) ? firmware_size : AVR_FLASH_SIZE;
        memcpy(g_avr.flash, firmware_data, copy_size);
    }
    
    /* Initialize AVR state */
    memset(g_avr.r, 0, sizeof(g_avr.r));
    g_avr.sp = AVR_RAM_SIZE - 1;
    g_avr.pc = 0;
    g_avr.sreg = 0;
    g_avr.cycles = 0;
    g_avr.running = true;
    
    /* Initialize display buffer */
    memset(g_avr.display_buffer, 0, DISPLAY_BUFFER_SIZE);
    
    /* Initialize button states */
    memset(g_button_states, 0, sizeof(g_button_states));
    
    return 0;
}

/**
 * @brief Run one AVR instruction (simplified)
 * @return 0 on success, -1 on error
 */
int mini_avr_step(void)
{
    if (!g_avr.running) {
        return -1;
    }
    
    /* For now, just increment PC and cycles */
    /* This is a placeholder - in a real implementation, 
       you would decode and execute actual AVR instructions */
    g_avr.pc += 2;  /* AVR instructions are 16-bit */
    g_avr.cycles++;
    
    /* Simple bounds checking */
    if (g_avr.pc >= AVR_FLASH_SIZE) {
        g_avr.pc = 0;  /* Reset to beginning */
    }
    
    /* Update display buffer with a simple pattern for testing */
    static uint8_t pattern = 0;
    pattern++;
    
    /* Create a simple moving pattern */
    for (int i = 0; i < DISPLAY_BUFFER_SIZE; i++) {
        g_avr.display_buffer[i] = pattern + i;
    }
    
    return 0;
}

/**
 * @brief Handle button press/release
 * @param button Button index
 * @param pressed True if pressed, false if released
 */
void mini_avr_button_event(button_e button, bool pressed)
{
    if (button < BTN_COUNT) {
        g_button_states[button] = pressed;
        
        /* Update some RAM location to simulate button input */
        if (pressed) {
            g_avr.ram[0x20 + button] = 0xFF;  /* Set button state in RAM */
        } else {
            g_avr.ram[0x20 + button] = 0x00;  /* Clear button state in RAM */
        }
    }
}

/**
 * @brief Get display buffer for rendering
 * @return Pointer to 128x64 display buffer
 */
uint8_t* mini_avr_get_display_buffer(void)
{
    return g_avr.display_buffer;
}

/**
 * @brief Cleanup AVR emulator
 */
void mini_avr_cleanup(void)
{
    if (g_avr.flash) {
        free(g_avr.flash);
        g_avr.flash = NULL;
    }
    if (g_avr.ram) {
        free(g_avr.ram);
        g_avr.ram = NULL;
    }
    if (g_avr.eeprom) {
        free(g_avr.eeprom);
        g_avr.eeprom = NULL;
    }
    
    g_avr.running = false;
}
