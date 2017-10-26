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
#include "bpfilter.h"
#include <sys/mbuf.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include "xzynq.h"

static struct mbuf* xzynq_defrag(struct mbuf *m)
{
	struct mbuf *m2;

	/* the entire packet should fit into one cluster */
	MGET(m2, M_DONTWAIT, MT_DATA);
	if (m2 == NULL) {
		m_freem(m);
		return NULL;
	}

	M_COPY_PKTHDR(m2, m);

	MCLGET(m2, M_DONTWAIT);
	if ((m2->m_flags & M_EXT) == 0) {
		m_freem(m);
		m_freem(m2);
		return NULL;
	}

	// this is NOT paranoid - this can happen with jumbo packets bigger
	// than a single cluster - should be handled above
	if (m->m_pkthdr.len > m2->m_ext.ext_size) {
		m_freem(m);
		m_freem(m2);
		return NULL;
	}

	m_copydata(m, 0, m->m_pkthdr.len, mtod(m2, caddr_t));
	m2->m_pkthdr.len = m2->m_len = m->m_pkthdr.len;

	m_freem(m);
	return m2;
}

int xzynq_reap_pkts(xzynq_dev_t *xzynq)
{
	xzynq_bd_t *bd_ptr, *cur_bd_ptr;
	int num_bds, i, j;

	num_bds = xzynq_bdring_from_hw_tx(&(xzynq_get_tx_ring(xzynq)),
			XZYNQ_EMACPS_NUM_MBUF_TX, &bd_ptr);
	cur_bd_ptr = bd_ptr;

	for (i = 0; i < num_bds; i++) {
		unsigned regval;

		regval = xzynq_bd_read(cur_bd_ptr, XZYNQ_EMACPS_BD_STAT_OFFSET);

		j = xzynq_bd_to_index(&(xzynq_get_tx_ring(xzynq)), cur_bd_ptr);

		if (xzynq->meminfo.tx_mbuf[j] != NULL) {
			m_freem(xzynq->meminfo.tx_mbuf[j]);
			xzynq->meminfo.tx_mbuf[j] = NULL;
		}
		xzynq->ecom.ec_if.if_opackets++;

		regval &= (XZYNQ_EMACPS_TXBUF_WRAP_MASK | XZYNQ_EMACPS_TXBUF_USED_MASK);

		/* If it was a fragment, the USED bit was cleared */
		if (!(regval & XZYNQ_EMACPS_TXBUF_USED_MASK))
			regval |= XZYNQ_EMACPS_TXBUF_USED_MASK;

		xzynq_bd_write(cur_bd_ptr, XZYNQ_EMACPS_BD_STAT_OFFSET, regval);

		cur_bd_ptr = xzynq_bdring_next(&(xzynq_get_tx_ring(xzynq)), cur_bd_ptr);
	}

	xzynq_bdring_free(&(xzynq_get_tx_ring(xzynq)), num_bds, bd_ptr);

	return num_bds;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_start(struct ifnet *ifp)
{
	xzynq_dev_t *xzynq = ifp->if_softc;
	struct nw_work_thread *wtp = WTP;
	struct mbuf *m, *m2;
	int num_frags, i, j, rc, timeout;
	xzynq_bd_t *bd_ptr, *bd_ptrs;
	off64_t phys;


	/* Transmit only if the link is up */
	if (!(ifp->if_flags_tx & IFF_RUNNING) || !xzynq->linkup) {
		NW_SIGUNLOCK_P(&ifp->if_snd_ex, xzynq->iopkt, wtp);
		return;
	}

	ifp->if_flags_tx |= IFF_OACTIVE;

	while (1) {
		/* Clean every transmitted packets */
		xzynq_reap_pkts(xzynq);

		/* Grab an outbound packet/mbuf chain */
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL) {
			goto done;
		}

		/* Count the number of fragments */
		m2 = m;
		num_frags = 0;
		while (m2 != NULL) {
			if (m2->m_len > 0)
				num_frags++;
			m2 = m2->m_next;
		}

		/* Try to defrag if necessary */
		if (num_frags > XZYNQ_EMAPS_MAX_FRAG) {
			m2 = xzynq_defrag(m);
			if (m2 == NULL) {
				slogf(_SLOGC_NETWORK, _SLOG_INFO,
						"devnp: %s defrag error", __func__);
				m_free(m2);
				xzynq->stats.tx_failed_allocs++;
				goto done;
			}
			/* m is now defragmented */
			m = m2;

			/* re-count the number of fragments */
			num_frags = 0;
			while (m2 != NULL) {
				if (m2->m_len > 0)
					num_frags++;
				m2 = m2->m_next;
			}
		}

		/* Waiting for enough free buffers if necessary */
		timeout = 100;
		while ((xzynq_get_tx_ring(xzynq).free_cnt < num_frags) &&
				(timeout--)) {
			usleep(1);
			xzynq_reap_pkts(xzynq);
		}

		if (timeout < 0) {
			m_free(m);
			xzynq->stats.tx_failed_allocs++;
			slogf(_SLOGC_NETWORK, _SLOG_INFO,
					"devnp: %s timeout waiting for free buffers",
					__func__);
			goto done;
		}

		/* Allocate enough buffer for the packet */
		rc = xzynq_bdring_alloc(&(xzynq_get_tx_ring(xzynq)), num_frags, &bd_ptr);
		if (rc) {
			m_free(m);
			xzynq->stats.tx_failed_allocs++;
			slogf(_SLOGC_NETWORK, _SLOG_INFO,
				"devnp: %s bdring_alloc error", __func__);
			goto done;
		}

		bd_ptrs = bd_ptr;

		/* Prepare the TX queue */
		for (i = 0; i < num_frags; i++) {
			unsigned regval;
			phys = mbuf_phys(m);
			CACHE_FLUSH (&xzynq->meminfo.cachectl, m->m_data, phys, m->m_len);

			xzynq_bd_write(bd_ptr, XZYNQ_EMACPS_BD_ADDR_OFFSET, phys);

			regval = xzynq_bd_read(bd_ptr, XZYNQ_EMACPS_BD_STAT_OFFSET);
			regval &= (XZYNQ_EMACPS_TXBUF_WRAP_MASK | XZYNQ_EMACPS_TXBUF_USED_MASK);
			regval |= ((regval & ~XZYNQ_EMACPS_TXBUF_LEN_MASK) | m->m_len);

			/* Set the LAST bit on the last buffer */
			if (i == (num_frags - 1))
				regval |= XZYNQ_EMACPS_TXBUF_LAST_MASK;

			xzynq_bd_write(bd_ptr, XZYNQ_EMACPS_BD_STAT_OFFSET, regval);

			/* Save the pointer for m_free() later */
			if (i == 0) {
				j = xzynq_bd_to_index(&(xzynq_get_tx_ring(xzynq)), bd_ptr);
				xzynq->meminfo.tx_mbuf[j] = m;
			}

#if NBPFILTER > 0
			// Pass the packet to any BPF listeners
			if (ifp->if_bpf) {
				bpf_mtap(ifp->if_bpf, m);
			}
#endif
			m = m->m_next;
			bd_ptr = xzynq_bdring_next(&(xzynq_get_tx_ring(xzynq)), bd_ptr);
		}

		rc = xzynq_bdring_to_hw(&(xzynq_get_tx_ring(xzynq)), num_frags, bd_ptrs, XZYNQ_EMACPS_SEND);
		if (rc) {
			slogf(_SLOGC_NETWORK, _SLOG_INFO,
				"devnp: %s bdring_to_hw error", __func__);
			xzynq->stats.tx_failed_allocs++;
			xzynq_bdring_unalloc(&(xzynq_get_tx_ring(xzynq)),
					num_frags, bd_ptrs);
			goto done;
		}

		/* transmit */
		xzynq_transmit(xzynq);
	}

done:
	ifp->if_flags_tx &= ~IFF_OACTIVE;
	NW_SIGUNLOCK_P(&ifp->if_snd_ex, xzynq->iopkt, wtp);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/transmit.c $ $Rev: 752035 $")
#endif
