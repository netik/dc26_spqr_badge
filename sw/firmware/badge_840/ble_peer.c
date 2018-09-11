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
#include "ble_peer.h"

#include "badge.h"

ble_peer_entry ble_peer_list[BLE_PEER_LIST_SIZE];

static mutex_t peer_mutex;

static THD_WORKING_AREA(waPeerThread, 32);
static THD_FUNCTION(peerThread, arg)
{
	ble_peer_entry * p;
	int i;
	(void)arg;
    
	chRegSetThreadName ("PeerEvent");

	while (1) {
		chThdSleepMilliseconds (1000);

		osalMutexLock (&peer_mutex);

		for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
			p = &ble_peer_list[i];

			if (p->ble_used == 0)
				continue;

			p->ble_ttl--;
			/* If this entry timed out, nuke it */
			if (p->ble_ttl == 0)
				memset (p, 0, sizeof(ble_peer_entry));
		}

		osalMutexUnlock (&peer_mutex);
	}

	/* NOTREACHED */
	return;
}

void
blePeerAdd (uint8_t * peer_addr, uint8_t * name, uint8_t len, int8_t rssi)
{
	ble_peer_entry * p;
	int firstfree = -1;
	int i;

	osalMutexLock (&peer_mutex);

	/* Check for duplicates */

	for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
		p = &ble_peer_list[i];
		if (p->ble_used == 0 && firstfree == -1)
			firstfree = i;

		if (memcmp (peer_addr, p->ble_peer_addr, 6) == 0) {
			p->ble_ttl = BLE_PEER_LIST_TTL;
			p->ble_rssi = rssi;
			osalMutexUnlock (&peer_mutex);
			return;
		}
	}

	/* Not a duplicate, but there's  more room for new peers. :( */

	if (firstfree == -1) {
		osalMutexUnlock (&peer_mutex);
		return;
	}

	/* New entry and we have a free slot; populate it. */

	p = &ble_peer_list[firstfree];

	memcpy (p->ble_peer_addr, peer_addr, sizeof(ble_peer_addr));
	if (name == NULL)
		memcpy (p->ble_peer_name, "<none>", 7);
	else
		memcpy (p->ble_peer_name, name, len);
	p->ble_rssi = rssi;
	p->ble_ttl = BLE_PEER_LIST_TTL;
	p->ble_used = 1;

	osalMutexUnlock (&peer_mutex);

	return;
}

void
blePeerShow (void)
{
	ble_peer_entry * p;
	int i;

	osalMutexLock (&peer_mutex);
	for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
		p = &ble_peer_list[i];

		if (p->ble_used == 0)
			continue;

		printf ("[%02x:%02x:%02x:%02x:%02x:%02x] ",
		    p->ble_peer_addr[5], p->ble_peer_addr[4],
		    p->ble_peer_addr[3], p->ble_peer_addr[2],
		    p->ble_peer_addr[1], p->ble_peer_addr[0]);
		printf ("[%s] ", p->ble_peer_name);
		printf ("[%d] ", p->ble_rssi);
		printf ("[%d]\r\n", p->ble_ttl);
	}

	osalMutexUnlock (&peer_mutex);

	return;
}

void
blePeerStart (void)
{
	memset (ble_peer_list, 0, sizeof(ble_peer_list));
	osalMutexObjectInit (&peer_mutex);

	chThdCreateStatic (waPeerThread, sizeof(waPeerThread),
	    NORMALPRIO, peerThread, NULL);

	return;
}
