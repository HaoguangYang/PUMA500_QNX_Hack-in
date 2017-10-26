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

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <device_qnx.h>

void bsd_mii_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	xzynq_dev_t *xzynq = ifp->if_softc;

	xzynq->bsd_mii.mii_media_active = IFM_ETHER;
	xzynq->bsd_mii.mii_media_status = IFM_AVALID;

	if (xzynq->force_link) {
		if (xzynq->linkup) {
			xzynq->bsd_mii.mii_media_status |= IFM_ACTIVE;
		}

		/* report back the previously forced values */
		switch (xzynq->cfg.media_rate) {
		case 0:
			xzynq->bsd_mii.mii_media_active |= IFM_NONE;
			break;

		case 1000 * 10:
			xzynq->bsd_mii.mii_media_active |= IFM_10_T;
			break;

		case 1000 * 100:
			xzynq->bsd_mii.mii_media_active |= IFM_100_TX;
			break;

		case 1000 * 1000:
			xzynq->bsd_mii.mii_media_active |= IFM_1000_T;
			break;

		default: /* this shouldnt really happen, but ... */
			xzynq->bsd_mii.mii_media_active |= IFM_NONE;
			break;
		}
		if (xzynq->cfg.duplex) {
			xzynq->bsd_mii.mii_media_active |= IFM_FDX;
		}
	} else if (xzynq->linkup) { /* link is auto-detect and up */
		xzynq->bsd_mii.mii_media_status |= IFM_ACTIVE;

		switch (xzynq->cfg.media_rate) {
		case 1000 * 10:
			xzynq->bsd_mii.mii_media_active |= IFM_10_T;
			break;

		case 1000 * 100:
			xzynq->bsd_mii.mii_media_active |= IFM_100_TX;
			break;

		case 1000 * 1000:
			xzynq->bsd_mii.mii_media_active |= IFM_1000_T;
			break;

		default: // this shouldnt really happen, but ...
			xzynq->bsd_mii.mii_media_active |= IFM_NONE;
			break;
		}

		if (xzynq->cfg.duplex) {
			xzynq->bsd_mii.mii_media_active |= IFM_FDX;
		}

		xzynq->bsd_mii.mii_media_active |= IFM_ETH_TXPAUSE;
		xzynq->bsd_mii.mii_media_active |= IFM_ETH_RXPAUSE;

		// could move this to event.c so there was no lag
		ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER | IFM_AUTO);

	} else { // link is auto-detect and down
		xzynq->bsd_mii.mii_media_active |= IFM_NONE;
		xzynq->bsd_mii.mii_media_status = 0;

		// could move this to event.c so there was no lag
		ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER | IFM_NONE);
	}

	// stuff parameter values with hoked-up bsd values
	ifmr->ifm_status = xzynq->bsd_mii.mii_media_status;
	ifmr->ifm_active = xzynq->bsd_mii.mii_media_active;
}

int bsd_mii_mediachange(struct ifnet *ifp)
{
	xzynq_dev_t *xzynq = ifp->if_softc;
	int old_media_rate = xzynq->cfg.media_rate;
	int old_duplex = xzynq->cfg.duplex;
	int old_force_link = xzynq->force_link;
	struct ifmedia *ifm = &xzynq->bsd_mii.mii_media;
	int user_duplex = ifm->ifm_media & IFM_FDX ? 1 : 0;
	int user_media = ifm->ifm_media & IFM_TMASK;

	if (!(ifp->if_flags & IFF_UP)) {
		return 0;
	}

	if (!(ifm->ifm_media & IFM_ETHER)) {
		return 0;
	}

	switch (user_media) {
	case IFM_AUTO: /* auto-select media */
		xzynq->force_link = 0;
		xzynq->cfg.media_rate = -1;
		xzynq->cfg.duplex = -1;
		ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER | IFM_AUTO);
		break;

	case IFM_NONE: /* disable media */
		/* forcing the link with a speed of zero means to disable the link */
		xzynq->force_link = 1;
		xzynq->cfg.media_rate = 0;
		xzynq->cfg.duplex = 0;
		ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER | IFM_NONE);
		break;

	case IFM_10_T: /* force 10baseT */
		xzynq->force_link = 1;
		xzynq->cfg.media_rate = 10 * 1000;
		xzynq->cfg.duplex = user_duplex;
		ifmedia_set(&xzynq->bsd_mii.mii_media, user_duplex ? IFM_ETHER
				| IFM_10_T | IFM_FDX : IFM_ETHER | IFM_10_T);
		break;

	case IFM_100_TX: /* force 100baseTX */
		xzynq->force_link = 1;
		xzynq->cfg.media_rate = 100 * 1000;
		xzynq->cfg.duplex = user_duplex;
		ifmedia_set(&xzynq->bsd_mii.mii_media, user_duplex ? IFM_ETHER
				| IFM_100_TX | IFM_FDX : IFM_ETHER | IFM_100_TX);
		break;

	case IFM_1000_T: /* force 1000baseT */
		/* only GigE full duplex supported */
		xzynq->force_link = 1;
		xzynq->cfg.media_rate = 1000 * 1000;
		xzynq->cfg.duplex = user_duplex;
		ifmedia_set(&xzynq->bsd_mii.mii_media, user_duplex ? IFM_ETHER
				| IFM_1000_T | IFM_FDX : IFM_ETHER | IFM_1000_T);
		break;

	default:
		return 0;
		break;
	}

	/* Forced flow control */
	ifmedia_set(&xzynq->bsd_mii.mii_media, (IFM_ETHER
								| IFM_1000_T | IFM_FDX
								| MDI_FLOW | MDI_FLOW_ASYM));

	/* does the user want something different than it already is? */
	if ((xzynq->cfg.media_rate != old_media_rate) || (xzynq->cfg.duplex
			!= old_duplex) || (xzynq->force_link != old_force_link)
			|| (xzynq->cfg.flags & NIC_FLAG_LINK_DOWN)) {
		/* re-initialize hardware with new parameters */
		ifp->if_init(ifp);
	}

	return 0;
}

void bsd_mii_initmedia(xzynq_dev_t *xzynq)
{
	nic_config_t *cfg = &xzynq->cfg;

	xzynq->bsd_mii.mii_ifp = &xzynq->ecom.ec_if;

	ifmedia_init(&xzynq->bsd_mii.mii_media, IFM_IMASK, bsd_mii_mediachange,
			bsd_mii_mediastatus);

	// we do NOT call mii_attach() - we do our own link management

	//
	// must create these entries to make ifconfig media work
	// see lib/socket/public/net/if_media.h for defines
	//

	// ifconfig wm0 none (x22)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);

	// ifconfig wm0 auto (x20)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO, 0, NULL);

	// ifconfig wm0 10baseT (x23 - half duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_10_T, 0, NULL);

	// ifconfig wm0 10baseT-FDX (x100023)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);

	// ifconfig wm0 10baseT-FDX mediaopt flow (x500023)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_FLOW, 0, NULL);

	// ifconfig wm0 10baseT-FDX mediaopt txpause (x100423)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

	// ifconfig wm0 10baseT-FDX mediaopt rxpause (x100223)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

	// ifconfig wm0 100baseTX (x26 - half duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX, 0, NULL);

	// ifconfig wm0 100baseTX-FDX (x100026 - full duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);

	// ifconfig wm0 100baseTX-FDX mediaopt flow (x500026 - full duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_FLOW, 0, NULL);

	// ifconfig wm0 100baseTX-FDX mediaopt txpause (x100426 - full duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

	// ifconfig wm0 100baseTX-FDX mediaopt rxpause (x100226 - full duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

       // ifconfig wm0 1000baseT mediaopt fdx (x100030 - full duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);

       // ifconfig wm0 1000baseT mediaopt fdx,flow (x500030 - full duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_FLOW, 0, NULL);

       // ifconfig wm0 1000baseT mediaopt fdx,txpause (x100430 - full duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

       // ifconfig wm0 1000baseT mediaopt fdx,rxpause (x100230 - full duplex)
    ifmedia_add(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

	switch (cfg->media_rate) {
		case -1:
			ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO);
			break;
		case 10*1000:
			if (cfg->duplex) {
				ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX);
			} else {
				ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_10_T);
			}
			break;
		case 100*1000:
			if (cfg->duplex) {
				ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX);
			} else {
				ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX);
			}
			break;
		case 1000*1000:
			if (cfg->duplex) {
				ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX);
			} else {
				ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T);
			}
			break;
		default:
			ifmedia_set(&xzynq->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
			break;
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/bsd_media.c $ $Rev: 752035 $")
#endif
