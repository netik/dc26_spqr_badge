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
#include "ble_advdata.h"


enum
{
    UNIT_0_625_MS = 625,        /**< Number of microseconds in 0.625 milliseconds. */
    UNIT_1_25_MS  = 1250,       /**< Number of microseconds in 1.25 milliseconds. */
    UNIT_10_MS    = 10000       /**< Number of microseconds in 10 milliseconds. */
};


#define MSEC_TO_UNITS(TIME, RESOLUTION) (((TIME) * 1000) / (RESOLUTION))


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
 	ble_gap_adv_params_t adv_params;
	ble_advdata_t advdata;
#ifdef notdef
	ble_gap_conn_params_t conn_params;
	ble_cfg_t cfg;
#endif

	const uint8_t * ble_name = (uint8_t *)"DC26 IDES";

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

#ifdef notdef
	memset (&cfg, 0, sizeof(cfg));

	cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 0;
	r = sd_ble_cfg_set (BLE_COMMON_CFG_VS_UUID, &cfg, ram_start);

 	printf ("BLE CFG SET: %d (RAM: %x)\r\n", r, ram_start);

	memset (&cfg, 0, sizeof(cfg));
	cfg.gap_cfg.role_count_cfg.periph_role_count =
	    BLE_GAP_ROLE_COUNT_PERIPH_DEFAULT;
	cfg.gap_cfg.role_count_cfg.central_role_count = 0;
	cfg.gap_cfg.role_count_cfg.central_sec_count = 0;

	r = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &cfg, ram_start);

 	printf ("BLE CFG SET: %d (RAM: %x)\r\n", r, ram_start);
#endif

	/* Enable BLE support in SoftDevice */

	r = sd_ble_enable (&ram_start);

 	printf ("BLE ENABLE: %d (RAM: %x)\r\n", r, ram_start);

	memset (&addr, 0, sizeof(addr));
	r = sd_ble_gap_addr_get (&addr);

	printf ("addr: (%d) %x:%x:%x:%x:%x:%x\r\n", r,
	    addr.addr[0], addr.addr[1], addr.addr[2],
	    addr.addr[3], addr.addr[4], addr.addr[5]);

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&perm);
	r = sd_ble_gap_device_name_set (&perm, ble_name, 9);

	printf ("NAME SET: %d\r\n", r);

        sd_ble_gap_appearance_set (BLE_APPEARANCE_GENERIC_COMPUTER);
#ifdef notdef
	memset (&conn_params, 0, sizeof(conn_params));

	conn_params.min_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS);
	conn_params.max_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS);
	conn_params.slave_latency = 0;
	conn_params.conn_sup_timeout = MSEC_TO_UNITS(4000, UNIT_10_MS);

	sd_ble_gap_ppcp_set (&conn_params);
#endif

	memset(&advdata, 0, sizeof(advdata));
	advdata.name_type = BLE_ADVDATA_FULL_NAME;
	advdata.short_name_len = 9;
	advdata.include_appearance = true;
	advdata.flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED |
	    BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;

	r = ble_advdata_set (&advdata, NULL);

	printf ("ADVDATA SET: %d\r\n", r);

	adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
	adv_params.p_peer_addr = NULL;
	adv_params.fp          = BLE_GAP_ADV_FP_ANY;
	adv_params.interval    = MSEC_TO_UNITS(500, UNIT_0_625_MS);
	adv_params.timeout     = 0 /*ADV_TIMEOUT_IN_SECONDS*/;

	r = sd_ble_gap_adv_start (&adv_params, BLE_CONN_CFG_TAG_DEFAULT);

	printf ("ADVERTISEMENT START: %d\r\n", r);

	return;
}

