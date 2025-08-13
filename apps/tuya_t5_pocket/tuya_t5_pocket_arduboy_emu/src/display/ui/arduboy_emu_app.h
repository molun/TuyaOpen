/**
 * @file arduboy_emu_app.h
 * Arduboy Emulator LVGL Integration
 */

#ifndef ARDUBOY_EMU_APP_H
#define ARDUBOY_EMU_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#ifndef ARDUBOY_EMU_SCREEN_WIDTH
#define ARDUBOY_EMU_SCREEN_WIDTH  384
#endif
#ifndef ARDUBOY_EMU_SCREEN_HEIGHT
#define ARDUBOY_EMU_SCREEN_HEIGHT 168
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize Arduboy emulator app
 */
void lv_arduboy_emu_app(void);

/**
 * Handle input events for Arduboy emulation
 * @param key The LVGL key code pressed
 */
void lv_arduboy_emu_app_handle_input(uint32_t key);

/**
 * Start the Arduboy emulator
 * @param hex_path Path to hex file, or NULL for embedded firmware
 */
void arduboy_emu_start(const char *hex_path);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ARDUBOY_EMU_APP_H */
