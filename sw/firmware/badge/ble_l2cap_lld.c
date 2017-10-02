#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "osal.h"
#include "chprintf.h"

#include "nrf_sdm.h"
#include "ble.h"
#include "ble_l2cap.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"

#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)

uint8_t ble_rx_buf[BLE_IDES_L2CAP_LEN];
uint16_t ble_local_cid;

static void bleL2CapReply (void);

void
bleL2CapDispatch (ble_evt_t * evt)
{
	ble_l2cap_evt_ch_setup_refused_t * refused;
	ble_l2cap_evt_ch_setup_request_t * request;
	ble_l2cap_evt_ch_setup_t * setup;
	ble_l2cap_evt_ch_rx_t * rx;
	ble_data_t rx_data;

	switch (evt->header.evt_id) {
		case BLE_L2CAP_EVT_CH_SETUP_REQUEST:
			printf ("L2CAP setup requested %x\r\n",
			    evt->evt.l2cap_evt.local_cid);
			ble_local_cid = evt->evt.l2cap_evt.local_cid;
			request = &evt->evt.l2cap_evt.params.ch_setup_request;
			printf ("MTU: %d peer MPS: %d "
			     "TX MPS: %d credits: %d ",
				request->tx_params.tx_mtu,
				request->tx_params.peer_mps,
				request->tx_params.tx_mps,
				request->tx_params.credits);
			printf ("PSM: %x\r\n", request->le_psm);
			bleL2CapReply ();
			break;

		case BLE_L2CAP_EVT_CH_SETUP_REFUSED:
                        refused = &evt->evt.l2cap_evt.params.ch_setup_refused;
			printf ("L2CAP setup refused: %x %x\r\n",
			    refused->source, refused->status);
			break;

		case BLE_L2CAP_EVT_CH_SETUP:
			printf ("L2CAP setup completed\r\n");
			ble_local_cid = evt->evt.l2cap_evt.local_cid;
			setup = &evt->evt.l2cap_evt.params.ch_setup;
			printf ("MTU: %d peer MPS: %d "
			     "TX MPS: %d credits: %d\r\n",
				setup->tx_params.tx_mtu,
				setup->tx_params.peer_mps,
				setup->tx_params.tx_mps,
				setup->tx_params.credits);
			break;

		case BLE_L2CAP_EVT_CH_RELEASED:
			printf ("L2CAP channel release\r\n");
			ble_local_cid = BLE_L2CAP_CID_INVALID;
			break;

		case BLE_L2CAP_EVT_CH_SDU_BUF_RELEASED:
			printf ("L2CAP channel SDU buffer released\r\n");
			break;

		case BLE_L2CAP_EVT_CH_CREDIT:
			printf ("L2CAP credit received\r\n");
			break;

		case BLE_L2CAP_EVT_CH_RX:
			printf ("L2CAP SDU received\r\n");
			rx = &evt->evt.l2cap_evt.params.rx;
			printf ("DATA RECEIVED: [%s]\r\n", rx->sdu_buf);
			rx_data.p_data = ble_rx_buf;
			rx_data.len = BLE_IDES_L2CAP_LEN;
			sd_ble_l2cap_ch_rx (ble_conn_handle,
			    evt->evt.l2cap_evt.local_cid, &rx_data);
			break;

		case BLE_L2CAP_EVT_CH_TX:
			printf ("L2CAP SDU transmitted\r\n");
			break;

		default:
			printf ("unknown L2CAP event: %x\r\n",
			    evt->header.evt_id);
			break;
	}

	return;
}

int
bleL2CapConnect (void)
{
	ble_l2cap_ch_setup_params_t params;
	uint16_t cid;
	int r;

	params.rx_params.rx_mtu = BLE_IDES_L2CAP_LEN;
	params.rx_params.rx_mps = BLE_IDES_L2CAP_LEN;
	params.rx_params.sdu_buf.p_data = ble_rx_buf;
	params.rx_params.sdu_buf.len = BLE_IDES_L2CAP_LEN;

	params.le_psm = BLE_IDES_PSM;
	params.status = 0;

	cid = BLE_L2CAP_CID_INVALID;

	r = sd_ble_l2cap_ch_setup (ble_conn_handle, &cid, &params);

	if (r != NRF_SUCCESS)
		printf ("L2CAP connect failed (%x)\r\n", r);

	return (r);
}

int
bleL2CapDisconnect (void)
{
	int r;

	r = sd_ble_l2cap_ch_release (ble_conn_handle, ble_local_cid);

	if (r != NRF_SUCCESS)
		printf ("L2CAP disconnect failed (%x)\r\n", r);

	return (r);
}

static void
bleL2CapReply (void)
{
	ble_l2cap_ch_setup_params_t params;
	int r;

	params.rx_params.rx_mtu = BLE_IDES_L2CAP_LEN;
	params.rx_params.rx_mps = BLE_IDES_L2CAP_LEN;
	params.rx_params.sdu_buf.p_data = ble_rx_buf;
	params.rx_params.sdu_buf.len = BLE_IDES_L2CAP_LEN;

	params.le_psm = BLE_IDES_PSM;
	params.status = 0;

	r = sd_ble_l2cap_ch_setup (ble_conn_handle, &ble_local_cid, &params);

	if (r != NRF_SUCCESS)
		printf ("L2CAP reply failed (%x)\r\n", r);

	return;
}

int
bleL2CapSend (char * str)
{
	ble_data_t data;
	int r;

	data.p_data = (void *)str;
	data.len = strlen (str) + 1;

	r = sd_ble_l2cap_ch_tx (ble_conn_handle, ble_local_cid, &data);

	if (r != NRF_SUCCESS)
		printf ("L2CAP tx failed (%x)\r\n", r);

	return (r);
}

void
bleL2CapStart (void)
{
	ble_local_cid = BLE_L2CAP_CID_INVALID;
	return;
}
