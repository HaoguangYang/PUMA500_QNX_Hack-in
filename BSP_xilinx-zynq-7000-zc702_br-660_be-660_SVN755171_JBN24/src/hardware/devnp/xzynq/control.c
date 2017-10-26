/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
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

#include "xzynq.h"
#include <hw/i2c.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define I2C_MUX_ADDR		0x74
//#define I2C_EEPROM_ADDR		0x54
//#define I2C_EEPROM_CHAN		0x04

static void i2c_receive_data(xzynq_dev_t *xzynq, int file, _Uint8t addr, _Uint8t* data, int len)
{
	int error, i;

	struct {
		i2c_recv_t hdr;
		_Uint8t reg_data[256];
	} rd_data;

	rd_data.hdr.slave.addr = addr;
	rd_data.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	rd_data.hdr.len = len;
	rd_data.hdr.stop = 1;

	if ((error = devctl(file, DCMD_I2C_RECV, &rd_data, sizeof(rd_data),
			NULL))) {
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
				"devnp: Cannot send a devctl to /dev/i2c1\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < len; i++)
		data[i] = rd_data.reg_data[i];
}

static int i2c_send_data(xzynq_dev_t *xzynq, int file, _Uint8t addr, _Uint8t data)
{
	int error;

	struct {
		i2c_send_t hdr;
		_Uint8t reg_data;
	} wd_data;

	wd_data.hdr.slave.addr = addr;
	wd_data.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	wd_data.hdr.len = 1;
	wd_data.hdr.stop = 1;
	wd_data.reg_data = data;

	if ((error = devctl(file, DCMD_I2C_SEND, &wd_data, sizeof(wd_data),
			NULL))) {
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
				"devnp: Cannot send a devctl to /dev/i2c1\n");
		return -1;
	}

	return 0;
}

void xzynq_read_eeproom_mac_address(xzynq_dev_t *xzynq)
{
	//No I2C on Zedboard, use hard code instead.
	//int fd, i, sum;
	//sum = 0;

	/* Read the MAC Address from the EEPROM */
	//fd = open("/dev/i2c1", O_RDWR);

	/*
	if (fd == -1) {
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
				"devnp: Cannot open /dev/i2c1\n");
	} else {
		//No I2C on Zedboard, use hard code instead.
		/* Select channel for the EEPROM */
		//i2c_send_data(xzynq, fd, I2C_MUX_ADDR, I2C_EEPROM_CHAN);

		/* Read the MAC address from the EEPROM */
		//i2c_send_data(xzynq, fd, I2C_EEPROM_ADDR, 0x0); /* Read at address 0x0 */
		//i2c_receive_data(xzynq, fd, I2C_EEPROM_ADDR, xzynq->cfg.current_address, 6);

		/* Check the validity */
		/*
		sum = 0;
		for (i = 0; i < 6; i++) {
			sum += xzynq->cfg.current_address[i];
		}
	}*/

	/* 00:00:00:00:00:00 or FF:FF:FF:FF:FF:FF */
	//if (sum == 0 || sum == (6 * 0xFF)) {
		/* Default address */
		xzynq->cfg.current_address[0] = 0x00;
		xzynq->cfg.current_address[1] = 0x0A;
		xzynq->cfg.current_address[2] = 0x35;
		xzynq->cfg.current_address[3] = 0x02;
		xzynq->cfg.current_address[4] = 0x78;
		xzynq->cfg.current_address[5] = 0x10;
	//}

	/* Close the device */
	//close(fd);
}

void xzynq_set_mac_address(xzynq_dev_t *xzynq, int index)
{
	uint32_t mac_addr;

	/* Set the MAC bits [31:0] in BOT */
	mac_addr = xzynq->cfg.current_address[0];
	mac_addr |= xzynq->cfg.current_address[1] << 8;
	mac_addr |= xzynq->cfg.current_address[2] << 16;
	mac_addr |= xzynq->cfg.current_address[3] << 24;
	xzynq_write_reg(xzynq->regbase,
			(XZYNQ_EMACPS_LADDR1L_OFFSET + index * 8), mac_addr);

	/* There are reserved bits in TOP so don't affect them */
	mac_addr
			= xzynq_read_reg(xzynq->regbase,
					(XZYNQ_EMACPS_LADDR1H_OFFSET + (index * 8)));

	mac_addr &= ~XZYNQ_EMACPS_LADDR_MACH_MASK;

	/* Set MAC bits [47:32] in TOP */
	mac_addr |= xzynq->cfg.current_address[4];
	mac_addr |= xzynq->cfg.current_address[5] << 8;

	xzynq_write_reg(xzynq->regbase,
			(XZYNQ_EMACPS_LADDR1H_OFFSET + index * 8), mac_addr);
}

void xzynq_get_mac_address(xzynq_dev_t *emac, uint8_t *address_ptr, uint8_t index)
{
	uint32_t mac_addr;
	uint8_t *aptr = address_ptr;

	mac_addr = xzynq_read_reg(emac->regbase, (XZYNQ_EMACPS_LADDR1L_OFFSET
					+ (index * 8)));
	aptr[0] = (uint8_t) mac_addr;
	aptr[1] = (uint8_t) (mac_addr >> 8);
	aptr[2] = (uint8_t) (mac_addr >> 16);
	aptr[3] = (uint8_t) (mac_addr >> 24);

	/* Read MAC bits [47:32] in TOP */
	mac_addr = xzynq_read_reg(emac->regbase, (XZYNQ_EMACPS_LADDR1H_OFFSET
					+ (index * 8)));
	aptr[4] = (uint8_t) mac_addr;
	aptr[5] = (uint8_t) (mac_addr >> 8);
}

void xzynq_set_operating_speed(xzynq_dev_t *emac, uint16_t speed)
{
	uint32_t reg;

	reg = xzynq_read_reg(emac->regbase, XZYNQ_EMACPS_NWCFG_OFFSET);
	reg &= ~(XZYNQ_EMACPS_NWCFG_1000_MASK | XZYNQ_EMACPS_NWCFG_100_MASK);

	switch (speed) {
	case 10:
		break;

	case 100:
		reg |= XZYNQ_EMACPS_NWCFG_100_MASK;
		break;

	case 1000:
		reg |= XZYNQ_EMACPS_NWCFG_1000_MASK;
		break;

	default:
		return;
	}

	/* Set register and return */
	xzynq_write_reg(emac->regbase, XZYNQ_EMACPS_NWCFG_OFFSET, reg);
}

uint16_t xzynq_get_operating_speed(xzynq_dev_t *emac)
{
	uint32_t reg;

	reg = xzynq_read_reg(emac->regbase, XZYNQ_EMACPS_NWCFG_OFFSET);

	if (reg & XZYNQ_EMACPS_NWCFG_1000_MASK)
		return 1000;
	else {
		if (reg & XZYNQ_EMACPS_NWCFG_100_MASK)
			return 100;
		else
			return 10;
	}
}

int xzynq_phy_read(xzynq_dev_t *emac, uint32_t phy_address,
		uint32_t register_num, uint16_t *phy_data_ptr)
{
	uint32_t mgtcr;
	volatile uint32_t ipisr;

	/* Make sure no other PHY operation is currently in progress */
	if (!(xzynq_read_reg(emac->regbase, XZYNQ_EMACPS_NWSR_OFFSET)
			& XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK))
		return EMAC_MII_BUSY;

	/* Construct mgtcr mask for the operation */
	mgtcr
			= XZYNQ_EMACPS_PHYMNTNC_OP_MASK
					| XZYNQ_EMACPS_PHYMNTNC_OP_R_MASK
					| (phy_address
							<< XZYNQ_EMACPS_PHYMNTNC_PHYAD_SHIFT_MASK)
					| (register_num
							<< XZYNQ_EMACPS_PHYMNTNC_PHREG_SHIFT_MASK);

	/* Write mgtcr and wait for completion */
	xzynq_write_reg(emac->regbase, XZYNQ_EMACPS_PHYMNTNC_OFFSET, mgtcr);

	do
		ipisr = xzynq_read_reg(emac->regbase, XZYNQ_EMACPS_NWSR_OFFSET);
	while ((ipisr & XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK) == 0);

	/* Read data */
	*phy_data_ptr = xzynq_read_reg(emac->regbase,
			XZYNQ_EMACPS_PHYMNTNC_OFFSET);

	return SUCCESS;
}

int xzynq_phy_write(xzynq_dev_t *emac, uint32_t phy_address,
		uint32_t register_num, uint16_t phy_data)
{
	uint32_t mgtcr;
	volatile uint32_t ipisr;

	/* Make sure no other PHY operation is currently in progress */
	if (!(xzynq_read_reg(emac->regbase, XZYNQ_EMACPS_NWSR_OFFSET)
			& XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK))
		return EMAC_MII_BUSY;

	/* Construct mgtcr mask for the operation */
	mgtcr
			= XZYNQ_EMACPS_PHYMNTNC_OP_MASK
					| XZYNQ_EMACPS_PHYMNTNC_OP_W_MASK
					| (phy_address
							<< XZYNQ_EMACPS_PHYMNTNC_PHYAD_SHIFT_MASK)
					| (register_num
							<< XZYNQ_EMACPS_PHYMNTNC_PHREG_SHIFT_MASK)
					| phy_data;

	/* Write mgtcr and wait for completion */
	xzynq_write_reg(emac->regbase, XZYNQ_EMACPS_PHYMNTNC_OFFSET, mgtcr);

	do
		ipisr = xzynq_read_reg(emac->regbase, XZYNQ_EMACPS_NWSR_OFFSET);
	while ((ipisr & XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK) == 0);

	return SUCCESS;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/control.c $ $Rev: 752035 $")
#endif
