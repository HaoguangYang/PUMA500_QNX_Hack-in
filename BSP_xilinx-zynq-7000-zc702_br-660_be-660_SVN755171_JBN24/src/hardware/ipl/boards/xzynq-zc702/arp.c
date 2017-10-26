/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems.
 * Copyright 2013, Adeneo Embedded.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include "arp.h"

void emac_arp_reply(arp_hdr *header)
{
	arp_hdr arp;

	/* Set the ethernet header */
	emac_set_eth_header(XZYNQ_EMACPS_ETH_TYPE_ARP,
			    header->sender_hardware_addr);

	/* Prepare the arp header with an reply operation */
	arp.operation = htons(XZYNQ_EMACPS_ARP_OP_REPLY);
	arp.hardware_type = htons(XZYNQ_EMACPS_ARP_HW_TYPE_ETH);
	arp.protocol_type = htons(XZYNQ_EMACPS_ETH_TYPE_IP);
	arp.hardware_addr_length = XZYNQ_EMACPS_MAC_SIZE;
	arp.protocol_addr_length = XZYNQ_EMACPS_IP_SIZE;

	/* Set the target to the sender */
	mem_cpy(arp.sender_hardware_addr, EMAC_MAC_ADDR, XZYNQ_EMACPS_MAC_SIZE);
	mem_cpy(arp.target_hardware_addr, header->sender_hardware_addr,
		XZYNQ_EMACPS_MAC_SIZE);
	mem_cpy(arp.sender_protocol_addr, EMAC_IP_ADDR, XZYNQ_EMACPS_IP_SIZE);
	mem_cpy(arp.target_protocol_addr, header->sender_protocol_addr,
		XZYNQ_EMACPS_IP_SIZE);

	/* Copy the ARP header just after the ethernet header */
	mem_cpy(emac_tx_buffer + XZYNQ_EMACPS_ETH_HDR_SIZE, (uint8_t *)&arp,
		XZYNQ_EMACPS_ARP_HDR_SIZE);

	emac_send(XZYNQ_EMACPS_ETH_HDR_SIZE + XZYNQ_EMACPS_ARP_HDR_SIZE);
}

void emac_arp_request(uint8_t *who)
{
	arp_hdr arp;
	uint8_t bc_mac[6]; //  = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	bc_mac[0] = 0xFF;
	bc_mac[1] = 0xFF;
	bc_mac[2] = 0xFF;
	bc_mac[3] = 0xFF;
	bc_mac[4] = 0xFF;
	bc_mac[5] = 0xFF;

	/* Set the ethernet header to broadcast the message */
	emac_set_eth_header(XZYNQ_EMACPS_ETH_TYPE_ARP, bc_mac);

	/* Prepare the arp header with an request operation */
	arp.operation = htons(XZYNQ_EMACPS_ARP_OP_REQUEST);
	arp.hardware_type = htons(XZYNQ_EMACPS_ARP_HW_TYPE_ETH);
	arp.protocol_type = htons(XZYNQ_EMACPS_ETH_TYPE_IP);
	arp.hardware_addr_length = XZYNQ_EMACPS_MAC_SIZE;
	arp.protocol_addr_length = XZYNQ_EMACPS_IP_SIZE;

	/* Set the target to everyone */
	mem_cpy(arp.sender_hardware_addr, EMAC_MAC_ADDR, XZYNQ_EMACPS_MAC_SIZE);
	mem_cpy(arp.target_hardware_addr, bc_mac, XZYNQ_EMACPS_MAC_SIZE);
	mem_cpy(arp.sender_protocol_addr, EMAC_IP_ADDR, XZYNQ_EMACPS_IP_SIZE);
	mem_cpy(arp.target_protocol_addr, who, XZYNQ_EMACPS_IP_SIZE);

	/* Copy the ARP header just after the ethernet header */
	mem_cpy(emac_tx_buffer + XZYNQ_EMACPS_ETH_HDR_SIZE, (uint8_t *)&arp,
		XZYNQ_EMACPS_ARP_HDR_SIZE);

	emac_send(XZYNQ_EMACPS_ETH_HDR_SIZE + XZYNQ_EMACPS_ARP_HDR_SIZE);
}

int emac_arp_process()
{
	arp_hdr *header =
		(arp_hdr *)(emac_rx_buffer + XZYNQ_EMACPS_ETH_HDR_SIZE);
	int i;

	switch (ntohs(header->operation)) {
	case XZYNQ_EMACPS_ARP_OP_REQUEST:

		/* Reply only if it's our MAC address */
		for (i = 0; i < XZYNQ_EMACPS_MAC_SIZE; i++)
			if (header->target_hardware_addr[i] != EMAC_MAC_ADDR[i])
				return 0;

		emac_arp_reply(header);
		break;
	case XZYNQ_EMACPS_ARP_OP_REPLY:

		/* Check if the source address is our MAC */
		for (i = 0; i < XZYNQ_EMACPS_MAC_SIZE; i++) {
			if (emac_rx_buffer[i] != EMAC_MAC_ADDR[i]) {
				return 0;
			}
		}

		/* Stock the MAC address */
		if (header->hardware_addr_length == XZYNQ_EMACPS_MAC_SIZE &&
		    ntohs(header->protocol_type) == XZYNQ_EMACPS_ETH_TYPE_IP) {
			for (i = 0; i < XZYNQ_EMACPS_MAC_SIZE; i++)
				EMAC_DST_MAC_ADDR[i] =
					header->sender_hardware_addr[i];

			emac_arp_done = 1;
		}

		break;
	default:
		/* errors */
		return -1;
	}

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
