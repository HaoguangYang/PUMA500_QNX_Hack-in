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
#include    <net/ifdrvcom.h>
#include    <sys/sockio.h>

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void xzynq_dump_stats(xzynq_dev_t *xzynq)
{
	nic_ethernet_stats_t *estats = &xzynq->stats.un.estats;
	nic_stats_t *stats = &xzynq->stats;

	/*
	 * WARNING: By reading theses registers, the value is automatically reset
	 */
	stats->rxed_ok = in32(xzynq->regbase + XZYNQ_EMACPS_RXCNT_OFFSET);
	stats->rxed_broadcast = in32(xzynq->regbase
			+ XZYNQ_EMACPS_RXBROADCNT_OFFSET);
	stats->rxed_multicast = in32(xzynq->regbase
			+ XZYNQ_EMACPS_RXMULTICNT_OFFSET);
	stats->octets_rxed_ok = in32(xzynq->regbase
			+ XZYNQ_EMACPS_OCTRXL_OFFSET);
	stats->octets_rxed_ok |= ((uint64_t) in32(xzynq->regbase
			+ XZYNQ_EMACPS_OCTRXL_OFFSET) << 32);

	stats->txed_ok = in32(xzynq->regbase + XZYNQ_EMACPS_TXCNT_OFFSET);
	stats->txed_broadcast = in32(xzynq->regbase
			+ XZYNQ_EMACPS_TXBCCNT_OFFSET);
	stats->txed_multicast = in32(xzynq->regbase
			+ XZYNQ_EMACPS_TXMCCNT_OFFSET);
	stats->octets_txed_ok = in32(xzynq->regbase
			+ XZYNQ_EMACPS_OCTTXL_OFFSET);
	stats->octets_txed_ok |= ((uint64_t) in32(xzynq->regbase
			+ XZYNQ_EMACPS_OCTTXL_OFFSET) << 32);

	estats->align_errors = in32(xzynq->regbase
			+ XZYNQ_EMACPS_RXALIGNCNT_OFFSET);
	estats->single_collisions = in32(xzynq->regbase
			+ XZYNQ_EMACPS_SNGLCOLLCNT_OFFSET);
	estats->multi_collisions = in32(xzynq->regbase
			+ XZYNQ_EMACPS_MULTICOLLCNT_OFFSET);
	estats->fcs_errors
			= in32(xzynq->regbase + XZYNQ_EMACPS_RXFCSCNT_OFFSET);
	estats->tx_deferred = in32(xzynq->regbase
			+ XZYNQ_EMACPS_TXDEFERCNT_OFFSET);
	estats->late_collisions = in32(xzynq->regbase
			+ XZYNQ_EMACPS_LATECOLLCNT_OFFSET);
	estats->no_carrier = in32(xzynq->regbase
			+ XZYNQ_EMACPS_TXCSENSECNT_OFFSET);
	estats->oversized_packets = in32(xzynq->regbase
			+ XZYNQ_EMACPS_RXOVRCNT_OFFSET);
	estats->jabber_detected = in32(xzynq->regbase
			+ XZYNQ_EMACPS_RXJABCNT_OFFSET);
	estats->short_packets = in32(xzynq->regbase
			+ XZYNQ_EMACPS_RXUNDRCNT_OFFSET);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int xzynq_ioctl(struct ifnet * ifp, unsigned long cmd, caddr_t data)
{
	int error = 0;
	xzynq_dev_t *xzynq = ifp->if_softc;
	struct drvcom_config *dcfgp;
	struct drvcom_stats *dstp;
	struct ifdrv_com *ifdc;

	switch (cmd) {
	case SIOCGDRVCOM:
		ifdc = (struct ifdrv_com *) data;
		switch (ifdc->ifdc_cmd) {
		case DRVCOM_CONFIG:
			dcfgp = (struct drvcom_config *) ifdc;

			if (ifdc->ifdc_len != sizeof(nic_config_t)) {
				error = EINVAL;
				break;
			}
			memcpy(&dcfgp->dcom_config, &xzynq->cfg,
					sizeof(xzynq->cfg));
			break;

		case DRVCOM_STATS:
			dstp = (struct drvcom_stats *) ifdc;

			if (ifdc->ifdc_len != sizeof(nic_stats_t)) {
				error = EINVAL;
				break;
			}
			xzynq_dump_stats(xzynq);
			memcpy(&dstp->dcom_stats, &xzynq->stats,
					sizeof(xzynq->stats));
			break;

		default:
			error = ENOTTY;
		}
		break;

	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA: {
		struct ifreq *ifr = (struct ifreq *) data;
		error = ifmedia_ioctl(ifp, ifr, &xzynq->bsd_mii.mii_media, cmd);
		break;
	}

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the
			 * hardware filter accordingly.
			 */
			if ((ifp->if_flags_tx & IFF_RUNNING) == 0) {
				/* Interface is currently down */
			} else {
				//xzynq_filter(xzynq);
			}
			error = 0;
		}
		break;
	}
	return error;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/ioctl.c $ $Rev: 752035 $")
#endif
