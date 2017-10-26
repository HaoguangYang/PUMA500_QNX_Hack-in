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

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int xzynq_process_rx(xzynq_dev_t *xzynq, struct nw_work_thread *wtp)
{
	int num_bds, i, j;
	struct mbuf *m;
	struct ifnet *ifp = &xzynq->ecom.ec_if;
	xzynq_bd_t *bd_ptr;
	xzynq_bd_t *cur_bd_ptr;

	while (1) {
		/* Get all RX received from the DMA */
		num_bds = xzynq_bdring_from_hw_rx(&(xzynq_get_rx_ring(xzynq)),
				XZYNQ_EMACPS_NUM_MBUF_RX, &bd_ptr);
		if (num_bds <= 0) {
			return 0;
		}

		for (i = 0, cur_bd_ptr = bd_ptr; i < num_bds; i++) {
			/* Check the validity of the packet */
			if (!(xzynq_bd_get_status(cur_bd_ptr) & XZYNQ_EMACPS_RXBUF_SOF_MASK)
					|| !(xzynq_bd_get_status(cur_bd_ptr)
							& XZYNQ_EMACPS_RXBUF_SOF_MASK)) {
				slogf(
						_SLOGC_NETWORK, _SLOG_ERROR,
						"%s: Error while received a packet. SOF/EOF are not set\n",
						ifp->if_xname);
				return 0;
			}

			/* Convert the BD to Index to get the mbuf structure */
			j = xzynq_bd_to_index(&(xzynq_get_rx_ring(xzynq)), cur_bd_ptr);
			m = xzynq->meminfo.rx_mbuf[j];

#if 1
			off64_t phys;
			phys = pool_phys(m->m_data, m->m_ext.ext_page);

			/* this should be done in xzynq_bdring_to_hw */
			CACHE_INVAL(&xzynq->meminfo.cachectl, m->m_data, phys, m->m_ext.ext_size);
#endif

			/* Get the info of the packet */
			m->m_pkthdr.len = xzynq_bd_get_length(cur_bd_ptr);
			m->m_len = xzynq_bd_get_length(cur_bd_ptr);
			m->m_pkthdr.rcvif = ifp;

			if (m->m_len <= 0) {
				slogf(_SLOGC_NETWORK, _SLOG_ERROR,
					"%s: Error while received a packet. Length is 0 !\n",
					ifp->if_xname);
				return 0;
			}

#if NBPFILTER > 0
			/* Pass this up to any BPF listeners. */
			if (ifp->if_bpf) {
				bpf_mtap(ifp->if_bpf, m);
			}
#endif
			/* Finally, pass this received pkt up */
			ifp->if_ipackets++;
			(*ifp->if_input)(ifp, m);

			/* Next */
			cur_bd_ptr = xzynq_bdring_next(&(xzynq_get_rx_ring(xzynq)), cur_bd_ptr);
		}

		/* Setup new RX packets */
		xzynq_bdring_free(&(xzynq_get_rx_ring(xzynq)), num_bds, bd_ptr);
		xzynq_setup_rx_buffers(xzynq, 1);
	}

	return 1;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/receive.c $ $Rev: 752035 $")
#endif
