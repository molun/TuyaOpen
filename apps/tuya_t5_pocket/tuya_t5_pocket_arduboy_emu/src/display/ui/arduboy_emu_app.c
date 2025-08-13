/**
 * @file arduboy_emu_app.c
 * Complete Arduboy Emulator LVGL Integration
 */

/*********************
 *      INCLUDES
 *********************/
#include "arduboy_emu_app.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Use minimal AVR emulation instead of simavr */
#include "mini_avr_emu.h"

/*********************
 *      DEFINES
 *********************/
/* LVGL key codes for input mapping */
#define KEY_UP    17  // LV_KEY_UP
#define KEY_LEFT  20  // LV_KEY_LEFT
#define KEY_DOWN  18  // LV_KEY_DOWN
#define KEY_RIGHT 19  // LV_KEY_RIGHT
#define KEY_ENTER 10  // LV_KEY_ENTER
#define KEY_ESC   27  // LV_KEY_ESC

/* OLED display dimensions (Arduboy original) */
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

/* Canvas buffer sizing for maximum scaling */
#define MAX_CANVAS_SIZE (OLED_WIDTH * OLED_HEIGHT * 8)  /* Max scaling factor of 8 */

/* Button count for key state tracking */
#define BUTTON_COUNT 6

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    lv_obj_t *canvas;           /* LVGL canvas object for rendering */
    int scale;                  /* Integer scale factor to fit into LVGL screen */
    int offset_x;               /* X offset to center the display */
    int offset_y;               /* Y offset to center the display */
    int canvas_width;           /* Actual canvas width in pixels */
    int canvas_height;          /* Actual canvas height in pixels */
    uint8_t luma_pixmap[OLED_WIDTH * OLED_HEIGHT]; /* Grayscale pixel data */
    bool initialized;           /* Initialization state flag */
} ArduboyLvglContext;

/**********************
 *  STATIC VARIABLES
 **********************/
static bool key_states[BUTTON_COUNT] = {false}; /* Track key states for button debouncing */
static lv_timer_t *g_release_timer = NULL;      /* Timer for auto key release */
static ArduboyLvglContext g_ctx;                /* Global context for LVGL integration */
static lv_color_t g_canvas_buffer[MAX_CANVAS_SIZE]; /* Static buffer to avoid memory management issues */

/**********************
 *  STATIC PROTOTYPES
 **********************/
static inline int key_to_button_e(uint32_t key);
static void key_release_timer_cb(lv_timer_t *timer);
static void arduboy_key_event_cb(lv_event_t *e);
static inline lv_color_t gray_u8_to_rgb(uint8_t v);
static void calculate_display_scaling(int32_t screen_width, int32_t screen_height);
static void create_canvas(void);
static void update_display_from_avr(void);

/**********************
 *   MINIMAL AVR INTEGRATION FUNCTIONS
 **********************/

/**
 * Initialize the LVGL canvas renderer
 */
void ssd1306_gl_init(float pixel_size, int win_width, int win_height)
{
    LV_UNUSED(pixel_size);
    LV_UNUSED(win_width);
    LV_UNUSED(win_height);

    /* Prevent double initialization and memory leaks */
    if (g_ctx.initialized) {
        return;
    }

    memset(&g_ctx, 0, sizeof(g_ctx));

    /* Get current screen dimensions */
    lv_obj_t *screen = lv_screen_active();
    int32_t screen_width = lv_obj_get_width(screen);
    int32_t screen_height = lv_obj_get_height(screen);

    /* Calculate optimal scaling and positioning */
    calculate_display_scaling(screen_width, screen_height);

    /* Create the canvas */
    create_canvas();

    g_ctx.initialized = true;
}

/**
 * Update the display from AVR emulation
 */
void ssd1306_gl_update_lumamap(void *ssd1306, const uint8_t luma_decay, const uint8_t luma_inc)
{
    LV_UNUSED(ssd1306);
    LV_UNUSED(luma_decay);
    LV_UNUSED(luma_inc);

    /* Update display from AVR emulation */
    update_display_from_avr();
}

/**
 * Render the display to LVGL canvas
 */
void ssd1306_gl_render(void *ssd1306)
{
    LV_UNUSED(ssd1306);

    if (!g_ctx.initialized || !g_ctx.canvas) {
        return;
    }

    /* Get display buffer from AVR emulation */
    uint8_t *display_buffer = mini_avr_get_display_buffer();
    if (!display_buffer) {
        return;
    }

    /* Convert display buffer to canvas pixels */
    for (int y = 0; y < g_ctx.canvas_height; y++) {
        for (int x = 0; x < g_ctx.canvas_width; x++) {
            int src_x = x / g_ctx.scale;
            int src_y = y / g_ctx.scale;
            
            if (src_x < OLED_WIDTH && src_y < OLED_HEIGHT) {
                int byte_index = (src_y / 8) * OLED_WIDTH + src_x;
                int bit_index = src_y % 8;
                
                if (byte_index < (OLED_WIDTH * OLED_HEIGHT / 8)) {
                    uint8_t pixel = (display_buffer[byte_index] >> bit_index) & 1;
                    lv_color_t color = pixel ? lv_color_white() : lv_color_black();
                    
                    lv_canvas_set_px_color(g_ctx.canvas, x, y, color);
                }
            }
        }
    }

    /* Invalidate the canvas to trigger redraw */
    lv_obj_invalidate(g_ctx.canvas);
}

/**
 * Cleanup display resources
 */
void ssd1306_gl_cleanup(void)
{
    if (g_ctx.canvas) {
        lv_obj_del(g_ctx.canvas);
        g_ctx.canvas = NULL;
    }
    g_ctx.initialized = false;
}

/**********************
 *   ARDUBOY EMULATOR FUNCTIONS
 **********************/

/**
 * Initialize Arduboy emulator app
 */
void lv_arduboy_emu_app(void)
{
    /* Nothing to initialize for now */
}

/**
 * Start the Arduboy emulator
 * @param hex_path Path to hex file, or NULL for embedded firmware
 */
void arduboy_emu_start(const char *hex_path)
{
    LV_UNUSED(hex_path);

    /* Initialize minimal AVR emulation with embedded firmware */
    extern const uint8_t firmware_2048_data[];
    extern const uint32_t firmware_2048_size;
    
    if (mini_avr_init(firmware_2048_data, firmware_2048_size) != 0) {
        return;
    }

    /* Create invisible focusable object to receive LVGL key events */
    lv_obj_t *focus = lv_obj_create(lv_screen_active());
    lv_obj_add_flag(focus, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(focus, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(focus, arduboy_key_event_cb, LV_EVENT_KEY, NULL);
    lv_group_add_obj(lv_group_get_default(), focus);
    lv_group_focus_obj(focus);
}

/**
 * Handle input events for Arduboy emulation
 * @param key The LVGL key code pressed
 */
void lv_arduboy_emu_app_handle_input(uint32_t key)
{
    int button_index = key_to_button_e(key);
    if (button_index >= 0 && button_index < BUTTON_COUNT) {
        if (!key_states[button_index]) {
            /* Key press - send button event to emulator */
            mini_avr_button_event((button_e)button_index, true);
            key_states[button_index] = true;
            
            /* Set up auto-release after short delay */
            if (g_release_timer == NULL) {
                g_release_timer = lv_timer_create(key_release_timer_cb, 100, NULL);
            } else {
                lv_timer_reset(g_release_timer);
            }
        }
    }
}

/**
 * Run AVR emulation step
 */
void arduboy_avr_loop(void)
{
    /* Run one step of the minimal AVR emulation */
    mini_avr_step();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Convert LVGL key code to Arduboy button enum
 * @param key LVGL key code
 * @return Button index (0-5) or -1 if not mapped
 */
static inline int key_to_button_e(uint32_t key)
{
    switch(key) {
        case KEY_UP: return 0;      // BTN_UP
        case KEY_DOWN: return 1;    // BTN_DOWN  
        case KEY_LEFT: return 2;    // BTN_LEFT
        case KEY_RIGHT: return 3;   // BTN_RIGHT
        case KEY_ENTER:
        case 122: return 4;         // BTN_A ('z' key)
        case KEY_ESC:
        case 120: return 5;         // BTN_B ('x' key)
        default: return -1;
    }
}

/**
 * Timer callback to automatically release pressed buttons
 * @param timer Unused timer parameter
 */
static void key_release_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    
    /* Release all pressed buttons */
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (key_states[i]) {
            mini_avr_button_event((button_e)i, false);
            key_states[i] = false;
        }
    }
}

/**
 * LVGL key event callback - forwards events to input handler
 * @param e LVGL event object
 */
static void arduboy_key_event_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_arduboy_emu_app_handle_input(key);
}

/**
 * Convert 8-bit grayscale value to LVGL color
 * @param v 8-bit grayscale value (0-255)
 * @return LVGL color with RGB components set to v
 */
static inline lv_color_t gray_u8_to_rgb(uint8_t v)
{
    return lv_color_make(v, v, v);
}

/**
 * Calculate optimal display scaling and positioning
 * @param screen_width LVGL screen width
 * @param screen_height LVGL screen height
 */
static void calculate_display_scaling(int32_t screen_width, int32_t screen_height)
{
    /* Calculate scale factors for both dimensions */
    int scale_by_height = screen_height / OLED_HEIGHT;
    int scale_by_width = screen_width / OLED_WIDTH;
    
    /* Use the smaller scale to maintain aspect ratio */
    g_ctx.scale = (scale_by_height < scale_by_width) ? scale_by_height : scale_by_width;
    if (g_ctx.scale < 1) g_ctx.scale = 1;
    
    /* Calculate scaled dimensions */
    int scaled_width = OLED_WIDTH * g_ctx.scale;
    int scaled_height = OLED_HEIGHT * g_ctx.scale;
    
    /* Center the display */
    g_ctx.offset_x = (screen_width - scaled_width) / 2;
    g_ctx.offset_y = (screen_height - scaled_height) / 2;

    g_ctx.canvas_width = scaled_width;
    g_ctx.canvas_height = scaled_height;

    /* Ensure we don't exceed static buffer size */
    int total_pixels = g_ctx.canvas_width * g_ctx.canvas_height;
    if (total_pixels > MAX_CANVAS_SIZE) {
        g_ctx.scale = 1;  /* Fallback to no scaling if too large */
        g_ctx.canvas_width = OLED_WIDTH;
        g_ctx.canvas_height = OLED_HEIGHT;
        g_ctx.offset_x = (screen_width - g_ctx.canvas_width) / 2;
        g_ctx.offset_y = (screen_height - g_ctx.canvas_height) / 2;
    }
}

/**
 * Create and configure the LVGL canvas for rendering
 */
static void create_canvas(void)
{
    g_ctx.canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_buffer(g_ctx.canvas, g_canvas_buffer, g_ctx.canvas_width, g_ctx.canvas_height, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_size(g_ctx.canvas, g_ctx.canvas_width, g_ctx.canvas_height);
    lv_obj_set_pos(g_ctx.canvas, g_ctx.offset_x, g_ctx.offset_y);

    /* Initialize with black background */
    lv_canvas_fill_bg(g_ctx.canvas, lv_color_black(), LV_OPA_COVER);
}

/**
 * Update display from AVR emulation
 */
static void update_display_from_avr(void)
{
    /* This function is called periodically to update the display */
    /* The actual display update is handled in ssd1306_gl_render() */
}