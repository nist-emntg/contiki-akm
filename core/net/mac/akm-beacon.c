/*
 * akm-beacon.c
 *
 *  Created on: Jul 24, 2013
 *      Author: local
 */

#include <stdio.h>
#include "random.h"
#include "contiki.h"
#include "net/mac/nullmac.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include  "mac/akm-mac.h"
#include  "packetbuf.h"
#include "clock.h"
static clock_time_t get_beacon_interval();
/*---------------------------------------------------------------------------*/
void send_beacon() {
	AKM_PRINTF("send_beacon\n");
#if BENCH
	printf("bench: send_beacon\n");
#endif
	AKM_MAC_OUTPUT.data.beacon.is_authenticated = is_authenticated();
	AKM_MAC_OUTPUT.data.beacon.is_capacity_available = is_capacity_available(
			NULL);
	akm_send(ALL_NEIGHBORS, BEACON, sizeof(AKM_MAC_OUTPUT.data.beacon));
}



void handle_beacon(beacon_t *pbeacon) {
	nodeid_t* senderId = &AKM_DATA.sender_id;
	AKM_PRINTF("handle_beacon : "); AKM_PRINTADDR(senderId);
	if (AKM_DATA.is_dodag_root) {
		AKM_PRINTF("is_dodag_root = true.\n");
		int i = find_authenticated_neighbor(senderId);
		if (i == -1
				|| AKM_DATA.authenticated_neighbors[i].state
						== UNAUTHENTICATED) {
			send_challenge(senderId, pbeacon);
		}
	} else {
		AKM_PRINTF(
				"is_authenticated sender = %d is_part_of_dodag = %d sender auth state is %s\n",
				pbeacon->is_authenticated, is_part_of_dodag(),
				get_auth_state_as_string(get_authentication_state(senderId)));

		if (get_authentication_state(senderId) == AUTH_PENDING) {
			/* Already authenticated him so just resend the ack */
			nodeid_t* pparent = get_parent_id();
			/* Redundant parent is not available */
			AKM_PRINTF(
					"handle_beacon: node is AUTH_PENDING - resend ACK. parent is  \n"); AKM_PRINTADDR(pparent);
			/* Send the parent along to the child so he may identify
			 * a pair between which to insert himself.
			 */
			send_auth_ack(senderId, pparent);

		} else if (is_authenticated() && is_part_of_dodag()
				&& get_authentication_state(senderId) == UNAUTHENTICATED
				&& (!pbeacon->is_authenticated
						|| (pbeacon->is_capacity_available
								&& is_capacity_available(senderId)))) {
			send_challenge(senderId, pbeacon);
		}
	}

}
/*-----------------------------------------------------------------------*/
static void handle_beacon_timer(void* ptr) {
	AKM_PRINTF("handle_beacon_timer %d \n", is_authenticated());
	send_beacon();
	AKM_DATA.beacon_timer.interval = get_beacon_interval();
}
/*-------------------------------------------------------------------------*/
static clock_time_t get_beacon_interval() {
	/* Schedule a  beacon message */
	if (!is_authenticated()) {
		return BEACON_TIMER_INTERVAL;
	} else if (is_capacity_available(NULL) && !AKM_DATA.is_dodag_root) {
		return BEACON_TIMER_AUTH_INTERVAL;
	} else {
		return BEACON_TIMER_IDLE_INTERVAL;
	}
}
/*--------------------------------------------------------------------------*/
void reset_beacon(void) {
	AKM_PRINTF("reset_beacon\n");
	AKM_DATA.beacon_timer.timer_state = TIMER_STATE_RUNNING;
	//AKM_DATA.beacon_timer.current_count = 0;
	AKM_DATA.beacon_timer.interval = get_beacon_interval();
}

/*---------------------------------------------------------------------------*/

void schedule_beacon(void) {
	AKM_PRINTF("schedule_beacon %d\n", get_beacon_interval());
	send_beacon();
	akm_timer_set(&AKM_DATA.beacon_timer, get_beacon_interval(),
			handle_beacon_timer, NULL, 0, TTYPE_CONTINUOUS);
}

