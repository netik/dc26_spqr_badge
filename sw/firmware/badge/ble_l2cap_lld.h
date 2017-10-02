#ifndef _BLE_L2CAP_LLD_H_
#define _BLE_L2CAP_LLD_H_

#define BLE_IDES_PSM		0x1234
#define BLE_IDES_L2CAP_LEN	128

extern void bleL2CapDispatch (ble_evt_t *);

extern int bleL2CapConnect (void);
extern int bleL2CapDisconnect (void);
extern int bleL2CapSend (char *);

extern void bleL2CapStart (void);

#endif /* __BLE_L2CAP_LLD_H */
