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
#include "ble_hci.h"

#include "ble_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gap_lld.h"

#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)


static ble_gap_lesc_p256_pk_t peer_pk;
static ble_gap_lesc_p256_pk_t my_pk;

uint16_t ble_conn_handle;
uint8_t ble_gap_role;
ble_gap_addr_t ble_peer_addr;

/*
 * This symbol is created by the linker script. Its address is
 * the start of application RAM.
 */

extern uint32_t __ram0_start__;

static int
bleGapScanStart (void)
{
	ble_gap_scan_params_t scan;
	int r;

	scan.active = FALSE;
	scan.use_whitelist = FALSE;
	scan.adv_dir_report = FALSE;
	scan.timeout = 0;
	scan.interval = MSEC_TO_UNITS(1000, UNIT_0_625_MS);
	scan.window = MSEC_TO_UNITS(50, UNIT_0_625_MS);

	r = sd_ble_gap_scan_start (&scan);

	return (r);
}

void
bleGapDispatch (ble_evt_t * evt)
{
	ble_gap_sec_keyset_t sec_keyset;
	ble_gap_sec_params_t sec_params;
	ble_gap_addr_t * addr;
	int r;

	switch (evt->header.evt_id) {
		case BLE_GAP_EVT_CONNECTED:
			ble_conn_handle = evt->evt.gap_evt.conn_handle;
			addr =&evt->evt.gap_evt.params.connected.peer_addr;
			memcpy (&ble_peer_addr, addr, sizeof(ble_gap_addr_t));
			ble_gap_role = evt->evt.gap_evt.params.connected.role;

			printf ("gap connected (handle: %x) ", ble_conn_handle);
			printf ("peer: %x:%x:%x:%x:%x:%x ",
			    addr->addr[5], addr->addr[4], addr->addr[3],
			    addr->addr[2], addr->addr[1], addr->addr[0]);
			printf ("role; %d\r\n",
			    evt->evt.gap_evt.params.connected.role);

			bleGapScanStart ();

			break;

		case BLE_GAP_EVT_DISCONNECTED:
			printf ("gap disconnected...\r\n");
			bleGapStart ();

			break;

		case BLE_GAP_EVT_AUTH_STATUS:
			printf ("gap auth status... (%d)\r\n",
			    evt->evt.gap_evt.params.auth_status.auth_status);
			break;

		case BLE_GAP_EVT_CONN_SEC_UPDATE:
			printf ("gap connections security update...\r\n");
			break;

		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
			memset (&sec_keyset, 0, sizeof(sec_keyset));
			memset (&sec_params, 0, sizeof(sec_params));
		        sec_keyset.keys_own.p_pk  = &my_pk;
        		sec_keyset.keys_peer.p_pk = &peer_pk;
			sec_params.min_key_size = 7;
			sec_params.max_key_size = 16;
			sec_params.io_caps = BLE_GAP_IO_CAPS_NONE;
			r = sd_ble_gap_sec_params_reply (ble_conn_handle,
			    BLE_GAP_SEC_STATUS_SUCCESS, &sec_params,
			    &sec_keyset);
			printf ("gap security param request...(%d)\r\n", r);
			break;

		case BLE_GAP_EVT_ADV_REPORT:
			printf ("GAP scan report...\r\n");
			addr =&evt->evt.gap_evt.params.adv_report.peer_addr;
			printf ("peer: %x:%x:%x:%x:%x:%x\r\n",
			    addr->addr[5], addr->addr[4], addr->addr[3],
			    addr->addr[2], addr->addr[1], addr->addr[0]);
			break;

		case BLE_GAP_EVT_TIMEOUT:
			printf ("GAP timeout event\r\n");
			bleGapStart ();
			break;

		default:
			printf ("invalid GAP event %d\r\n",
			   evt->header.evt_id);
			break;
	}

	return;
}

uint8_t *
bleGapAdvStart (uint8_t * size)
{
	uint8_t * pkt;

	pkt = chHeapAlloc (NULL, BLE_GAP_ADV_MAX_SIZE);

	if (pkt == NULL) {
		*size = 0;
		return (NULL);
	}

	memset (pkt, 0, BLE_GAP_ADV_MAX_SIZE);
	*size = BLE_GAP_ADV_MAX_SIZE;

	return (pkt);
}

uint32_t
bleGapAdvElementAdd (void * pElem, uint8_t len, uint8_t etype,
		  uint8_t * pkt, uint8_t * size)
{
	uint8_t * p;

	if ((len + 2) > *size)
            return (NRF_ERROR_NO_MEM);

	p = pkt;
	p += BLE_GAP_ADV_MAX_SIZE - *size;
	p[0] = len + 1;
	p[1] = etype;
	memcpy (&p[2], pElem, len);

	*size -= (2 + len);

	return (NRF_SUCCESS);
}

uint32_t
bleGapAdvFinish (uint8_t * pkt, uint8_t len)
{
        ble_gap_adv_params_t adv_params;
	uint32_t r;

	r = sd_ble_gap_adv_data_set (pkt, len, NULL, 0);

	if (r != NRF_SUCCESS)
            return (r);

	adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
	adv_params.p_peer_addr = NULL;
	adv_params.fp = BLE_GAP_ADV_FP_ANY;
	adv_params.interval = MSEC_TO_UNITS(33, UNIT_0_625_MS);
	adv_params.timeout = 0 /*ADV_TIMEOUT_IN_SECONDS*/;

	r = sd_ble_gap_adv_start (&adv_params, BLE_IDES_APP_TAG);

	chHeapFree (pkt);

	return (r);
}

void
bleGapStart (void)
{
	ble_gap_conn_sec_mode_t perm;

	uint8_t * ble_name = (uint8_t *)"DC26 IDES";
	uint8_t * pkt;
	uint8_t size;
	uint16_t val;

	ble_conn_handle = BLE_CONN_HANDLE_INVALID;

	sd_ble_gap_tx_power_set (4);

	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&perm);
	sd_ble_gap_device_name_set (&perm, ble_name,
	    strlen ((char *)ble_name));

	sd_ble_gap_appearance_set (BLE_APPEARANCE_GENERIC_COMPUTER);

	pkt = bleGapAdvStart (&size);

	/* Set our full name */

	bleGapAdvElementAdd (ble_name, strlen ((char *)ble_name),
	    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, pkt, &size);

	/* Set our appearance */

	val = BLE_APPEARANCE_GENERIC_COMPUTER;
	bleGapAdvElementAdd (&val, 2,
	    BLE_GAP_AD_TYPE_APPEARANCE, pkt, &size);

	/* Set flags */

	val = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED |
            BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;
	bleGapAdvElementAdd (&val, 1,
	    BLE_GAP_AD_TYPE_FLAGS, pkt, &size);

	/* Set role */

	val = 2 /*BLE_GAP_ROLE_PERIPH*/;
	bleGapAdvElementAdd (&val, 1,
	    BLE_GAP_AD_TYPE_LE_ROLE, pkt, &size);

	/* Begin advertisement */

	bleGapAdvFinish (pkt, BLE_GAP_ADV_MAX_SIZE - size);

	bleGapScanStart ();

	return;
}

int
bleGapConnect (ble_gap_addr_t * peer)
{
	ble_gap_scan_params_t sparams;
	ble_gap_conn_params_t cparams;
	int r;

	memset (&sparams, 0, sizeof(sparams));
	memset (&cparams, 0, sizeof(cparams));

	sparams.active = FALSE;
	sparams.use_whitelist = FALSE;
	sparams.adv_dir_report = FALSE;
	sparams.timeout = 5; /* Timeout in seconds */
	sparams.interval = MSEC_TO_UNITS(1000, UNIT_0_625_MS);
	sparams.window = MSEC_TO_UNITS(50, UNIT_0_625_MS);

	cparams.min_conn_interval = MSEC_TO_UNITS(10, UNIT_1_25_MS);
	cparams.max_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS);
	cparams.slave_latency = 0;
	cparams.conn_sup_timeout = MSEC_TO_UNITS(400, UNIT_10_MS);

	/* Can't scan and connect at the same time. */

	/*sd_ble_gap_scan_stop ();*/

	r = sd_ble_gap_connect (peer, &sparams,
	    &cparams, BLE_IDES_APP_TAG);

	if (r != NRF_SUCCESS) {
		printf ("GAP connect failed: %x\r\n", r);
		return (r);
	}

	return (NRF_SUCCESS);
}

int
bleGapDisconnect (void)
{
	int r;

	r = sd_ble_gap_disconnect (ble_conn_handle,
	    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);

	return (r);
}

