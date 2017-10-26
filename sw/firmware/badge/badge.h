#ifndef _BADGE_H_
#define _BADGE_H_
#include "chprintf.h"

#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)

#endif /* _BADGE_H_ */
