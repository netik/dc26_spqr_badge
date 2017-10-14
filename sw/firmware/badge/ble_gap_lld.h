#ifndef _BLE_GAP_LLD_H_
#define _BLE_GAP_LLD_H_

extern uint16_t ble_conn_handle;
extern uint8_t ble_gap_role;
extern ble_gap_addr_t ble_peer_addr;

#define BLE_IDES_SCAN_TIMEOUT		30
#define BLE_IDES_ADV_TIMEOUT		30

enum
{
    UNIT_0_625_MS = 625,        /**< Number of microseconds in 0.625 milliseconds. */
    UNIT_1_25_MS  = 1250,       /**< Number of microseconds in 1.25 milliseconds. */
    UNIT_10_MS    = 10000       /**< Number of microseconds in 10 milliseconds. */
};


#define MSEC_TO_UNITS(TIME, RESOLUTION) (((TIME) * 1000) / (RESOLUTION))

extern void bleGapDispatch (ble_evt_t *);

extern void bleGapStart (void);

extern int bleGapConnect (ble_gap_addr_t *);
extern int bleGapDisconnect (void);

#endif /* __BLE_GAP_LLD_H */
