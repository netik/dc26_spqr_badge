#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "osal.h"
#include "chprintf.h"

#include "nrf_sdm.h"
#include "ble.h"
#include "ble_gap.h"

#include "ble_gap_lld.h"

static thread_reference_t sdThreadReference;
static ble_evt_t ble_evt;

#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)

void
nordic_fault_handler (uint32_t id, uint32_t pc, uint32_t info)
{
	return;
}

static void
bleEventDispatch (ble_evt_t * evt)
{
	if (evt->header.evt_id >= BLE_EVT_BASE &&
	    evt->header.evt_id <= BLE_EVT_LAST)
		printf ("common BLE event\r\n");

	if (evt->header.evt_id >= BLE_GAP_EVT_BASE &&
	    evt->header.evt_id <= BLE_GAP_EVT_LAST) {
		printf ("GAP event (%d)\r\n", evt->header.evt_id);
		bleGapDispatch (evt);
	}

	if (evt->header.evt_id >= BLE_GATTC_EVT_BASE &&
	    evt->header.evt_id <= BLE_GATTC_EVT_LAST)
		printf ("GATT client event\r\n");

	if (evt->header.evt_id >= BLE_GATTS_EVT_BASE &&
	    evt->header.evt_id <= BLE_GATTS_EVT_LAST)
		printf ("GATT server event\r\n");

	if (evt->header.evt_id >= BLE_L2CAP_EVT_BASE &&
	    evt->header.evt_id <= BLE_L2CAP_EVT_LAST)
		printf ("L2CAP event\r\n");

	return;
}

static THD_WORKING_AREA(waSdThread, 256);
static THD_FUNCTION(sdThread, arg)
{
	uint8_t * p_dest;
	uint16_t p_len;
	int r;

	(void)arg;
    
	chRegSetThreadName("SDEvent");

	while (1) {
		osalSysLock ();
		osalThreadSuspendS (&sdThreadReference);
		osalSysUnlock ();

		while (1) {
			p_dest = (uint8_t *)&ble_evt;
			p_len = sizeof (ble_evt);
			r = sd_ble_evt_get (p_dest, &p_len);
			if (r != NRF_SUCCESS)
				break;
			bleEventDispatch (&ble_evt);
		}
    	}

	/* NOTREACHED */
}

/*
 * ISR for the SoftDevice event interrupt
 */

OSAL_IRQ_HANDLER(Vector98)
{
	OSAL_IRQ_PROLOGUE();
	osalSysLockFromISR ();
	osalThreadResumeI (&sdThreadReference, MSG_OK);
	osalSysUnlockFromISR (); 
	OSAL_IRQ_EPILOGUE();
	return;
}

/*
 * This symbol is created by the linker script. Its address is
 * the start of application RAM.
 */

extern uint32_t __ram0_start__;

void
ble_start(void)
{
	int r;
 	uint32_t ram_start = (uint32_t)&__ram0_start__;
	nrf_clock_lf_cfg_t clock_source;
	ble_gap_addr_t addr;

	/* Create SoftDevice event thread */

	chThdCreateStatic(waSdThread, sizeof(waSdThread),
	    NORMALPRIO + 1, sdThread, NULL);

	/* Set up SoftDevice ISR */

	nvicEnableVector (SD_EVT_IRQn, 5);

	/* Initialize the SoftDevice */

	memset (&clock_source, 0, sizeof(clock_source));
	clock_source.source = NRF_CLOCK_LF_SRC_XTAL;
	clock_source.accuracy = NRF_CLOCK_LF_ACCURACY_20_PPM;
	r = sd_softdevice_enable (&clock_source, nordic_fault_handler);

	if (r == NRF_SUCCESS)
		printf ("SoftDevice enabled.\r\n");
	else {
		printf ("Enabling softdevice failed (%x)\r\n");
		return;
	}

	/* Enable BLE support in SoftDevice */

	r = sd_ble_enable (&ram_start);

	if (r == NRF_SUCCESS)
		printf ("Bluetooth LE enabled. (RAM base: %x)\r\n",
		    ram_start);
	else {
		printf ("Enabling BLE failed (%x)\r\n");
		return;
	}

	memset (&addr, 0, sizeof(addr));
	sd_ble_gap_addr_get (&addr);

	printf ("Station address: %x:%x:%x:%x:%x:%x\r\n",
	    addr.addr[5], addr.addr[4], addr.addr[3],
	    addr.addr[2], addr.addr[1], addr.addr[0]);

	bleGapStart ();

	return;
}
