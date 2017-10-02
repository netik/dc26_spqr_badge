#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ble.h"
#include "ble_gap.h"
#include "ble_l2cap.h"

#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"

static void
radio_disconnect (BaseSequentialStream *chp, int argc, char *argv[])
{
	bleGapDisconnect ();
	return;
}

static void
radio_connect (BaseSequentialStream *chp, int argc, char *argv[])
{
	ble_gap_addr_t peer;
	unsigned int addr[6];

	memset (&peer, 0, sizeof(peer));

	if (argc != 2) {
		chprintf (chp, "No peer specified\r\n");
		return;
	}

	sscanf (argv[1], "%x:%x:%x:%x:%x:%x",
	    &addr[5], &addr[4], &addr[3], &addr[2], &addr[1], &addr[0]);

	peer.addr_id_peer = TRUE;
	peer.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
	peer.addr[0] = addr[0];
	peer.addr[1] = addr[1];
	peer.addr[2] = addr[2];
	peer.addr[3] = addr[3];
	peer.addr[4] = addr[4];
	peer.addr[5] = addr[5];

	bleGapConnect (&peer);

	return;
}

static void
radio_l2capconnect (BaseSequentialStream *chp, int argc, char *argv[])
{
	bleL2CapConnect ();
	return;
}

static void
radio_l2capdisconnect (BaseSequentialStream *chp, int argc, char *argv[])
{
	bleL2CapDisconnect ();
	return;
}

static void
radio_send (BaseSequentialStream *chp, int argc, char *argv[])
{
	if (argc != 2) {
		chprintf (chp, "No message specified\r\n");
		return;
	}

	bleL2CapSend (argv[1]);
	return;
}

void
cmd_radio (BaseSequentialStream *chp, int argc, char *argv[])
{

	if (argc == 0) {
		chprintf(chp, "Radio commands:\r\n");
		chprintf(chp, "connect [addr]       Connect to peer\r\n");
		chprintf(chp, "disconnect           Disconnect from peer\r\n");
		chprintf(chp, "l2capconnect         Create L2CAP channel\r\n");
		chprintf(chp, "send [msg]           Transmit to peer\r\n");
	}

	if (strcmp (argv[0], "connect") == 0)
		radio_connect (chp, argc, argv);
	else if (strcmp (argv[0], "disconnect") == 0)
		radio_disconnect (chp, argc, argv);
	else if (strcmp (argv[0], "l2capconnect") == 0)
		radio_l2capconnect (chp, argc, argv);
	else if (strcmp (argv[0], "l2capdisconnect") == 0)
		radio_l2capdisconnect (chp, argc, argv);
	else if (strcmp (argv[0], "send") == 0)
		radio_send (chp, argc, argv);
	else
		chprintf(chp, "Unrecognized radio command\r\n");

	return;
}
