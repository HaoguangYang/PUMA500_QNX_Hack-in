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
#include "emac.h"
#include "tftp.h"
#include "timer.h"

/*
 * Global Variables
 */
uint8_t *emac_rx_buffers = (uint8_t *)XZYNQ_EMACPS_RX_BD_START + 0x1000;
/* Data of the received frame */
uint8_t *emac_rx_buffer;
/* Data of the received frame */
uint8_t *emac_tx_buffer = (uint8_t *)XZYNQ_EMACPS_TX_BD_START + 0x1000;
/* Instance to the first MAC controller */
emac eth0;

uint8_t emac_arp_done;
uint8_t emac_tftp_done;
uint8_t emac_tftp_started;

/*
 * MAC Address of our interface
 */
uint8_t EMAC_MAC_ADDR[6] = { 0x00, 0x0A, 0x35, 0x02, 0x78, 0x10 };
uint8_t EMAC_IP_ADDR[4] = { 10, 1, 1, 2 };

/*
 * MAC Address of the TFTP server
 */
uint8_t EMAC_DST_MAC_ADDR[6] = { 0 };

static int emac_phy_init()
{
	uint16_t tmp16;
	uint32_t link_speed, tmp;

	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 22, 0);

	/* Control register - MAC */
	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 22, 2);
	emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 21, &tmp16);
	tmp16 |= (1 << 5);      /* RGMII receive timing transition when data stable */
	tmp16 |= (1 << 4);      /* RGMII transmit clock internally delayed */
	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 21, tmp16);
	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 22, 0);

	/* Control register */
	emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 0, &tmp16);
	tmp16 |= (1 << 12);     /* auto-negotiation enable */
	tmp16 |= (1 << 8);      /* enable full duplex */
	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 0, tmp16);

	/* link speed advertisement for autonegotiation */
	emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 4, &tmp16);
	tmp16 |= (1 << 8) | (1 << 7);   /* enable 100Mbps */
	tmp16 |= (1 << 6) | (1 << 5);   /* enable 10 Mbps */
	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 4, tmp16);

	/* enable gigabit advertisement */
	emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 9, &tmp16);
	tmp16 |= (1 << 9) | (1 << 8);
	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 9, tmp16);

	/* Reset the PHY */
	emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 0, &tmp16);
	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 0, tmp16 | 0x8000);
	do {
		emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 0, &tmp16);
	} while (tmp16 & 0x8000);

	ser_putstr("Starting auto-negotiation ... ");
	/* Auto-neogiciation: 8s max */
	timer_set(8000);
	do {
		emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 19, &tmp16);
		if ((tmp16 >> 15) & 1) {
			ser_putstr("ERROR\n");
			/* Auto-negociation error */
			return -1;
		}
		emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 1, &tmp16);
	} while (!(tmp16 & (1 << 5)) && timer_remaining() > 0);

	/* Timeout ... */
	if (!(tmp16 & (1 << 5))) {
		ser_putstr("ERROR\n");
		/* Auto-negociation error */
		return -1;
	}

	ser_putstr("DONE\n");
	emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 22, 0);
	emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 17, &tmp16);
	if (((tmp16 >> 10) & 1))
		emac_phy_write(&eth0, XZYNQ_EMACPS_PHY_ADDR, 22, 0);
	else {
		/* Link is not up */
		ser_putstr("Link is not up\n");
		return -1;
	}

	/* Get the link speed negociated from the PHY */
	emac_phy_read(&eth0, XZYNQ_EMACPS_PHY_ADDR, 17, &tmp16);
	if (((tmp16 >> 14) & 3) == 2)
		link_speed = 1000;
	else if (((tmp16 >> 14) & 3) == 1)
		link_speed = 100;
	else
		link_speed = 10;

	/* Advertise the MAC controller with the new speed */
	tmp = in32(eth0.base_address + XZYNQ_EMACPS_NWCFG_OFFSET);
	if (link_speed == 10)
		tmp &= ~(0x1);          /* enable 10Mbps */
	else
		tmp |= 0x1;             /* enable 100Mbps */
	if (link_speed == 1000)
		tmp |= 0x400;           /* enable 1000Mbps */
	else
		tmp &= ~(0x400);        /* disable gigabit */
	out32(eth0.base_address + XZYNQ_EMACPS_NWCFG_OFFSET, tmp);

	emac_set_operating_speed(&eth0, link_speed);

	/* Setup the clock */
	out32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Configure GEM0_RCLK_CTRL */
	out32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_GEM0_RCLK_CTRL_REG, ((0 << 4) | (1 << 0)));

	/* 125MHz */
	if (link_speed == 1000) {
		out32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_GEM0_CLK_CTRL_REG,
				((1 << 20) | (8 << 8) | (0 << 4) | (1 << 0)));
		ser_putstr("Speed is 1000Mbits\n");
	}
	/* 25 MHz */
	else if (link_speed == 100) {
		out32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_GEM0_CLK_CTRL_REG,
				((1 << 20) | (40 << 8) | (0 << 4) | (1 << 0)));
		ser_putstr("Speed is 100Mbits\n");
	}
	/* 2.5 Mhz */
	else {
		out32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_GEM0_CLK_CTRL_REG,
				((10 << 20) | (40 << 8) | (0 << 4) | (1 << 0)));
		ser_putstr("Speed is 10Mbits\n");
	}

	/* SLCR lock */
//	out32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	/* Some delay before using the PHY */
	msleep(100);

	return 0;
}

int emac_init()
{
	emac_bd bd_template;

	/* Set device base address and ID */
	eth0.base_address = XZYNQ_EMAC0_BASE;
	emac_reset(&eth0);

	/* Set the MAC Address */
	emac_set_mac_address(&eth0, EMAC_MAC_ADDR, 1);

	/* Intialize RX ring buffer */
	emac_bd_clear(&bd_template);
	emac_bdring_create(&(emac_get_rx_ring(&eth0)), XZYNQ_EMACPS_RX_BD_START,
			XZYNQ_EMACPS_RX_BD_START, XZYNQ_EMACPS_BD_ALIGNMENT,
			RXBD_CNT);

	emac_bdring_clone(&(emac_get_rx_ring(&eth0)),
			&bd_template, XZYNQ_EMACPS_RECV);

	/* Intialize TX ring buffer */
	emac_bd_clear(&bd_template);
	emac_bd_set_status(&bd_template, XZYNQ_EMACPS_TXBUF_USED_MASK);
	emac_bdring_create(&(emac_get_tx_ring(&eth0)), XZYNQ_EMACPS_TX_BD_START,
			XZYNQ_EMACPS_TX_BD_START, XZYNQ_EMACPS_BD_ALIGNMENT,
			TXBD_CNT);

	emac_bdring_clone(&(emac_get_tx_ring(&eth0)),
			&bd_template, XZYNQ_EMACPS_SEND);

	/*
	 * Configure the DMA
	 * Bits 4-0: To set AHB fixed burst length for DMA data operations ->
	 *           Set with binary 010 00100 to use INCR4 AHB bursts.
	 * Bits 9-8: Receiver packet buffer memory size ->
	 *       Set with binary 11 to Use full configured addressable space
	 * Bit 11  : Auto checksum computation
	 * Bit 10  : Transmitter packet buffer memory size ->
	 *       Set with binary 1 to Use full configured addressable space
	 * Bits 23-16 : DMA receive buffer size in AHB system memory ->
	 *   Set with binary 00011000 to use 1536 byte (1*max length/buffer).
	 */
	out32(eth0.base_address + XZYNQ_EMACPS_DMACR_OFFSET, 0x00180F04);

	/*
	 * MAC Setup
	 * Following is the setup for Network Configuration register.
	 * Bit 0:  Set for 100 Mbps operation.
	 * Bit 1:  Set for Full Duplex mode.
	 * Bit 4:  Set to allow Copy all frames.
	 * Bit 17: Set for FCS removal.
	 * Bits 20-18: Set with value binary 010 to divide pclk by 32
	 *             (pclk up to 80 MHz)
	 */
	/* By default we configure it at the highest speed */
	out32(eth0.base_address + XZYNQ_EMACPS_NWCFG_OFFSET,
	      0x000A0003 | XZYNQ_EMACPS_NWCFG_1000_MASK);

	/*
	 * Initialize the PHY by auto-negociation
	 */
	if (emac_phy_init() != 0)
		return -1;
	return 0;
}

int emac_wait_for(uint8_t *cond, int timeout)
{
	uint32_t status_rx;

	/* Configure the timer */
	timer_set(timeout);

	while (!(*cond)) {
		/* Clear the status */
		status_rx = in32(eth0.base_address + XZYNQ_EMACPS_RXSR_OFFSET);
		out32(eth0.base_address + XZYNQ_EMACPS_RXSR_OFFSET, status_rx);

		if (status_rx & XZYNQ_EMACPS_RXSR_FRAMERX_MASK) {
			if (emac_process_rx() != 0)
				return -1;
		}

		/* We have waited enough ... */
		if (timer_remaining() <= 0)
			return -2;
	}

	return 0;
}

int emac_process_rx()
{
	uint32_t num_rx_buf, i;
	ethernet_hdr *header;
	emac_bd *bd_rx_ptr;
	emac_bd *bd_iter;
	int res = 0;

	num_rx_buf =
		emac_bdring_from_hw_rx(&(emac_get_rx_ring(&eth0)), RXBD_CNT,
				&bd_rx_ptr);
	if (!num_rx_buf)
		return 0; /* Nothing todo */

	for (i = 0, bd_iter = bd_rx_ptr; i < num_rx_buf; i++) {
		emac_rx_buffer =
			(uint8_t *)(emac_bd_get_buf_addr(bd_iter) & ~0xFF);
		header = (ethernet_hdr *)emac_rx_buffer;

		/* Manage only ARP/IP frames */
		switch (ntohs(header->emac_type)) {
		case XZYNQ_EMACPS_ETH_TYPE_ARP:
			/* Pass the address of the frame without the ethernet header */
			res = emac_arp_process();
			break;

		case XZYNQ_EMACPS_ETH_TYPE_IP:
			/* Pass the address of the frame without the ethernet header */
			res = emac_tftp_process();
			break;
		}

		bd_iter = emac_bdring_next(&(emac_get_rx_ring(&eth0)), bd_iter);
	}

	/* Free the frame */
	emac_bdring_free(&(emac_get_rx_ring(&eth0)), num_rx_buf, bd_rx_ptr);

	/* Reallocate the frames */
	for (i = 0; i < num_rx_buf; i++) {
		emac_bdring_alloc(&(emac_get_rx_ring(&eth0)), 1, &bd_rx_ptr);
		emac_bd_clear_rx_new(bd_rx_ptr);
		emac_bdring_to_hw(&(emac_get_rx_ring(&eth0)), 1, bd_rx_ptr);
	}

	return res;
}

void emac_set_eth_header(uint16_t type, uint8_t *dmac)
{
	ethernet_hdr header;

	/* Fill the Ethernet header of the frame to send */
	header.emac_type = htons(type);
	mem_cpy(header.dmac, dmac, XZYNQ_EMACPS_MAC_SIZE);
	mem_cpy(header.smac, EMAC_MAC_ADDR, XZYNQ_EMACPS_MAC_SIZE);

	mem_cpy(emac_tx_buffer, (uint8_t *)&header,
			XZYNQ_EMACPS_ETH_HDR_SIZE);
}

void emac_send(int size)
{
	emac_bd *bd_tx_ptr;

	emac_bdring_alloc(&(emac_get_tx_ring(&eth0)), 1, &bd_tx_ptr);

	emac_bd_set_address_tx(bd_tx_ptr, emac_tx_buffer);
	emac_bd_set_length(bd_tx_ptr, size);
	emac_bd_clear_tx_used(bd_tx_ptr);
	emac_bd_set_last(bd_tx_ptr);

	emac_bdring_to_hw(&(emac_get_tx_ring(&eth0)), 1, bd_tx_ptr);

	/* Start transmit */
	emac_transmit((&eth0));

	/* Wait for complete TX */
	while (!(in32(eth0.base_address + XZYNQ_EMACPS_ISR_OFFSET) &
				XZYNQ_EMACPS_IXR_TXCOMPL_MASK))
		;

	/* Clear the status */
	out32(eth0.base_address + XZYNQ_EMACPS_TXSR_OFFSET,
			in32(eth0.base_address + XZYNQ_EMACPS_TXSR_OFFSET));

	emac_bdring_from_hw_tx(&(emac_get_tx_ring(&eth0)), 1, &bd_tx_ptr);
	emac_bdring_free(&(emac_get_tx_ring(&eth0)), 1, bd_tx_ptr);
}

void emac_fini()
{
	/* Disable ALL interrupts */
	emac_int_disable(&eth0, XZYNQ_EMACPS_IXR_ALL_MASK);

	/* Stop receiving/sending data */
	emac_stop(&eth0);
}

void emac_start(emac *emac)
{
	uint32_t reg;

	/* Start DMA */
	/* When starting the DMA channels, both transmit and receive sides
	 * need an initialized BD list.
	 */
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_RXQBASE_OFFSET,
			emac->rx_bdring.base_bd_addr);

	emac_write_reg(emac->base_address, XZYNQ_EMACPS_TXQBASE_OFFSET,
			emac->tx_bdring.base_bd_addr);

	/* clear any existed int status */
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_ISR_OFFSET,
			XZYNQ_EMACPS_IXR_ALL_MASK);

	/* Enable transmitter if not already enabled */
	reg = emac_read_reg(emac->base_address, XZYNQ_EMACPS_NWCTRL_OFFSET);
	if (!(reg & XZYNQ_EMACPS_NWCTRL_TXEN_MASK))
		emac_write_reg(emac->base_address, XZYNQ_EMACPS_NWCTRL_OFFSET,
				reg | XZYNQ_EMACPS_NWCTRL_TXEN_MASK);

	/* Enable receiver if not already enabled */
	reg = emac_read_reg(emac->base_address, XZYNQ_EMACPS_NWCTRL_OFFSET);
	if (!(reg & XZYNQ_EMACPS_NWCTRL_RXEN_MASK))
		emac_write_reg(emac->base_address, XZYNQ_EMACPS_NWCTRL_OFFSET,
				reg | XZYNQ_EMACPS_NWCTRL_RXEN_MASK);

	/* Enable TX and RX interrupts */
	emac_int_enable(emac, (XZYNQ_EMACPS_IXR_TX_ERR_MASK |
				XZYNQ_EMACPS_IXR_RX_ERR_MASK |
				XZYNQ_EMACPS_IXR_FRAMERX_MASK |
				XZYNQ_EMACPS_IXR_TXCOMPL_MASK));
}

void emac_stop(emac *emac)
{
	uint32_t reg;

	/* Disable all interrupts */
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_IDR_OFFSET,
			XZYNQ_EMACPS_IXR_ALL_MASK);

	/* Disable the receiver & transmitter */
	reg = emac_read_reg(emac->base_address, XZYNQ_EMACPS_NWCTRL_OFFSET);
	reg &= ~XZYNQ_EMACPS_NWCTRL_RXEN_MASK;
	reg &= ~XZYNQ_EMACPS_NWCTRL_TXEN_MASK;
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_NWCTRL_OFFSET, reg);
}

void emac_reset(emac *emac)
{
	uint32_t reg;
	uint8_t i;

	/* Stop the device and reset hardware */
	emac_stop(emac);

	/* Setup hardware with default values */
	emac_write_reg(emac->base_address,
			XZYNQ_EMACPS_NWCTRL_OFFSET,
			(XZYNQ_EMACPS_NWCTRL_STATCLR_MASK |
			 XZYNQ_EMACPS_NWCTRL_MDEN_MASK) &
			~XZYNQ_EMACPS_NWCTRL_LOOPEN_MASK);

	emac_write_reg(emac->base_address,
			XZYNQ_EMACPS_NWCFG_OFFSET,
			XZYNQ_EMACPS_NWCFG_100_MASK |
			XZYNQ_EMACPS_NWCFG_FDEN_MASK |
			XZYNQ_EMACPS_NWCFG_UCASTHASHEN_MASK);

	emac_write_reg(emac->base_address,
			XZYNQ_EMACPS_DMACR_OFFSET,
			((((XZYNQ_EMACPS_RX_BUF_SIZE /
				XZYNQ_EMACPS_RX_BUF_UNIT) +
			   ((XZYNQ_EMACPS_RX_BUF_SIZE %
				 XZYNQ_EMACPS_RX_BUF_UNIT) ? 1 : 0)) <<
			  XZYNQ_EMACPS_DMACR_RXBUF_SHIFT) &
			 XZYNQ_EMACPS_DMACR_RXBUF_MASK) |
			XZYNQ_EMACPS_DMACR_RXSIZE_MASK |
			XZYNQ_EMACPS_DMACR_TXSIZE_MASK);

	emac_write_reg(emac->base_address, XZYNQ_EMACPS_TXSR_OFFSET, 0x0);
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_RXQBASE_OFFSET, 0x0);
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_TXQBASE_OFFSET, 0x0);
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_RXSR_OFFSET, 0x0);
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_IDR_OFFSET,
			XZYNQ_EMACPS_IXR_ALL_MASK);

	reg = emac_read_reg(emac->base_address, XZYNQ_EMACPS_ISR_OFFSET);
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_ISR_OFFSET, reg);
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_PHYMNTNC_OFFSET, 0x0);

	/* clear all counters */
	for (i = 0;
			i < (XZYNQ_EMACPS_LAST_OFFSET - XZYNQ_EMACPS_OCTTXL_OFFSET) / 4;
			i++)
		emac_read_reg(emac->base_address,
				XZYNQ_EMACPS_OCTTXL_OFFSET + i * 4);

	/* Disable the receiver */
	reg = emac_read_reg(emac->base_address, XZYNQ_EMACPS_NWCTRL_OFFSET);
	reg &= ~XZYNQ_EMACPS_NWCTRL_RXEN_MASK;
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_NWCTRL_OFFSET, reg);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
