#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "joypad_lld.h"

#include "chprintf.h"

#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)

#define BUTTON_ENTER_PORT	IOPORT1
#define BUTTON_UP_PORT		IOPORT1
#define BUTTON_DOWN_PORT	IOPORT1
#define BUTTON_LEFT_PORT	IOPORT1
#define BUTTON_RIGHT_PORT	IOPORT1

#define BUTTON_ENTER_PIN	0
#define BUTTON_UP_PIN		BTN1
#define BUTTON_DOWN_PIN		BTN2
#define BUTTON_LEFT_PIN		BTN3
#define BUTTON_RIGHT_PIN	BTN4

static void joyInterrupt (EXTDriver *, expchannel_t);

static thread_reference_t joyThreadReference;
static THD_WORKING_AREA(waJoyThread, 128);

/* Default state is all buttons pressed. */

static uint8_t joyState = 0xFF;

static const joyInfo joyTbl[6] = {
	{ BUTTON_ENTER_PORT, BUTTON_ENTER_PIN, JOY_ENTER, keySelect },
	{ BUTTON_UP_PORT, BUTTON_UP_PIN, JOY_UP, keyUp },
	{ BUTTON_DOWN_PORT, BUTTON_DOWN_PIN, JOY_DOWN, keyDown },
	{ BUTTON_LEFT_PORT, BUTTON_LEFT_PIN, JOY_LEFT, keyLeft },
	{ BUTTON_RIGHT_PORT, BUTTON_RIGHT_PIN, JOY_RIGHT, keyRight },
};

static const EXTConfig ext_config = {
	{
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  BTN1 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  BTN2 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  BTN3 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  BTN4 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ 0 , NULL },
		{ 0 , NULL },
		{ 0 , NULL },
		{ 0 , NULL }
	}
};

static void
joyInterrupt (EXTDriver *extp, expchannel_t chan)
{
	osalSysLockFromISR ();
	if (joyThreadReference != NULL)
		osalThreadResumeI (&joyThreadReference, MSG_OK);
	osalSysUnlockFromISR ();
	return;
}

static uint8_t
joyHandle (uint8_t s)
{
#ifdef notyet
	OrchardAppEvent evt;
#endif
	uint8_t pad;

	pad = palReadPad (joyTbl[s].port, joyTbl[s].pin);

	pad <<= s;

	if (pad ^ (joyState & joyTbl[s].bit)) {
		joyState &= ~joyTbl[s].bit;
		joyState |= pad;

		if (pad)
			printf ("button %x released\r\n", joyTbl[s].code);
		else
			printf ("button %x pressed\r\n", joyTbl[s].code);
#ifdef notyet  
		joyEvent.code = joyTbl[s].code;
		if (pad)
			joyEvent.flags = keyRelease;
		else
			joyEvent.flags = keyPress;

		if (instance.context != NULL) {
			evt.type = keyEvent;
 			evt.key = joyEvent;
			if (joyEvent.code != keyTilt &&
			    joyEvent.flags == keyPress) {
				dacPlay("click.raw");
				dacWait ();
			}
			instance.app->event (instance.context, &evt);
		}
#endif
		return (1);
	}

	return (0);
}

static THD_FUNCTION(joyThread, arg)
{
	(void)arg;
    
	chRegSetThreadName ("JoyEvent");
    
	while (1) {
		osalSysLock ();
		osalThreadSuspendS (&joyThreadReference);
		osalSysUnlock ();
#ifdef notyet
		if (joyHandle (JOY_ENTER_SHIFT))
			continue;
#endif
		if (joyHandle (JOY_UP_SHIFT))
			continue;

		if (joyHandle (JOY_DOWN_SHIFT))
			continue;

		if (joyHandle (JOY_LEFT_SHIFT))
			continue;

		if (joyHandle (JOY_RIGHT_SHIFT))
			continue;
	}

	/* NOTREACHED */
}

void
joyStart (void)
{
	/* Launch button handler thread. */

	chThdCreateStatic (waJoyThread, sizeof(waJoyThread),
	    NORMALPRIO + 1, joyThread, NULL);

#ifdef notyet
	palSetPad (BUTTON_ENTER_PORT, BUTTON_ENTER_PIN);
#endif
	palSetPad (BUTTON_UP_PORT, BUTTON_UP_PIN);
	palSetPad (BUTTON_DOWN_PORT, BUTTON_DOWN_PIN);
	palSetPad (BUTTON_LEFT_PORT, BUTTON_LEFT_PIN);
	palSetPad (BUTTON_RIGHT_PORT, BUTTON_RIGHT_PIN);

	extStart (&EXTD1, &ext_config);
	return;
}
