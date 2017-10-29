#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "hal_spi.h"
#include "chprintf.h"
#include "shell.h"

#include "gfx.h"

#include "ble_lld.h"

#include "ff.h"
#include "ffconf.h"
#include "diskio.h"

#include "async_io_lld.h"
#include "joypad_lld.h"

#include "badge.h"

/* linker set for command objects */

orchard_command_start();
orchard_command_end();

#define LED_EXT 14

bool watchdog_started = false;

SPIConfig spi1_config = {
	NULL,			/* enc_cp */
	NRF5_SPI_FREQ_8MBPS,	/* freq */
	IOPORT1_SPI_SCK,	/* sckpad */
	IOPORT1_SPI_MOSI,	/* mosipad */
	IOPORT1_SPI_MISO,	/* misopad */
	IOPORT1_SDCARD_CS,	/* sspad */
	FALSE,			/* lsbfirst */
	2,			/* mode */
	0xFF			/* dummy data for spiIgnore() */
};

I2CConfig i2c2_config = {
	100000,			/* clock */
	IOPORT1_I2C_SCK,	/* scl_pad */
	IOPORT1_I2C_SDA		/* sda_pad */
};

void
gpt_callback(GPTDriver *gptp)
{
	(void)gptp;
	return;
}

void
mmc_callback(GPTDriver *gptp)
{
	(void)gptp;
	disk_timerproc ();
}

/*
 * GPT configuration
 * Frequency: 16MHz
 * Resolution: 32 bits
 */

static const GPTConfig gpt1_config = {
    .frequency  = NRF5_GPT_FREQ_16MHZ,
    .callback   = gpt_callback,
    .resolution = 32,
};

static const GPTConfig gpt2_config = {
    .frequency  = NRF5_GPT_FREQ_62500HZ,
    .callback   = mmc_callback,
    .resolution = 32,
};

static THD_WORKING_AREA(shell_wa, 1024);

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD1,
  orchard_commands
};

static SerialConfig serial_config = {
    .speed   = 115200,
    .tx_pad  = UART_TX,
    .rx_pad  = UART_RX,
#if NRF5_SERIAL_USE_HWFLOWCTRL	== TRUE
    .rts_pad = UART_RTS,
    .cts_pad = UART_CTS,
#endif
};


static THD_WORKING_AREA(waThread1, 64);
static THD_FUNCTION(Thread1, arg) {

    (void)arg;
    uint8_t led = LED4;
    
    chRegSetThreadName("blinker");

    
    while (1) {
	palTogglePad(IOPORT1, led);
	chThdSleepMilliseconds(1000);
    }
}

void
SVC_Handler (void)
{
	while (1) {}
}

/**@brief Function for application main entry.
 */
int main(void)
{
    font_t font;

#ifdef CRT0_VTOR_INIT
    __disable_irq();
    SCB->VTOR = 0;
    __enable_irq();
#endif

    halInit();
    chSysInit();
    shellInit();
 
    sdStart (&SD1, &serial_config);
    printf ("\r\n");

    palClearPad (IOPORT1, LED1);
    palClearPad (IOPORT1, LED2);
    palClearPad (IOPORT1, LED3);
    palClearPad (IOPORT1, LED4);

    gptStart (&GPTD2, &gpt1_config);
    gptStart (&GPTD3, &gpt2_config);

    /* Launch test blinker thread. */
    
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1,
		      Thread1, NULL);

    chThdSleep(2);
    printf("SYSTEM START\r\n");
    chThdSleep(2);
    printf(PORT_INFO "\r\n");
    chThdSleep(2);

    printf("Priority levels %d\r\n", CORTEX_PRIORITY_LEVELS);

    joyStart ();
    asyncIoStart ();

    palSetPad (IOPORT1, IOPORT1_SPI_MOSI);
    palSetPad (IOPORT1, IOPORT1_SPI_MISO);
    palSetPad (IOPORT1, IOPORT1_SPI_SCK);
    palSetPad (IOPORT1, IOPORT1_SDCARD_CS);
    palSetPad (IOPORT1, IOPORT1_TOUCH_CS);

    spiStart (&SPID1, &spi1_config);

    i2cStart (&I2CD2, &i2c2_config);

    gfxInit ();

    /*disk_initialize (DRV_MMC);*/

    if (gfileMount ('F', "0:") == FALSE)
        printf ("No SD card found.\r\n");
    else
        printf ("SD card detected.\r\n");

    gdispClear (Blue);

    font = gdispOpenFont ("DejaVuSans24");

    gdispDrawStringBox (0, 0, gdispGetWidth(),
        gdispGetFontMetric(font, fontHeight),
        "Hello world......", font, White, justifyCenter);

    gdispCloseFont (font);

    font = gdispOpenFont ("fixed_10x20");

    gdispDrawStringBox (0, 40, gdispGetWidth(),
        gdispGetFontMetric(font, fontHeight),
        "Hello world......", font, White, justifyCenter);

    gdispCloseFont (font);

    bleStart ();

    NRF_P0->DETECTMODE = 0;

    /* Launch shell thread */

    chThdCreateStatic(shell_wa, sizeof(shell_wa), NORMALPRIO+1,
		      shellThread, (void *)&shell_cfg1);

    while (true) {
	if (watchdog_started &&
	    (palReadPad(IOPORT1, BTN1) == 0)) {
	    palTogglePad(IOPORT1, LED1);
	    wdgReset(&WDGD1);
	    printf("Watchdog reseted\r\n");
	}
	chThdSleepMilliseconds(250);
    }

}
