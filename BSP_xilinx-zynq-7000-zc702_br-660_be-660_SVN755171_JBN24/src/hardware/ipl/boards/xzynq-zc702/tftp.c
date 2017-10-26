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

#include "tftp.h"

#define TFTP_OFFSET (XZYNQ_EMACPS_ETH_HDR_SIZE + XZYNQ_EMACPS_IP_HDR_SIZE + \
		     XZYNQ_EMACPS_UDP_HDR_SIZE)
#define UDP_OFFSET (XZYNQ_EMACPS_ETH_HDR_SIZE + XZYNQ_EMACPS_IP_HDR_SIZE)

size_t strlen(const char *fn)
{
	const char *sc;

	for (sc = fn; *sc != '\0'; ++sc)
		;
	return (sc - fn) + 1;
}

void emac_set_ip_header(int subsize)
{
	ip_hdr header;

	/* Construct the IP Datagram */
	header.version = 5;
	header.version |= (4 << 4);
	header.service = 0;
	header.length = htons(XZYNQ_EMACPS_IP_HDR_SIZE + subsize);
	header.identification = 0;
	header.fragment = 0;
	header.ttl = XZYNQ_EMACPS_IP_TTL;
	header.protocol = XZYNQ_EMACPS_IP_PROTO_UDP;
	header.crc = 0;

	/* Copy our ip address as source */
	mem_cpy(header.src_ip_addr, EMAC_IP_ADDR, 4);
	/* Copy the TFTP ip address as target */
	mem_cpy(header.dst_ip_addr, TFTP_IP_SERVER, 4);

	/* The IP datagram is just after the ethernet */
	mem_cpy(emac_tx_buffer + XZYNQ_EMACPS_ETH_HDR_SIZE,
		(uint8_t *)&header, XZYNQ_EMACPS_IP_HDR_SIZE);
}

void emac_set_udp_header(int subsize, uint16_t port_dst)
{
	udp_hdr header;

	/* Construct the UDP Datagram */
	header.port_src = htons(XZYNQ_EMACPS_TFTP_SRC_PORT);
	header.port_dst = htons(port_dst);
	header.crc = 0;
	header.length = htons(XZYNQ_EMACPS_UDP_HDR_SIZE + subsize);

	/* The UDP datagram is just after the IP */
	mem_cpy(emac_tx_buffer + XZYNQ_EMACPS_IP_HDR_SIZE +
		XZYNQ_EMACPS_ETH_HDR_SIZE, (uint8_t *)&header,
		XZYNQ_EMACPS_UDP_HDR_SIZE);
}

void emac_tftp_request_image(const char *fn)
{
	uint16_t op = htons(1);

	/* filename size + OP size + mode size */
#define TFTP_SIZE (strlen(fn) + 2 + 6 + 8 + 2 + 8 + 5)
	emac_set_eth_header(XZYNQ_EMACPS_ETH_TYPE_IP, EMAC_DST_MAC_ADDR);
	emac_set_ip_header(XZYNQ_EMACPS_UDP_HDR_SIZE + TFTP_SIZE);
	emac_set_udp_header(TFTP_SIZE, XZYNQ_EMACPS_TFTP_DST_PORT);

	/* Construct the TFTP Datagram */
	mem_cpy(emac_tx_buffer + TFTP_OFFSET, (uint8_t *)&op, 2);
	mem_cpy(emac_tx_buffer + TFTP_OFFSET + 2, (uint8_t *)fn, strlen(fn));
	/* byte transfert */
	mem_cpy(emac_tx_buffer + TFTP_OFFSET + 2 + strlen(fn), "octet", 6);
	/* Timeout of 5ms */
	mem_cpy(emac_tx_buffer + TFTP_OFFSET + 8 + strlen(fn), "timeout", 8);
	mem_cpy(emac_tx_buffer + TFTP_OFFSET + 16 + strlen(fn),
		__stringify(XZYNQ_EMACPS_TFTP_TIMEOUT), 2);
	/* Block size of XZYNQ_EMACPS_TFTP_BLK_SIZE bytes */
	mem_cpy(emac_tx_buffer + TFTP_OFFSET + 18 + strlen(fn), "blksize", 8);
	/* WARNING: we condiser FOUR numbers + \0 */
	mem_cpy(emac_tx_buffer + TFTP_OFFSET + 26 + strlen(fn),
		__stringify(XZYNQ_EMACPS_TFTP_BLK_SIZE), 5);

	emac_send(TFTP_OFFSET + TFTP_SIZE);
}

int emac_tftp_process()
{
	uint16_t op = ntohs(*((uint16_t *)emac_rx_buffer + (TFTP_OFFSET / 2)));
	uint16_t block =
		ntohs(*((uint16_t *)emac_rx_buffer + ((TFTP_OFFSET / 2) + 1)));
	uint16_t port =
		ntohs(*((uint16_t *)emac_rx_buffer + (UDP_OFFSET / 2)));
	uint16_t size =
		ntohs(*((uint16_t *)emac_rx_buffer + ((UDP_OFFSET / 2) + 2)));
	uint16_t port_dst =
		ntohs(*((uint16_t *)emac_rx_buffer + ((UDP_OFFSET / 2) + 1)));
	static int started = 0;
	static uint16_t last_blk = 0;

	/* if it's our port, we consider it as a TFTP message */
	if (port_dst != XZYNQ_EMACPS_TFTP_SRC_PORT)
		return 0;

	/*
	 * Size of the data is in the UDP header so we have to remove the UDP
	 * header and the OP size in the TFTP header
	 */
	size -= sizeof(udp_hdr);
	size -= 4;

	/* EMAC_DST_MAC_ADDR MUST contains the mac address of the TFTP server */
	switch (op) {
	case XZYNQ_EMACPS_TFTP_OP_DATA:
		if (started == 0) {
			ser_putstr("TFTP: Transfer error (transfer not started)\n");
			return -1;
		}

		/* TODO: Maybe another/previous TFTP transfer on the network */
		if ((block - 1) != last_blk) {
			return 0;
		}
		last_blk = block;

		/*
		 * DATA are just after the TFTP header (4 bytes)
		 * The offset to store the data are the
		 * BLK_SIZE * the block number
		 */
		mem_cpy((uint8_t *)(QNX_LOAD_ADDR +
				    (XZYNQ_EMACPS_TFTP_BLK_SIZE *
				     (block - 1))),
			(uint8_t *)emac_rx_buffer + TFTP_OFFSET + 4, size);

		emac_set_eth_header(XZYNQ_EMACPS_ETH_TYPE_IP,
				    EMAC_DST_MAC_ADDR);
		emac_set_ip_header(XZYNQ_EMACPS_UDP_HDR_SIZE + 4);
		emac_set_udp_header(4, port);

		/* Last block ? */
		if (size < XZYNQ_EMACPS_TFTP_BLK_SIZE) {
			ser_putstr("\nTFTP: Transfer done\n");
			emac_tftp_done = 1;
		}

		/* Construct the TFTP Datagram */
		op = htons(XZYNQ_EMACPS_TFTP_OP_ACK);
		block = htons(block);
		mem_cpy(emac_tx_buffer + TFTP_OFFSET, (uint8_t *)&op, 2);
		mem_cpy(emac_tx_buffer + TFTP_OFFSET + 2, (uint8_t *)&block, 2);

		/* Ack the block */
		emac_send(TFTP_OFFSET + 4);

		/* Progress bar */
		if (!(last_blk % TFTP_CHAR_OCCURENCE))
			ser_putstr("#");
		break;

	case XZYNQ_EMACPS_TFTP_OP_NEGO:
		block = 0;
		started = 1;
		last_blk = 0;

		emac_set_eth_header(XZYNQ_EMACPS_ETH_TYPE_IP,
				    EMAC_DST_MAC_ADDR);
		emac_set_ip_header(XZYNQ_EMACPS_UDP_HDR_SIZE + 4);
		emac_set_udp_header(4, port);

		/* Construct the TFTP Datagram */
		op = htons(XZYNQ_EMACPS_TFTP_OP_ACK);
		mem_cpy(emac_tx_buffer + TFTP_OFFSET, (uint8_t *)&op, 2);
		mem_cpy(emac_tx_buffer + TFTP_OFFSET + 2, (uint8_t *)&block,
			2);

		/* Ack the block */
		emac_send(TFTP_OFFSET + 4);

		emac_tftp_started = 1;

		break;

	default:
		/* errors: we are not supposed to received anything else */
		started = 0;
		block = 0;
		last_blk = 0;
		ser_putstr("TFTP: Transfer error\n");
		return -1;
	}

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
