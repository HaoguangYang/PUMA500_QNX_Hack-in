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

#ifndef XZYNQ_EMACPS_H          /* prevent circular inclusions */
#define XZYNQ_EMACPS_H          /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#define RXBD_CNT 32
#define TXBD_CNT 2

#define XZYNQ_EMACPS_MAX_MAC_ADDR         4     /**< Maxmum number of mac address
	                                                                         supported */
#define XZYNQ_EMACPS_BD_ALIGNMENT         4     /**< Minimum buffer descriptor alignment
	                                                                         on the local bus */
#define XZYNQ_EMACPS_RX_BUF_ALIGNMENT 4         /**< Minimum buffer alignment when using
	                                                                                 options that impose alignment
	                                                                                 restrictions on the buffer data on
	                                                                                 the local bus */

#define XZYNQ_EMACPS_SEND                1      /**< send direction */
#define XZYNQ_EMACPS_RECV                2      /**< receive direction */

/* Erros constants */
#define SUCCESS 0
#define INVALID_PARAM 1
#define DMA_SG_LIST_ERROR 2
#define DMA_SG_NO_LIST 3
#define FAILURE 4
#define EMAC_MII_BUSY 5

#include <stdint.h>
#include <hw/inout.h>
#include <arpa/inet.h>          /* for htons/htonl stuff */

#include "ipl_xzynq.h"
#include "qnx_types.h"
#include "emac_hw.h"
#include "emac_bd.h"
#include "emac_bdring.h"

/* Memory Address of the starting queue RX/TX in RAM */
#define XZYNQ_EMACPS_RX_BD_START        0x16C00000
#define XZYNQ_EMACPS_TX_BD_START        0x16D00000

/* For the moment the PHY address is hard-coded */
#define XZYNQ_EMACPS_PHY_ADDR 7

/* The next few constants help upper layers determine the size of memory
 * pools used for Ethernet buffers and descriptor lists.
 */
#define XZYNQ_EMACPS_MAC_ADDR_SIZE       6              /* size of Ethernet header */

#define XZYNQ_EMACPS_MTU                         1500   /* max MTU size of Ethernet frame */
#define XZYNQ_EMACPS_HDR_SIZE            14             /* size of Ethernet header */
#define XZYNQ_EMACPS_HDR_VLAN_SIZE       18             /* size of Ethernet header with VLAN */
#define XZYNQ_EMACPS_TRL_SIZE            4              /* size of Ethernet trailer (FCS) */
#define XZYNQ_EMACPS_MAX_FRAME_SIZE               (XZYNQ_EMACPS_MTU + XZYNQ_EMACPS_HDR_SIZE + \
						   XZYNQ_EMACPS_TRL_SIZE)
#define XZYNQ_EMACPS_MAX_VLAN_FRAME_SIZE  (XZYNQ_EMACPS_MTU + XZYNQ_EMACPS_HDR_SIZE + \
					   XZYNQ_EMACPS_HDR_VLAN_SIZE + XZYNQ_EMACPS_TRL_SIZE)

/**
 * The emac driver instance data. The user is required to allocate a
 * structure of this type for every emac device in the system. A pointer
 * to a structure of this type is then passed to the driver API functions.
 */
typedef struct emac {
	uint32_t base_address;
	/**< Physical base address of IPIF registers */

	emac_bdring tx_bdring;          /* Transmit BD ring */
	emac_bdring rx_bdring;          /* Receive BD ring */
} emac;

/*
 * Header of an ethernet frame
 */
#define XZYNQ_EMACPS_ETH_HDR_SIZE       sizeof(ethernet_hdr)
#define XZYNQ_EMACPS_ETH_TYPE_ARP       0x806
#define XZYNQ_EMACPS_ETH_TYPE_IP        0x800
#define XZYNQ_EMACPS_MAC_SIZE           6
#define XZYNQ_EMACPS_IP_SIZE            4
typedef struct {
	uint8_t dmac[6];
	uint8_t smac[6];
	uint16_t emac_type;
} __packed ethernet_hdr;

#define emac_get_tx_ring(emac) ((emac)->tx_bdring)
#define emac_get_rx_ring(emac) ((emac)->rx_bdring)

#define emac_int_enable(emac, mask)					 \
	emac_write_reg((emac)->base_address,			 \
		       XZYNQ_EMACPS_IER_OFFSET,						\
		       (mask & XZYNQ_EMACPS_IXR_ALL_MASK));

#define emac_int_disable(emac, mask)				\
	emac_write_reg((emac)->base_address,			\
		       XZYNQ_EMACPS_IDR_OFFSET,					       \
		       (mask & XZYNQ_EMACPS_IXR_ALL_MASK));

#define emac_transmit(emac)							\
	emac_write_reg(emac->base_address,				\
		       XZYNQ_EMACPS_NWCTRL_OFFSET,				       \
		       (emac_read_reg(emac->base_address,		       \
				      XZYNQ_EMACPS_NWCTRL_OFFSET) | XZYNQ_EMACPS_NWCTRL_STARTTX_MASK))

/*
 * Initialization functions in xemacps.c
 */
int emac_init();
void emac_fini();
void emac_start(emac *emac);
void emac_stop(emac *emac);
void emac_reset(emac *emac);

int emac_set_mac_address(emac *emac, void *address_ptr, uint8_t index);
void emac_get_mac_address(emac *emac, void *address_ptr,
			  uint8_t index);

void emac_set_operating_speed(emac *emac, uint16_t speed);
uint16_t emac_get_operating_speed(emac *emac);

int emac_phy_read(emac *emac, uint32_t phy_address,
		  uint32_t register_num, uint16_t *phy_data_ptr);
int emac_phy_write(emac *emac, uint32_t phy_address,
		   uint32_t register_num, uint16_t phy_data);

int emac_process_rx();
void emac_send(int size);
void emac_set_eth_header(uint16_t type, uint8_t *dmac);
int emac_wait_for(uint8_t *cond, int timeout);

/*
 * Global Variables
 */
extern uint8_t *emac_rx_buffers;        /* Buffers of all received frames */
extern uint8_t *emac_rx_buffer;         /* Data of the current received frame */
extern uint8_t *emac_tx_buffer;         /* Data of the frame to be send */
extern emac eth0;                       /* Instance to the first MAC controller */

/*
 * Global variables use as condition to wait on some data.
 */
extern uint8_t emac_arp_done;
extern uint8_t emac_tftp_done;
extern uint8_t emac_tftp_started;

/*
 * MAC Address of the TFTP server
 */
extern uint8_t EMAC_DST_MAC_ADDR[6];

/*
 * MAC Address of our interface
 */
extern uint8_t EMAC_MAC_ADDR[6];
extern uint8_t EMAC_IP_ADDR[4];

static inline void mem_cpy(void *d, void *s, unsigned long size)
{
	copy((unsigned long)d, (unsigned long)s, size);
}

#ifdef __cplusplus
}
#endif
#endif                          /* end of protection macro */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
