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

#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)

void
nordic_fault_handler (uint32_t id, uint32_t pc, uint32_t info)
{
	return;
}

static thread_reference_t sdThreadReference;
static ble_evt_t ble_evt;

static THD_WORKING_AREA(waSdThread, 64);
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
			r = sd_ble_evt_get(p_dest, &p_len);
			if (r != NRF_SUCCESS)
				break;
			printf ("moo... %d %x %d\r\n", r,
			    ble_evt.header.evt_id, ble_evt.header.evt_len);
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
	ble_gap_conn_sec_mode_t perm;
 	ble_gap_adv_params_t params;
	const uint8_t * ble_name = (uint8_t *)"NRF52 IDES";

	/* Create SoftDevice event thread */

	chThdCreateStatic(waSdThread, sizeof(waSdThread),
	    NORMALPRIO + 1, sdThread, NULL);

	/* Set up SoftDevice ISR */

	nvicEnableVector (SD_EVT_IRQn, 5);

	/* Initialize the SoftDevice */

	memset (&clock_source, 0, sizeof(clock_source));
	clock_source.source = NRF_CLOCK_LF_SRC_XTAL;
	clock_source.accuracy = NRF_CLOCK_LF_ACCURACY_75_PPM;
	r = sd_softdevice_enable (&clock_source, nordic_fault_handler);

	printf ("SOFTDEVICE ENABLE: %d\r\n", r);

	/* Enable BLE support in SoftDevice */

	r = sd_ble_enable (&ram_start);

 	printf ("BLE ENABLE: %d (RAM: %x)\r\n", r, ram_start);

	memset (&addr, 0, sizeof(addr));
	r = sd_ble_gap_addr_get (&addr);

	printf ("addr: (%d) %x:%x:%x:%x:%x:%x\r\n", r,
	    addr.addr[0], addr.addr[1], addr.addr[2],
	    addr.addr[3], addr.addr[4], addr.addr[5]);

	perm.sm = 0;
	perm.lv = 0;

	r = sd_ble_gap_device_name_set (&perm, ble_name, 10);

	printf ("NAME SET: %d\r\n", r);

	params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
	params.p_peer_addr = NULL;
	params.fp          = BLE_GAP_ADV_FP_ANY;
	params.interval    = 0x1000;
	params.timeout     = 30 /*ADV_TIMEOUT_IN_SECONDS*/;

	r = sd_ble_gap_adv_start (&params, BLE_CONN_CFG_TAG_DEFAULT);

	printf ("ADVERTISEMENT START: %d\r\n", r);

	return;
}

