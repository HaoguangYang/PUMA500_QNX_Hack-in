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

/**************************************************************************/
/*                                                                        */
/**************************************************************************/
const struct sigevent* xzynq_isr_kermask(void *arg, int iid)
{
	xzynq_dev_t *xzynq = arg;
	struct _iopkt_inter *ient = &xzynq->inter;

	InterruptMask(xzynq->cfg.irq[0], iid);

	return interrupt_queue(xzynq->iopkt, ient);
}

/**************************************************************************/
/*                                                                        */
/**************************************************************************/
const struct sigevent *xzynq_isr(void *arg, int iid)
{
	xzynq_dev_t *xzynq = arg;
	struct _iopkt_inter *ient = &xzynq->inter;
	uint32_t status;

	/* Disable all interrupts */
	out32(xzynq->regbase + XZYNQ_EMACPS_IDR_OFFSET, XZYNQ_EMACPS_IXR_ALL_MASK);

	/* Clear interrupts */
	status = in32(xzynq->regbase + XZYNQ_EMACPS_ISR_OFFSET);
	out32(xzynq->regbase + XZYNQ_EMACPS_ISR_OFFSET, status);

	return interrupt_queue(xzynq->iopkt, ient);
}

/**************************************************************************/
/*                                                                        */
/**************************************************************************/
int xzynq_process_interrupt(void *arg, struct nw_work_thread *wtp)
{
	xzynq_dev_t *xzynq = arg;
	uint32_t	status;

	/*
	 * Clear receive status
	 * @TODO: Manage the status if there is some errors
	 */
	status = in32(xzynq->regbase + XZYNQ_EMACPS_RXSR_OFFSET);
	out32(xzynq->regbase + XZYNQ_EMACPS_RXSR_OFFSET, status);

	if (status & XZYNQ_EMACPS_RXSR_FRAMERX_MASK) {
		xzynq_process_rx(xzynq, wtp);
	}

	return 1;
}

/**************************************************************************/
/* device_enable_interrupt - enables rx interrupts                        */
/**************************************************************************/
int xzynq_enable_interrupt(void *arg)
{
	xzynq_dev_t *xzynq = arg;

	/* Re-enable the interrupts */
	out32(xzynq->regbase + XZYNQ_EMACPS_IER_OFFSET, XZYNQ_EMACPS_IXR_FRAMERX_MASK);

	return 1;
}

/**************************************************************************/
/* device_enable_interrupt - enables rx interrupts                        */
/**************************************************************************/
int xzynq_enable_interrupt_kermask(void *arg)
{
	xzynq_dev_t *xzynq = arg;

	InterruptUnmask(xzynq->cfg.irq[0], xzynq->iid);

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/intr.c $ $Rev: 752035 $")
#endif
