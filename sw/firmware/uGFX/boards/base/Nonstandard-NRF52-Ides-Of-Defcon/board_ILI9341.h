/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#define ILI9341_CD		0x00000800	/* Command/data select */
#define ILI9341_WR		0x00000400	/* Write signal */
#define ILI9341_RD		0x00000200	/* Read signal */

#define ILI9341_DATA		0xFF000000
#define ILI9341_PIXEL_LO(x)	(((x) << 24) & ILI9341_DATA)
#define ILI9341_PIXEL_HI(x)	(((x) << 16) & ILI9341_DATA)

__attribute__ ((noinline))
static void init_board(GDisplay *g) {
	(void) g;

	palSetPad (IOPORT1, IOPORT1_SCREEN_RD);
	palSetPad (IOPORT1, IOPORT1_SCREEN_WR);
	palSetPad (IOPORT1, IOPORT1_SCREEN_CD);

	palSetPad (IOPORT1, IOPORT1_SCREEN_D0);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D1);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D2);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D3);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D4);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D5);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D6);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D7);

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

static void release_bus(GDisplay *g) {
	(void) g;
	return;
}

__attribute__ ((noinline))
static void write_index(GDisplay *g, uint16_t index) {
	(void) g;

	/*
         * Assert command/data pin (select command)
         * and assert the write command pin.
         */

	IOPORT1->OUTCLR = ILI9341_CD|ILI9341_DATA;
	IOPORT1->OUTSET = ILI9341_PIXEL_LO(index);

	__disable_irq();
	IOPORT1->OUTCLR = ILI9341_WR;
	__asm__("nop");
	__asm__("nop");
	IOPORT1->OUTSET = ILI9341_WR;
	__enable_irq();

	/* Deassert command/data and write pins. */

	IOPORT1->OUTSET = ILI9341_CD;

	return;
}

__attribute__ ((noinline))
static void write_data(GDisplay *g, uint16_t data) {
	(void) g;

	IOPORT1->OUTCLR = ILI9341_DATA;
	IOPORT1->OUTSET = ILI9341_PIXEL_LO(data);

	__disable_irq();
	IOPORT1->OUTCLR = ILI9341_WR;
	__asm__("nop");
	__asm__("nop");
	IOPORT1->OUTSET = ILI9341_WR;
	__enable_irq();

	return;
}

__attribute__ ((noinline))
static void write_data16(GDisplay *g, uint16_t data) {
	(void) g;

	IOPORT1->OUTCLR = ILI9341_DATA;
	IOPORT1->OUTSET = ILI9341_PIXEL_HI(data);

	__disable_irq();
	IOPORT1->OUTCLR = ILI9341_WR;
	__asm__("nop");
	__asm__("nop");
	IOPORT1->OUTSET = ILI9341_WR;
	__enable_irq();

	IOPORT1->OUTCLR = ILI9341_DATA;
	IOPORT1->OUTSET = ILI9341_PIXEL_LO(data);

	__disable_irq();
	IOPORT1->OUTCLR = ILI9341_WR;
	__asm__("nop");
	__asm__("nop");
	IOPORT1->OUTSET = ILI9341_WR;
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
