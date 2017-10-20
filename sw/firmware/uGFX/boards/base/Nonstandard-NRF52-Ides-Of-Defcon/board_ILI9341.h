/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include <chprintf.h>
#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)

#define ILI9341_CS		0x00001000	/* Chip select -- always on */
#define ILI9341_CD		0x00000800	/* Command/data select */
#define ILI9341_WR		0x00000400	/* Write signal */
#define ILI9341_RD		0x00000200	/* Read signal */

#define ILI9341_DATA		0xFF000000
#define ILI9341_PIXEL_LO(x)	(((x) << 24) & ILI9341_DATA)
#define ILI9341_PIXEL_HI(x)	(((x) << 16) & ILI9341_DATA)

#define NRF52_OUT		0x50000504
#define NRF52_SET		0x50000508
#define NRF52_CLR		0x5000050C
#define NRF52_IN		0x50000510

__attribute__ ((noinline))
static void init_board(GDisplay *g) {
	(void) g;

	palSetPad (IOPORT1, 0x09);
	palSetPad (IOPORT1, 0x0A);
	palSetPad (IOPORT1, 0x0B);
	palClearPad (IOPORT1, 0x0C);

	palSetPad (IOPORT1, 0x18);
	palSetPad (IOPORT1, 0x19);
	palSetPad (IOPORT1, 0x1A);
	palSetPad (IOPORT1, 0x1B);
	palSetPad (IOPORT1, 0x1C);
	palSetPad (IOPORT1, 0x1D);
	palSetPad (IOPORT1, 0x1E);
	palSetPad (IOPORT1, 0x1F);

	return;
}

static GFXINLINE void post_init_board(GDisplay *g) {
	(void) g;
}

static GFXINLINE void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	(void) state;
}

static GFXINLINE void set_backlight(GDisplay *g, uint8_t percent) {
	(void) g;
	(void) percent;
}

static void acquire_bus(GDisplay *g) {
	(void) g;
	return;
}

__attribute__ ((noinline))
static void release_bus(GDisplay *g) {
	(void) g;
	return;
}

__attribute__ ((noinline))
static void write_index(GDisplay *g, uint16_t index) {
	volatile uint32_t * pSet = (uint32_t *)NRF52_SET;
	volatile uint32_t * pClr = (uint32_t *)NRF52_CLR;

	(void) g;

	/*
         * Assert command/data pin (select command)
         * and assert the write command pin.
         */

	*pClr = ILI9341_CD|ILI9341_DATA;
	*pSet = ILI9341_PIXEL_LO(index);

	__disable_irq();
	*pClr = ILI9341_WR;
	__asm__("nop");
	__asm__("nop");
	*pSet = ILI9341_WR;
	__enable_irq();

	/* Deassert command/data and write pins. */

	*pSet = ILI9341_CD;

	return;
}

__attribute__ ((noinline))
static void write_data(GDisplay *g, uint16_t data) {
	volatile uint32_t * pSet = (uint32_t *)NRF52_SET;
	volatile uint32_t * pClr = (uint32_t *)NRF52_CLR;

	(void) g;

	*pClr = ILI9341_DATA;
	*pSet = ILI9341_PIXEL_LO(data);

	__disable_irq();
	*pClr = ILI9341_WR;
	__asm__("nop");
	__asm__("nop");
	*pSet = ILI9341_WR;
	__enable_irq();

	return;
}

__attribute__ ((noinline))
static void write_data16(GDisplay *g, uint16_t data) {
	volatile uint32_t * pSet = (uint32_t *)NRF52_SET;
	volatile uint32_t * pClr = (uint32_t *)NRF52_CLR;

	(void) g;

	*pClr = ILI9341_DATA;
	*pSet = ILI9341_PIXEL_HI(data);

	__disable_irq();
	*pClr = ILI9341_WR;
	__asm__("nop");
	__asm__("nop");
	*pSet = ILI9341_WR;
	__enable_irq();

	*pClr = ILI9341_DATA;
	*pSet = ILI9341_PIXEL_LO(data);

	__disable_irq();
	*pClr = ILI9341_WR;
	__asm__("nop");
	__asm__("nop");
	*pSet = ILI9341_WR;
	__enable_irq();

	return;
}

static GFXINLINE void setreadmode(GDisplay *g) {
	(void) g;
}

static GFXINLINE void setwritemode(GDisplay *g) {
	(void) g;
}

static GFXINLINE uint16_t read_data(GDisplay *g) {
	(void) g;
	return 0;
}

#endif /* _GDISP_LLD_BOARD_H */
