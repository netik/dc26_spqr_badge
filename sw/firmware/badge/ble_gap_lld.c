/*-
 * Copyright (c) 2017
 *      Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "nrf_sdm.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_hci.h"

#include "ble_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gap_lld.h"
#include "ble_peer.h"

#include "orchard-app.h"

#include "badge.h"

static ble_gap_lesc_p256_pk_t peer_pk;
static ble_gap_lesc_p256_pk_t my_pk;

static uint8_t * bleGapAdvBlockStart (uint8_t *);
static uint32_t bleGapAdvBlockAdd (void * pElem, uint8_t len, uint8_t etype,
	uint8_t * pkt, uint8_t * size);
static uint32_t bleGapAdvBlockFinish (uint8_t *, uint8_t);
static int bleGapScanStart (void);
static int bleGapAdvStart (void);

uint16_t ble_conn_handle;
uint8_t ble_gap_role;
ble_gap_addr_t ble_peer_addr;

static int
bleGapScanStart (void)
{
	ble_gap_scan_params_t scan;
	int r;

	scan.active = FALSE;
	scan.use_whitelist = FALSE;
	scan.adv_dir_report = FALSE;
	scan.timeout = BLE_IDES_SCAN_TIMEOUT;
	scan.interval = MSEC_TO_UNITS(1000, UNIT_0_625_MS);
	scan.window = MSEC_TO_UNITS(50, UNIT_0_625_MS);

	r = sd_ble_gap_scan_start (&scan);

	return (r);
}

static int
bleGapAdvStart (void)
{
	uint8_t * pkt;
	uint8_t size;
	uint16_t val;
	uint8_t ble_name[BLE_GAP_DEVNAME_MAX_LEN];
	uint16_t len = BLE_GAP_DEVNAME_MAX_LEN;
	int r;

	pkt = bleGapAdvBlockStart (&size);

	/* Set our appearance */

	val = BLE_APPEARANCE_DC26;
	r = bleGapAdvBlockAdd (&val, 2,
	    BLE_GAP_AD_TYPE_APPEARANCE, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Set our full name */

	sd_ble_gap_device_name_get (ble_name, &len);

	r = bleGapAdvBlockAdd (ble_name, len,
	    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Set manufacturer ID */

	val = BLE_COMPANY_ID_IDES;
	r = bleGapAdvBlockAdd (&val, 2,
	    BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Set flags */

	val = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED |
            BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;
	r = bleGapAdvBlockAdd (&val, 1,
	    BLE_GAP_AD_TYPE_FLAGS, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Set role */

	val = 2 /*BLE_GAP_ROLE_PERIPH*/;
	r = bleGapAdvBlockAdd (&val, 1,
	    BLE_GAP_AD_TYPE_LE_ROLE, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Begin advertisement */

	r = bleGapAdvBlockFinish (pkt, BLE_GAP_ADV_MAX_SIZE - size);

	return (r);
}

void
bleGapDispatch (ble_evt_t * evt)
{
	ble_gap_sec_keyset_t sec_keyset;
	ble_gap_sec_params_t sec_params;
	ble_gap_evt_timeout_t * timeout;
#ifdef BLE_GAP_VERBOSE
	ble_gap_evt_conn_sec_update_t * sec;
	ble_gap_conn_sec_mode_t * secmode;
	int r;
#endif
	ble_gap_addr_t * addr;
	uint8_t * name;
	uint8_t len;

	switch (evt->header.evt_id) {
		case BLE_GAP_EVT_CONNECTED:
			ble_conn_handle = evt->evt.gap_evt.conn_handle;
			addr = &evt->evt.gap_evt.params.connected.peer_addr;
			memcpy (&ble_peer_addr, addr, sizeof(ble_gap_addr_t));
			ble_gap_role = evt->evt.gap_evt.params.connected.role;
#ifdef BLE_GAP_VERBOSE
			printf ("gap connected (handle: %x) ",
			    ble_conn_handle);
			printf ("peer: %x:%x:%x:%x:%x:%x ",
			    addr->addr[5], addr->addr[4], addr->addr[3],
			    addr->addr[2], addr->addr[1], addr->addr[0]);
			printf ("role; %d\r\n", ble_gap_role);
#endif
			if (ble_gap_role == BLE_GAP_ROLE_CENTRAL)
				bleGapScanStart ();
			orchardAppRadioCallback (connectEvent, evt, NULL, 0);
			break;

		case BLE_GAP_EVT_DISCONNECTED:
#ifdef BLE_GAP_VERBOSE
			printf ("gap disconnected...\r\n");
#endif
			bleGapStart ();
			orchardAppRadioCallback (disconnectEvent, evt,
			    NULL, 0);
			break;

		case BLE_GAP_EVT_AUTH_STATUS:
#ifdef BLE_GAP_VERBOSE
			printf ("gap auth status... (%d)\r\n",
			    evt->evt.gap_evt.params.auth_status.auth_status);
#endif
			break;

		case BLE_GAP_EVT_CONN_SEC_UPDATE:
#ifdef BLE_GAP_VERBOSE
			sec = &evt->evt.gap_evt.params.conn_sec_update;
			secmode = &sec->conn_sec.sec_mode;
			printf ("gap connections security update...\r\n");
			printf ("security mode: %d level: %d keylen: %d\r\n",
			    secmode->sm, secmode->lv,
			    sec->conn_sec.encr_key_size);
#endif
			break;

		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
			memset (&sec_keyset, 0, sizeof(sec_keyset));
			memset (&sec_params, 0, sizeof(sec_params));
		        sec_keyset.keys_own.p_pk  = &my_pk;
        		sec_keyset.keys_peer.p_pk = &peer_pk;
			sec_params.min_key_size = 7;
			sec_params.max_key_size = 16;
			sec_params.io_caps = BLE_GAP_IO_CAPS_NONE;
#ifdef BLE_GAP_VERBOSE
			r =
#endif
			sd_ble_gap_sec_params_reply (ble_conn_handle,
			    BLE_GAP_SEC_STATUS_SUCCESS, &sec_params,
			    &sec_keyset);
#ifdef BLE_GAP_VERBOSE
			printf ("gap security param request...(%d)\r\n", r);
#endif
			break;
		case BLE_GAP_EVT_ADV_REPORT:
			addr = &evt->evt.gap_evt.params.adv_report.peer_addr;
#ifdef BLE_GAP_VERBOSE
			printf ("GAP scan report...\r\n");
			printf ("peer: %x:%x:%x:%x:%x:%x rssi: %d\r\n",
			    addr->addr[5], addr->addr[4], addr->addr[3],
			    addr->addr[2], addr->addr[1], addr->addr[0],
			    evt->evt.gap_evt.params.adv_report.rssi);
#endif
			len = evt->evt.gap_evt.params.adv_report.dlen;
			name = evt->evt.gap_evt.params.adv_report.data;
			if (bleGapAdvBlockFind (&name, &len,
			    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME) !=
			    NRF_SUCCESS) {
				name = NULL;
				len = 0;
			}
			blePeerAdd (addr->addr, name, len,
			    evt->evt.gap_evt.params.adv_report.rssi);
			orchardAppRadioCallback (advertisementEvent, evt,
			    NULL, 0);
			break;
		case BLE_GAP_EVT_TIMEOUT:
			timeout = &evt->evt.gap_evt.params.timeout;
#ifdef BLE_GAP_VERBOSE
			printf ("GAP timeout event, src: %d\r\n",
			    timeout->src);
#endif
			if (timeout->src == BLE_GAP_TIMEOUT_SRC_ADVERTISING &&
			    ble_conn_handle == BLE_CONN_HANDLE_INVALID)
				bleGapAdvStart ();
			if (timeout->src == BLE_GAP_TIMEOUT_SRC_SCAN)
				bleGapScanStart ();
			if (timeout->src == BLE_GAP_TIMEOUT_SRC_CONN) {
				orchardAppRadioCallback (connectTimeoutEvent,
			 	     evt, NULL, 0);
				bleGapStart ();
			}
			break;

		default:
			printf ("invalid GAP event %d\r\n",
			   evt->header.evt_id);
			break;
	}

	return;
}

static uint8_t *
bleGapAdvBlockStart (uint8_t * size)
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

static uint32_t
bleGapAdvBlockAdd (void * pElem, uint8_t len, uint8_t etype,
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

static uint32_t
bleGapAdvBlockFinish (uint8_t * pkt, uint8_t len)
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
	adv_params.timeout = BLE_IDES_ADV_TIMEOUT;

	r = sd_ble_gap_adv_start (&adv_params, BLE_IDES_APP_TAG);

	chHeapFree (pkt);

	return (r);
}

uint32_t
bleGapAdvBlockFind (uint8_t ** pkt, uint8_t * len, uint8_t id)
{
	uint8_t * p;
	uint8_t l = 0;
	uint8_t t = 0;

	if (*len == 0 || *len > BLE_GAP_ADV_MAX_SIZE)
		return (NRF_ERROR_INVALID_PARAM);

	p = *pkt;

	while (p < (*pkt + *len)) {
		l = p[0];
		t = p[1];

		/* Sanity check. */

		if (l == 0 || l > BLE_GAP_ADV_MAX_SIZE)
			return (NRF_ERROR_INVALID_PARAM);

		if (t == id)
			break;
		else
			p += (l + 1);
	}

	if (t == id && l != 0) {
		*pkt = p + 2;
		*len = l - 1;
		return (NRF_SUCCESS);
	}

	return (NRF_ERROR_NOT_FOUND);
}

void
bleGapStart (void)
{
	ble_gap_conn_sec_mode_t perm;
	uint8_t * ble_name = (uint8_t *)BLE_NAME_IDES;

	ble_conn_handle = BLE_CONN_HANDLE_INVALID;

	sd_ble_gap_tx_power_set (4);

	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&perm);
	sd_ble_gap_device_name_set (&perm, ble_name,
	    strlen ((char *)ble_name));

	sd_ble_gap_appearance_set (BLE_APPEARANCE_DC26);

	bleGapAdvStart ();
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
	sparams.timeout = BLE_IDES_SCAN_TIMEOUT;
	sparams.interval = MSEC_TO_UNITS(1000, UNIT_0_625_MS);
	sparams.window = MSEC_TO_UNITS(50, UNIT_0_625_MS);

	cparams.min_conn_interval = MSEC_TO_UNITS(10, UNIT_1_25_MS);
	cparams.max_conn_interval = MSEC_TO_UNITS(10, UNIT_1_25_MS);
	cparams.slave_latency = 0;
	cparams.conn_sup_timeout = MSEC_TO_UNITS(400, UNIT_10_MS);

	r = sd_ble_gap_connect (peer, &sparams, &cparams, BLE_IDES_APP_TAG);

	if (r != NRF_SUCCESS) {
		printf ("GAP connect failed: %x\r\n", r);
		bleGapStart ();
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
