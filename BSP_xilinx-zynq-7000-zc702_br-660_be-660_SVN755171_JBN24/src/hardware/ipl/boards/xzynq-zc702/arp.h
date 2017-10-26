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

#ifndef _IPL_ARP_H
#define _IPL_ARP_H

#include <stdint.h>
#include <arpa/inet.h>      /* for htons/htonl stuff */
#include "ipl.h"
#include "ipl_xzynq.h"
#include "emac.h"

#define XZYNQ_EMACPS_ARP_HDR_SIZE    sizeof(arp_hdr)
#define XZYNQ_EMACPS_ARP_OP_REQUEST  1
#define XZYNQ_EMACPS_ARP_OP_REPLY    2
#define XZYNQ_EMACPS_ARP_HW_TYPE_ETH 1
typedef struct {
	uint16_t hardware_type;
	uint16_t protocol_type;
	uint8_t hardware_addr_length;
	uint8_t protocol_addr_length;
	uint16_t operation;
	uint8_t sender_hardware_addr[XZYNQ_EMACPS_MAC_SIZE];
	uint8_t sender_protocol_addr[XZYNQ_EMACPS_IP_SIZE];
	uint8_t target_hardware_addr[XZYNQ_EMACPS_MAC_SIZE];
	uint8_t target_protocol_addr[XZYNQ_EMACPS_IP_SIZE];
} __packed arp_hdr;

int emac_arp_process();
void emac_arp_reply(arp_hdr *header);
void emac_arp_request(uint8_t *who);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
