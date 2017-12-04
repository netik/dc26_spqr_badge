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

/*
 * This module implements the top-level support for the BLE radio in the
 * NRF52832 chip using the s132 SoftDevice stack. While it's possible to
 * access the radio directly, this only allows you to send and receive
 * raw packets. The SoftDevice includes Nordic's BLE stack. It's provided
 * as a binary blob which is linked with the rest of the badge code.
 *
 * Support for the different BLE functions is broken up into modules.
 * This module contains the top level SoftDevice initialization code and
 * dispatch loop. Separate modules are provided to handle GAP and L2CAP
 * features.
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

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_peer.h"

#include "badge.h"

uint8_t ble_station_addr[6];

static thread_reference_t sdThreadReference;
static ble_evt_t ble_evt;

/*
 * This symbol is created by the linker script. Its address is
 * the start of application RAM.
 */

extern uint32_t __ram0_start__;

void
nordic_fault_handler (uint32_t id, uint32_t pc, uint32_t info)
{
	return;
}

/******************************************************************************
*
* bleEventDispatch - main BLE event dispatcher
*
* This function is a helper routine for the BLE dispatch thread. Each event
* ID falls into a range: this functon will check the range and invoke the
* appropriate handler. Currently we mainly handle GAP and L2CAP events.
*
* RETURNS: N/A
*/

static void
bleEventDispatch (ble_evt_t * evt)
{
	if (evt->header.evt_id >= BLE_EVT_BASE &&
	    evt->header.evt_id <= BLE_EVT_LAST)
		printf ("common BLE event\r\n");

	if (evt->header.evt_id >= BLE_GAP_EVT_BASE &&
	    evt->header.evt_id <= BLE_GAP_EVT_LAST)
		bleGapDispatch (evt);

	if (evt->header.evt_id >= BLE_GATTC_EVT_BASE &&
	    evt->header.evt_id <= BLE_GATTC_EVT_LAST)
		printf ("GATT client event\r\n");

	if (evt->header.evt_id >= BLE_GATTS_EVT_BASE &&
	    evt->header.evt_id <= BLE_GATTS_EVT_LAST)
		printf ("GATT server event\r\n");

	if (evt->header.evt_id >= BLE_L2CAP_EVT_BASE &&
	    evt->header.evt_id <= BLE_L2CAP_EVT_LAST)
		bleL2CapDispatch (evt);

	return;
}

/******************************************************************************
*
* bleSdThread - SoftDevice event dispatch thread
*
* This function implements the SoftDevice event thread loop. It's woken
* up whenever the SoftDevice event interrupt triggers. There may be several
* events pending when the interrupt triggers, so this woutine must drain
* all events from the queue before sleeping again.
*
* RETURNS: N/A
*/

static THD_WORKING_AREA(waSdThread, 256);
static THD_FUNCTION(sdThread, arg)
{
	uint8_t * p_dest;
	uint16_t p_len;
	int r;

	(void)arg;
    
	chRegSetThreadName ("SDEvent");

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

/******************************************************************************
*
* Vector98 - SoftDevice interrupt handler
*
* This ISR is invoked whenever the SoftDevice triggers an event interrupt.
* It will wake up the event handler thread to process the event queue.
*
* RETURNS: N/A
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

/******************************************************************************
*
* bleStart() -- BLE radio driver startup routine
*
* This function initializes the BLE radio on the NRF52832 using the s132
* SoftDevice stack. The SoftDevice is stored in flash along with the badge
* OS image. Commands are sent to it using service call instructions. The
* SoftDevice also uses a portion of the NRF52 RAM. Exactly how much depends
* on some of the configuration done here.
*
* The SoftDevice requires an event handler, which is implemented using a
* separate thread that is started here. Once the thread is running, the
* SoftDevice can be started. Some GAP and L2CAP configuration parameters
* must be set before fully enabling BLE support.
*
* Once this is done, the GAP and L2CAP sub-modules are initialized as well.
*
* RETURNS: N/A
*/

void
bleStart (void)
{
	ble_gap_addr_t addr;

	/* Create SoftDevice event thread */

	chThdCreateStatic (waSdThread, sizeof(waSdThread),
	    NORMALPRIO + 10, sdThread, NULL);

	/* Set up SoftDevice ISR */

	nvicEnableVector (SD_EVT_IRQn, 5);

	/* Initialize peer list handling */

	blePeerStart ();

	/* Start and configure SoftDevice */

	bleEnable ();

	memset (&addr, 0, sizeof(addr));
	sd_ble_gap_addr_get (&addr);

	ble_station_addr[0] = addr.addr[5];
	ble_station_addr[1] = addr.addr[4];
	ble_station_addr[2] = addr.addr[3];
	ble_station_addr[3] = addr.addr[2];
	ble_station_addr[4] = addr.addr[1];
	ble_station_addr[5] = addr.addr[0];

	printf ("Station address: %x:%x:%x:%x:%x:%x\r\n",
	    ble_station_addr[0], ble_station_addr[1],
	    ble_station_addr[2], ble_station_addr[3],
	    ble_station_addr[4], ble_station_addr[5]);

	return;
}

void
bleEnable (void)
{
	int r;
 	uint32_t ram_start = (uint32_t)&__ram0_start__;
	nrf_clock_lf_cfg_t clock_source;
	ble_cfg_t cfg;

	/* Initialize the SoftDevice */

	memset (&clock_source, 0, sizeof(clock_source));
	clock_source.source = NRF_CLOCK_LF_SRC_XTAL;
	clock_source.accuracy = NRF_CLOCK_LF_ACCURACY_20_PPM;

	r = sd_softdevice_enable (&clock_source, nordic_fault_handler);

	if (r == NRF_SUCCESS)
		printf ("SoftDevice version %d.%d.%d enabled.\r\n",
		    SD_MAJOR_VERSION, SD_MINOR_VERSION, SD_BUGFIX_VERSION);
	else {
		printf ("Enabling softdevice failed (%x)\r\n", r);
		return;
	}

	/*
	 * Set up SoftDevice internal configuration to support
	 * L2CAP connections.
	 */

	memset (&cfg, 0, sizeof(cfg));

	cfg.conn_cfg.conn_cfg_tag = BLE_IDES_APP_TAG;
	cfg.conn_cfg.params.gap_conn_cfg.conn_count = 2;
	cfg.conn_cfg.params.gap_conn_cfg.event_length =
            BLE_GAP_EVENT_LENGTH_DEFAULT;

	r = sd_ble_cfg_set (BLE_CONN_CFG_GAP, &cfg, ram_start);

	memset (&cfg, 0, sizeof(cfg));

	cfg.conn_cfg.conn_cfg_tag = BLE_IDES_APP_TAG;
	cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 0;

	r = sd_ble_cfg_set (BLE_COMMON_CFG_VS_UUID, &cfg, ram_start);

	memset (&cfg, 0, sizeof(cfg));

	cfg.conn_cfg.conn_cfg_tag = BLE_IDES_APP_TAG;
	cfg.conn_cfg.params.l2cap_conn_cfg.rx_mps = BLE_IDES_L2CAP_LEN;
	cfg.conn_cfg.params.l2cap_conn_cfg.tx_mps = BLE_IDES_L2CAP_LEN;
	cfg.conn_cfg.params.l2cap_conn_cfg.rx_queue_size = 1;
	cfg.conn_cfg.params.l2cap_conn_cfg.tx_queue_size = 1;
	cfg.conn_cfg.params.l2cap_conn_cfg.ch_count = 2;

	r = sd_ble_cfg_set (BLE_CONN_CFG_L2CAP, &cfg, ram_start);

	/* Enable BLE support in SoftDevice */

	r = sd_ble_enable (&ram_start);

	if (r == NRF_SUCCESS)
		printf ("Bluetooth LE enabled. (RAM base: %x)\r\n",
		    ram_start);
	else {
		printf ("Enabling BLE failed (%x %x)\r\n", r, ram_start);
		return;
	}

	/* Initiallize GAP and L2CAP submodules */

	bleGapStart ();
	bleL2CapStart ();

	return;
}

void
bleDisable (void)
{
	int r;

	r = sd_softdevice_disable ();

	if (r == NRF_SUCCESS)
		printf ("Bluetooth LE disabled\r\n");
	else
		printf ("Disaabling BLE failed (%x)\r\n", r);

	return;
}
