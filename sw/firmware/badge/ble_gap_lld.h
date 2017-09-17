#ifndef _BLE_GAP_LLD_H_
#define _BLE_GAP_LLD_H_

enum
{
    UNIT_0_625_MS = 625,        /**< Number of microseconds in 0.625 milliseconds. */
    UNIT_1_25_MS  = 1250,       /**< Number of microseconds in 1.25 milliseconds. */
    UNIT_10_MS    = 10000       /**< Number of microseconds in 10 milliseconds. */
};


#define MSEC_TO_UNITS(TIME, RESOLUTION) (((TIME) * 1000) / (RESOLUTION))

extern void bleGapDispatch (ble_evt_t *);
extern uint8_t * bleGapAdvStart (uint8_t *);
extern uint32_t bleGapAdvElementAdd (void * pElem, uint8_t len, uint8_t etype,
	uint8_t * pkt, uint8_t * size);
extern uint32_t bleGapAdvFinish (uint8_t *, uint8_t);

extern void bleGapStart (void);

#endif /* __BLE_GAP_LLD_H */
