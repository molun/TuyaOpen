/**
 * @file mini_avr_emu.h
 * @brief Minimal AVR emulation for ARM Cortex-M33
 * @version 0.1
 * @date 2025-03-25
 */

#ifndef __MINI_AVR_EMU_H__
#define __MINI_AVR_EMU_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/

/* AVR ATmega32u4 specific constants */
#define AVR_FLASH_SIZE    32768
#define AVR_RAM_SIZE      2560
#define AVR_EEPROM_SIZE   1024

/* Display buffer size for 128x64 OLED */
#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64
#define DISPLAY_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)

/* Button definitions */
typedef enum {
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_A,
    BTN_B,
    BTN_COUNT,
} button_e;

/***********************************************************
***********************typedef define***********************
***********************************************************/

typedef struct {
    uint8_t r[32];        /* General purpose registers R0-R31 */
    uint16_t sp;          /* Stack pointer */
    uint16_t pc;          /* Program counter */
    uint8_t sreg;         /* Status register */
    uint8_t *flash;       /* Flash memory */
    uint8_t *ram;         /* RAM */
    uint8_t *eeprom;      /* EEPROM */
    uint32_t cycles;      /* Cycle counter */
    bool running;         /* CPU running state */
    uint8_t display_buffer[DISPLAY_BUFFER_SIZE]; /* Display buffer */
} mini_avr_t;

/***********************************************************
********************function declaration********************
***********************************************************/

/**
 * @brief Initialize minimal AVR emulator
 * @param firmware_data Pointer to firmware data
 * @param firmware_size Size of firmware data
 * @return 0 on success, -1 on failure
 */
int mini_avr_init(const uint8_t *firmware_data, uint32_t firmware_size);

/**
 * @brief Run one AVR instruction
 * @return 0 on success, -1 on error
 */
int mini_avr_step(void);

/**
 * @brief Handle button press/release
 * @param button Button index
 * @param pressed True if pressed, false if released
 */
void mini_avr_button_event(button_e button, bool pressed);

/**
 * @brief Get display buffer for rendering
 * @return Pointer to 128x64 display buffer
 */
uint8_t* mini_avr_get_display_buffer(void);

/**
 * @brief Cleanup AVR emulator
 */
void mini_avr_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* __MINI_AVR_EMU_H__ */
