#define _GNU_SOURCE

/*
	Copyright 2017 Delio Brignoli <brignoli.delio@gmail.com>

	Arduboy board implementation using simavr.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Include Tuya system APIs */
#include "tkl_system.h"

#include "simavr/sim_avr.h"
#include "simavr/avr_adc.h"
#include "simavr/avr_ioport.h"
#include "simavr/avr_extint.h"
#include "simavr/sim_io.h"
#include "simavr/sim_hex.h"
// #include <sim_gdb.h>
#include "simavr/sim_time.h"
#include "simavr/parts/ssd1306_virt.h"

#include "sim_arduboy.h"
#include "arduboy_avr.h"

/* ssd1306_gl functions are now in arduboy_emu_app.c */
extern void ssd1306_gl_init(float pixel_size, int win_width, int win_height);
extern void ssd1306_gl_update_lumamap(struct ssd1306_t *ssd1306, const uint8_t luma_decay, const uint8_t luma_inc);
extern void ssd1306_gl_render(struct ssd1306_t *ssd1306);
extern void ssd1306_gl_cleanup(void);


#define MHZ_16 (16000000)


static struct button_info {
	enum button_e btn_id;
	avr_irq_t *irq;
	const char *name;
	char port_name;
	int port_idx;
	bool pressed;
} buttons[BTN_COUNT] = {
	{0, NULL, "btn.up", 'F', 7}, /* BTN_UP */
	{1, NULL, "btn.down", 'F', 4},
	{2, NULL, "btn.left", 'F', 5},
	{3, NULL, "btn.right", 'F', 6},
	{4, NULL, "btn.a", 'E', 6},
	{5, NULL, "btn.b", 'B', 4},
};

/* SSD1306 wired to the SPI bus, with the following additional pins: */
static ssd1306_wiring_t ssd1306_wiring =
{
	.chip_select.port = 'D',
	.chip_select.pin = 6,
	.data_instruction.port = 'D',
	.data_instruction.pin = 4,
	.reset.port = 'D',
	.reset.pin = 7,
};

static struct arduboy_avr_mod_state {
	struct avr_t *avr;
	ssd1306_t ssd1306;
	SYS_TICK_T start_time_ticks;
	bool yield;
} mod_s;

/*
Sleep callback for embedded systems - simplified timing sync
*/
static void avr_callback_sleep_sync(
		avr_t *avr,
		avr_cycle_count_t how_long)
{
	/* For embedded systems, use a simplified timing approach */
	/* Calculate how long we should wait in microseconds */
	uint64_t sleep_us = avr_cycles_to_usec(avr, how_long);
	
	/* For very short sleeps, just yield control */
	if (sleep_us < 100) {
		mod_s.yield = true;
		return;
	}
	
	/* For longer sleeps, use Tuya system delay */
	tkl_system_sleep(sleep_us / 1000); /* Convert to milliseconds */
}

/*
Platform time source callback for embedded systems
*/
static uint64_t avr_platform_time_ns(avr_t *avr)
{
	(void)avr;
	/* Use Tuya system ticks converted to nanoseconds */
	SYS_TICK_T ticks = tkl_system_get_tick_count();
	return (uint64_t)ticks * 1000000ull; /* Convert ticks to nanoseconds */
}

static avr_cycle_count_t update_luma(
		avr_t *avr,
		avr_cycle_count_t when,
		void *param)
{
	ssd1306_gl_update_lumamap(param, LUMA_DECAY, LUMA_INC);
	return avr->cycle + avr_usec_to_cycles(avr, SSD1306_FRAME_PERIOD_US);
}

static avr_cycle_count_t render_timer_callback(
			avr_t *avr,
			avr_cycle_count_t when,
			void *param)
{
    ssd1306_gl_render(param);
	/* Don't force yield here - let the main loop control timing */
	return avr->cycle + avr_usec_to_cycles(avr, GL_FRAME_PERIOD_US);
}

struct ssd1306_t *arduboy_avr_ssd1306(void)
{
	return &mod_s.ssd1306;
}

void arduboy_avr_button_event(enum button_e btn_e, bool pressed)
{
	struct button_info *btn = (btn_e < BTN_COUNT) ? &buttons[btn_e] : NULL;
	if (btn && btn->pressed != pressed) {
		avr_raise_irq(btn->irq, !pressed);
		btn->pressed = pressed;
	}
}

void arduboy_adc_update_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
	avr_irq_t *iop_irq = avr_io_getirq(mod_s.avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_ADC1);
	uint16_t milivolts = (uint16_t)(rand() % ADC_VREF_V256);
	avr_raise_irq(iop_irq, milivolts);
}

void arduboy_avr_loop(void)
{
	avr_t *avr = mod_s.avr;
	mod_s.yield = false;
	while (!mod_s.yield) {
		avr->run(avr);
		int state = avr->state;
		if (state == cpu_Done || state == cpu_Crashed)
			break;
	}
}

int arduboy_avr_setup(struct sim_arduboy_opts *opts)
{
	avr_t *avr = avr_make_mcu_by_name("atmega32u4");
	if (!avr) {
		return -1;
	}

	avr_init(avr);

	/*
	BTN_A is wired to INT6 which defaults to level triggered.
	This means that while button A is pressed the interrupt triggers
	continuously. This is very expensive to simulate so we set non-strict
	level trigger mode for INT6.

	Why doesn't this affect real h/w?
	*/
	avr_extint_set_strict_lvl_trig(avr, EXTINT_IRQ_OUT_INT6, 0);

    {
        /* Load .hex and setup program counter */
        uint32_t boot_base = 0, boot_size = 0;
        uint8_t * boot = NULL;
        if (opts->hex_file_path && opts->hex_file_path[0]) {
            boot = read_ihex_file(opts->hex_file_path, &boot_size, &boot_base);
            if (!boot) {
                fprintf(stderr, "Unable to load %s\n", opts->hex_file_path);
                return -1;
            }
        } else {
            /* Fallback to embedded firmware */
            extern const uint32_t firmware_2048_base;
            extern const uint32_t firmware_2048_size;
            extern const uint8_t firmware_2048_data[];
            boot_base = firmware_2048_base;
            boot_size = firmware_2048_size;
            boot = (uint8_t *)firmware_2048_data;
        }
        memcpy(avr->flash + boot_base, boot, boot_size);
        if (opts->hex_file_path && opts->hex_file_path[0]) {
            free(boot);
        }
        avr->pc = boot_base;
        /* end of flash, remember we are writing /code/ */
        avr->codeend = avr->flashend;
    }

	/* more simulation parameters */
	avr->log = 1 + opts->verbose;
	avr->frequency = MHZ_16;
	
	/* Set up platform-specific callbacks */
	avr->sleep = avr_callback_sleep_sync;
	avr_set_platform_time_source(avr, avr_platform_time_ns);
	
	avr->run_cycle_limit = avr_usec_to_cycles(avr, 2*GL_FRAME_PERIOD_US);
	avr->aref = ADC_VREF_V256;

	/* setup and connect display controller */
	ssd1306_init(avr, &mod_s.ssd1306, OLED_WIDTH_PX, OLED_HEIGHT_PX);
	ssd1306_connect(&mod_s.ssd1306, &ssd1306_wiring);
	ssd1306_gl_init(opts->pixel_size, opts->win_width, opts->win_height);

	/* setup and connect buttons */
	for (int btn_idx=0; btn_idx<BTN_COUNT; btn_idx++) {
		struct button_info *binfo = &buttons[btn_idx];
		binfo->irq = avr_alloc_irq(&avr->irq_pool, 0, 1, &binfo->name);
		uint32_t iop_ctl = AVR_IOCTL_IOPORT_GETIRQ(binfo->port_name);
		avr_irq_t *iop_irq = avr_io_getirq(avr, iop_ctl, binfo->port_idx);
		avr_connect_irq(binfo->irq, iop_irq);
		/* pull up pin */
		avr_raise_irq(binfo->irq, 1);
	}

	/* Take simulation start time using Tuya system ticks */
	mod_s.start_time_ticks = tkl_system_get_tick_count();

	/* Setup display render timers */
	avr_cycle_timer_register_usec(avr, SSD1306_FRAME_PERIOD_US, update_luma, &mod_s.ssd1306);
	avr_cycle_timer_register_usec(avr, GL_FRAME_PERIOD_US, render_timer_callback, &mod_s.ssd1306);

	/* Setup initial random seed */
	srand((unsigned int)time(NULL));

	/* Setup ADC1 update hook for Arduboy initRandomSeed() function */
	avr_irq_t *iop_irq = avr_io_getirq(avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
	avr_irq_register_notify(iop_irq, arduboy_adc_update_hook, NULL);

	/* setup for GDB debugging */
	avr->gdb_port = opts->gdb_port;

	mod_s.avr = avr;
	return 0;
}

void arduboy_avr_teardown(void)
{

}