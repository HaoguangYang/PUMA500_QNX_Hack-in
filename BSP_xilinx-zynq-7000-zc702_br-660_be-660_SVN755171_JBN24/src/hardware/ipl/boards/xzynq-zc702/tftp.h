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

#ifndef _IPL_TFPT_H
#define _IPL_TFTP_H

#include <stdint.h>
#include <arpa/inet.h>          /* for htons/htonl stuff */
#include "ipl_xzynq.h"
#include "ipl.h"
#include "emac.h"

#define TFTP_CHAR_OCCURENCE			75

#define XZYNQ_EMACPS_TFTP_DST_PORT  69
#define XZYNQ_EMACPS_TFTP_SRC_PORT  3434        /* Should be random */
#define XZYNQ_EMACPS_TFTP_OP_RRQ    1
#define XZYNQ_EMACPS_TFTP_OP_WRQ    2
#define XZYNQ_EMACPS_TFTP_OP_DATA   3
#define XZYNQ_EMACPS_TFTP_OP_ACK    4
#define XZYNQ_EMACPS_TFTP_OP_ERROR  5
#define XZYNQ_EMACPS_TFTP_OP_NEGO   6
/* Ethernet MTU, less the TFTP, UDP and IP header lengths */
#define XZYNQ_EMACPS_TFTP_BLK_SIZE  1428
#define XZYNQ_EMACPS_TFTP_TIMEOUT   5

#define XZYNQ_EMACPS_UDP_HDR_SIZE sizeof(udp_hdr)
typedef struct {
	uint16_t port_src;
	uint16_t port_dst;
	uint16_t length;
	uint16_t crc;
} __packed udp_hdr;

#define XZYNQ_EMACPS_IP_HDR_SIZE   sizeof(ip_hdr)
#define XZYNQ_EMACPS_IP_TTL        128
#define XZYNQ_EMACPS_IP_PROTO_UDP  17
typedef struct {
	uint8_t version;
	uint8_t service;
	uint16_t length;
	uint16_t identification;
	uint16_t fragment;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t crc;
	uint8_t src_ip_addr[XZYNQ_EMACPS_IP_SIZE];
	uint8_t dst_ip_addr[XZYNQ_EMACPS_IP_SIZE];
} __packed ip_hdr;

void emac_tftp_request_image(const char *fn);
int emac_tftp_process();

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
