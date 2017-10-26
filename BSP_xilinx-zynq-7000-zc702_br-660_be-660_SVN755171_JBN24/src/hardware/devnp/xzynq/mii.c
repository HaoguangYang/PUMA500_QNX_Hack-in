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

#include <xzynq.h>

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
uint16_t xzynq_mdi_read(void *hdl, uint8_t phyid, uint8_t reg)
{
	uint32_t mgtcr;
	volatile uint32_t ipisr;
	xzynq_dev_t *xzynq = (xzynq_dev_t *) hdl;

	/* Construct Mgtcr mask for the operation */
	mgtcr = XZYNQ_EMACPS_PHYMNTNC_OP_MASK | XZYNQ_EMACPS_PHYMNTNC_OP_R_MASK
			| (phyid << XZYNQ_EMACPS_PHYMNTNC_PHYAD_SHIFT_MASK)
			| (reg << XZYNQ_EMACPS_PHYMNTNC_PHREG_SHIFT_MASK);

	/* Write Mgtcr and wait for completion */
	out32(xzynq->regbase + XZYNQ_EMACPS_PHYMNTNC_OFFSET, mgtcr);

	do {
		ipisr = in32(xzynq->regbase + XZYNQ_EMACPS_NWSR_OFFSET);
	} while ((ipisr & XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK) == 0);

	/* Read data */
	return in32(xzynq->regbase + XZYNQ_EMACPS_PHYMNTNC_OFFSET);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_mdi_write(void *hdl, uint8_t phyid, uint8_t reg, uint16_t data)
{
	uint32_t mgtcr;
	volatile uint32_t ipisr;
	xzynq_dev_t *xzynq = (xzynq_dev_t *) hdl;

	/* Construct Mgtcr mask for the operation */
	mgtcr = XZYNQ_EMACPS_PHYMNTNC_OP_MASK | XZYNQ_EMACPS_PHYMNTNC_OP_W_MASK
			| (phyid << XZYNQ_EMACPS_PHYMNTNC_PHYAD_SHIFT_MASK)
			| (reg << XZYNQ_EMACPS_PHYMNTNC_PHREG_SHIFT_MASK)
			| data;

	/* Write Mgtcr and wait for completion */
	out32(xzynq->regbase + XZYNQ_EMACPS_PHYMNTNC_OFFSET, mgtcr);

	do {
		ipisr = in32(xzynq->regbase + XZYNQ_EMACPS_NWSR_OFFSET);
	} while ((ipisr & XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK) == 0);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_mdi_callback(void *handle, uint8_t phyaddr, uint8_t linkstate)
{
	xzynq_dev_t *xzynq = handle;
	int mode, gig_en = 0;
	uint32_t reg;
	char *s = 0;
	struct ifnet *ifp = &xzynq->ecom.ec_if;
	nic_config_t *cfg = &xzynq->cfg;
	uintptr_t slrc_virt_base;

	switch (linkstate) {
	case MDI_LINK_UP:
		cfg->flags &= ~NIC_FLAG_LINK_DOWN;
		xzynq->linkup = 1;
		MDI_GetActiveMedia(xzynq->mdi, cfg->phy_addr, &mode);

		switch (mode) {
		case MDI_10bTFD:
			cfg->media_rate = 10000L;
			break;
		case MDI_10bT:
			s = "10 BaseT Half Duplex";
			cfg->duplex = 0;
			cfg->media_rate = 10000L;
			break;
		case MDI_100bTFD:
			s = "100 BaseT Full Duplex";
			cfg->duplex = 1;
			cfg->media_rate = 100000L;
			break;
		case MDI_100bT:
			s = "100 BaseT Half Duplex";
			cfg->duplex = 0;
			cfg->media_rate = 100000L;
			break;
		case MDI_100bT4:
			s = "100 BaseT4";
			cfg->duplex = 0;
			cfg->media_rate = 100000L;
			break;
		case MDI_1000bT:
			s = "1000 BaseT Half Duplex !!!NOT SUPPORTED!!!";
			cfg->duplex = 0;
			cfg->media_rate = 1000 * 1000L;
			gig_en = 1;
			break;
		case MDI_1000bTFD:
			s = "1000 BaseT Full Duplex";
			cfg->duplex = 1;
			cfg->media_rate = 1000 * 1000L;
			gig_en = 1;
			break;
		default:
			s = "Unknown";
			cfg->duplex = 0;
			cfg->media_rate = 0L;
			break;
		}

		/* Configure the MAC */
		reg = in32(xzynq->regbase + XZYNQ_EMACPS_NWCFG_OFFSET);
		reg &= ~XZYNQ_EMACPS_NWCFG_MDCCLKDIV_MASK;
		reg |= (0x7 << XZYNQ_EMACPS_NWCFG_MDC_SHIFT_MASK); /* 540Mhz */

		if (cfg->duplex)
			reg |= XZYNQ_EMACPS_NWCFG_FDEN_MASK;
		else
			reg &= ~XZYNQ_EMACPS_NWCFG_FDEN_MASK;

		if (cfg->media_rate == 100000L)
			reg |= XZYNQ_EMACPS_NWCFG_100_MASK;
		else
			reg &= ~XZYNQ_EMACPS_NWCFG_100_MASK;

		if (gig_en)
			reg |= XZYNQ_EMACPS_NWCFG_1000_MASK;
		else
			reg &= ~XZYNQ_EMACPS_NWCFG_1000_MASK;

		out32(xzynq->regbase + XZYNQ_EMACPS_NWCFG_OFFSET, reg);

		/* GEM0_CLK Setup */
		slrc_virt_base
				= mmap_device_io(XZYNQ_SLCR_LEN,
						XZYNQ_SLCR_BASE);

		/* SLCR unlock */
		out32(slrc_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET,
				XZYNQ_SLCR_UNLOCK_KEY);

		/* Configure GEM0_RCLK_CTRL */
		out32(slrc_virt_base + XZYNQ_SLCR_GEM0_RCLK_CTRL_REG, ((0 << 4)
				| (1 << 0)));

		/* Set divisors for appropriate frequency in GEM0_CLK_CTRL */
		if (gig_en) { /* 125MHz */
			out32(slrc_virt_base + XZYNQ_SLCR_GEM0_CLK_CTRL_REG,
					((1 << 20) | (8 << 8) | (0 << 4) | (1
							<< 0)));
		} else if (cfg->media_rate == 100000) { /* 25 MHz */
			out32(slrc_virt_base + XZYNQ_SLCR_GEM0_CLK_CTRL_REG,
					((1 << 20) | (40 << 8) | (0 << 4) | (1
							<< 0)));
		} else { /* 2.5 MHz */
			out32(slrc_virt_base + XZYNQ_SLCR_GEM0_CLK_CTRL_REG,
					((10 << 20) | (40 << 8) | (0 << 4) | (1
							<< 0)));
		}

		/* SLCR lock */
		/*out32(slrc_virt_base + XZYNQ_SLCR_LOCK_OFFSET,
				XZYNQ_SLCR_LOCK_KEY);*/
		if_link_state_change(ifp, LINK_STATE_UP);
		break;

	case MDI_LINK_DOWN:
		cfg->media_rate = cfg->duplex = -1;
		MDI_AutoNegotiate(xzynq->mdi, cfg->phy_addr, NoWait);
		cfg->flags |= NIC_FLAG_LINK_DOWN;
		xzynq->linkup = 0;
		if_link_state_change(ifp, LINK_STATE_DOWN);
		break;

	default:
		break;
	}
	(void)s;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int xzynq_mdi_find_phy(xzynq_dev_t *xzynq)
{
	int status;
	int phy_idx = xzynq->cfg.phy_addr;

	/* DeRegister the phy if we have already one register */
	if (xzynq->mdi) {
		MDI_DeRegister(&xzynq->mdi); // repetitively called by if_init
	}

	/* Register callback for write/read PHY */
	status = MDI_Register_Extended(xzynq, xzynq_mdi_write, xzynq_mdi_read,
			xzynq_mdi_callback, (mdi_t **) &xzynq->mdi, NULL, 0, 0);
	if (status != MDI_SUCCESS) {
		xzynq->mdi = NULL;
		return -1;
	}

	callout_init(&xzynq->mii_callout);

	/* Get PHY address */
	for (phy_idx = 0; phy_idx < 32; phy_idx++) {
		if (MDI_FindPhy(xzynq->mdi, phy_idx) == MDI_SUCCESS
				&& MDI_InitPhy(xzynq->mdi, phy_idx)
						== MDI_SUCCESS) {
			MDI_ResetPhy(xzynq->mdi, phy_idx, WaitBusy);
			break;
		}
		/* If PHY is not detected then exit */
		if (phy_idx == 31) {
			return -1;
		}
	}

	xzynq->cfg.phy_addr = phy_idx;
	return phy_idx;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_mdi_configure_phy(xzynq_dev_t *xzynq, int phyid)
{
	nic_config_t *cfg = &xzynq->cfg;
	int an_capable, i;
	uint16_t reg;

	MDI_InitPhy(xzynq->mdi, phyid);

	cfg->connector = NIC_CONNECTOR_MII;
	an_capable = xzynq_mdi_read(xzynq, cfg->phy_addr, MDI_BMSR)
			& BMSR_AN_ABILITY;

	/* General configuration */
	if (xzynq->force_link != -1 || !an_capable) {
		reg = xzynq_mdi_read(xzynq, cfg->phy_addr, MDI_BMCR);
		reg &= ~(BMCR_RESTART_AN | BMCR_SPEED_100 | BMCR_SPEED_1000
				| BMCR_FULL_DUPLEX);
		if (an_capable && xzynq->force_link != 0) {
			/*
			 * If we force the speed, but the link partner
			 * is autonegotiating, there is a greater chance
			 * that everything will work if we advertise with
			 * the speed that we are forcing to.
			 */
			MDI_SetAdvert(xzynq->mdi, cfg->phy_addr,
					xzynq->force_link);
			reg |= BMCR_RESTART_AN | BMCR_AN_ENABLE;
		} else {
			reg &= ~BMCR_AN_ENABLE;

			if (cfg->media_rate == 100 * 1000) {
				reg |= BMCR_SPEED_100;
			} else {
				if (cfg->media_rate == 1000 * 1000) {
					reg |= BMCR_SPEED_1000;
				}
			}
		}
		xzynq_mdi_write(xzynq, cfg->phy_addr, MDI_BMCR, reg);

		if (reg & BMCR_AN_ENABLE) {
			MDI_EnableMonitor(xzynq->mdi, 1);
		} else {
			MDI_DisableMonitor(xzynq->mdi);
		}
	} else { /* not forcing the link */
		cfg->flags |= NIC_FLAG_LINK_DOWN;

		/* Enable PAUSE frame */
		MDI_GetMediaCapable(xzynq->mdi, cfg->phy_addr, &i);
		i |= MDI_FLOW | MDI_FLOW_ASYM;
		MDI_SetAdvert(xzynq->mdi, cfg->phy_addr, i);

		MDI_AutoNegotiate(xzynq->mdi, cfg->phy_addr, NoWait);
		MDI_EnableMonitor(xzynq->mdi, 1);
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/mii.c $ $Rev: 752035 $")
#endif
