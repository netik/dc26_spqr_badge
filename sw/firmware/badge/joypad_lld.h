#ifndef _JOYPAD_LLD_H_
#define _JOYPAD_LLD_H_

/* Joypad event codes */

typedef enum _OrchardAppEventKeyCode {
	keyUp = 0x80,
	keyDown = 0x81,
	keyLeft = 0x82,
	keyRight = 0x83,
	keySelect = 0x84,
} OrchardAppEventKeyCode;

/* Joypad events */

typedef struct _joyInfo {
	ioportid_t port;
	uint8_t pin;
	uint8_t bit;
	OrchardAppEventKeyCode code;
} joyInfo;


#define JOY_ENTER	0x01
#define JOY_UP		0x02
#define JOY_DOWN	0x04
#define JOY_LEFT	0x08
#define JOY_RIGHT	0x10

#define JOY_ENTER_SHIFT	0
#define JOY_UP_SHIFT	1
#define JOY_DOWN_SHIFT	2
#define JOY_LEFT_SHIFT	3
#define JOY_RIGHT_SHIFT	4

extern void joyStart (void);
#endif /* _JOYPAD_LLD_H_ */
