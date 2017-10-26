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

#include "ipl.h"
#include "emac.h"

int emac_set_mac_address(emac *emac, void *address_ptr, uint8_t index)
{
	uint32_t mac_addr;
	uint8_t *aptr = (uint8_t *)address_ptr;

	/* index ranges 1 to 4, for offset calculation is 0 to 3. */
	index--;

	/* Set the MAC bits [31:0] in BOT */
	mac_addr = aptr[0];
	mac_addr |= aptr[1] << 8;
	mac_addr |= aptr[2] << 16;
	mac_addr |= aptr[3] << 24;
	emac_write_reg(emac->base_address,
		       (XZYNQ_EMACPS_LADDR1L_OFFSET + index * 8), mac_addr);

	/* There are reserved bits in TOP so don't affect them */
	mac_addr = emac_read_reg(emac->base_address,
				 (XZYNQ_EMACPS_LADDR1H_OFFSET + (index * 8)));

	mac_addr &= ~XZYNQ_EMACPS_LADDR_MACH_MASK;

	/* Set MAC bits [47:32] in TOP */
	mac_addr |= aptr[4];
	mac_addr |= aptr[5] << 8;

	emac_write_reg(emac->base_address,
		       (XZYNQ_EMACPS_LADDR1H_OFFSET + (index * 8)), mac_addr);

	return SUCCESS;
}

void emac_get_mac_address(emac *emac, void *address_ptr, uint8_t index)
{
	uint32_t mac_addr;
	uint8_t *aptr = (uint8_t *) address_ptr;

	/* index ranges 1 to 4, for offset calculation is 0 to 3. */
	index--;

	mac_addr = emac_read_reg(emac->base_address,
				 (XZYNQ_EMACPS_LADDR1L_OFFSET + (index * 8)));
	aptr[0] = (uint8_t)mac_addr;
	aptr[1] = (uint8_t)(mac_addr >> 8);
	aptr[2] = (uint8_t)(mac_addr >> 16);
	aptr[3] = (uint8_t)(mac_addr >> 24);

	/* Read MAC bits [47:32] in TOP */
	mac_addr = emac_read_reg(emac->base_address,
				 (XZYNQ_EMACPS_LADDR1H_OFFSET + (index * 8)));
	aptr[4] = (uint8_t)mac_addr;
	aptr[5] = (uint8_t)(mac_addr >> 8);
}

void emac_set_operating_speed(emac *emac, uint16_t speed)
{
	uint32_t reg;

	reg = emac_read_reg(emac->base_address, XZYNQ_EMACPS_NWCFG_OFFSET);
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
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_NWCFG_OFFSET, reg);
}

uint16_t emac_get_operating_speed(emac *emac)
{
	uint32_t reg;

	reg = emac_read_reg(emac->base_address, XZYNQ_EMACPS_NWCFG_OFFSET);

	if (reg & XZYNQ_EMACPS_NWCFG_1000_MASK)
		return 1000;
	else {
		if (reg & XZYNQ_EMACPS_NWCFG_100_MASK)
			return 100;
		else
			return 10;
	}
}

int emac_phy_read(emac *emac, uint32_t phy_address,
		  uint32_t register_num, uint16_t *phy_data_ptr)
{
	uint32_t mgtcr;
	volatile uint32_t ipisr;

	/* Make sure no other PHY operation is currently in progress */
	if (!(emac_read_reg(emac->base_address,
			    XZYNQ_EMACPS_NWSR_OFFSET) &
	      XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK))
		return EMAC_MII_BUSY;

	/* Construct mgtcr mask for the operation */
	mgtcr =
		XZYNQ_EMACPS_PHYMNTNC_OP_MASK | XZYNQ_EMACPS_PHYMNTNC_OP_R_MASK |
		(phy_address << XZYNQ_EMACPS_PHYMNTNC_PHYAD_SHIFT_MASK) |
		(register_num << XZYNQ_EMACPS_PHYMNTNC_PHREG_SHIFT_MASK);

	/* Write mgtcr and wait for completion */
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_PHYMNTNC_OFFSET, mgtcr);

	do
		ipisr = emac_read_reg(emac->base_address,
				      XZYNQ_EMACPS_NWSR_OFFSET);
	while ((ipisr & XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK) == 0);

	/* Read data */
	*phy_data_ptr = emac_read_reg(emac->base_address,
				      XZYNQ_EMACPS_PHYMNTNC_OFFSET);

	return SUCCESS;
}

int emac_phy_write(emac *emac, uint32_t phy_address,
		   uint32_t register_num, uint16_t phy_data)
{
	uint32_t mgtcr;
	volatile uint32_t ipisr;

	/* Make sure no other PHY operation is currently in progress */
	if (!(emac_read_reg(emac->base_address,
			    XZYNQ_EMACPS_NWSR_OFFSET) &
	      XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK))
		return EMAC_MII_BUSY;

	/* Construct mgtcr mask for the operation */
	mgtcr =
		XZYNQ_EMACPS_PHYMNTNC_OP_MASK | XZYNQ_EMACPS_PHYMNTNC_OP_W_MASK |
		(phy_address << XZYNQ_EMACPS_PHYMNTNC_PHYAD_SHIFT_MASK) |
		(register_num << XZYNQ_EMACPS_PHYMNTNC_PHREG_SHIFT_MASK) | phy_data;

	/* Write mgtcr and wait for completion */
	emac_write_reg(emac->base_address, XZYNQ_EMACPS_PHYMNTNC_OFFSET, mgtcr);

	do
		ipisr = emac_read_reg(emac->base_address,
				      XZYNQ_EMACPS_NWSR_OFFSET);
	while ((ipisr & XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK) == 0);

	return SUCCESS;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
