/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include "xzynq_spi.h"

#include <sys/neutrino.h>

static intrspin_t spi_lock;

/*
 * We use the same buffer for transmit and receive
 * For exchange, that's exactly what we wanted
 * For Read, it doesn't matter what we write to SPI, so we are OK.
 * For transmit, the receive data is put at the buffer we just transmitted, we are still OK.
 */

static const struct sigevent *spi_intr(void *area, int id)
{
	uint32_t reg_value = 0;
	struct sigevent *ret = NULL;
	uint32_t data;
	uintptr_t base;
	xzynq_spi_t *dev = area;
	base = dev->vbase;

	InterruptLock(&spi_lock);
	reg_value = in32(base + XZYNQ_SPI_ISR_OFFSET);

	/* Clear the interrupts */
	out32(base + XZYNQ_SPI_ISR_OFFSET, reg_value);

	/* Disable all interrupts */
	out32(base + XZYNQ_SPI_IDR_OFFSET, 0xFFFFFFFF);

	/*
	 * If a fault happened return message as requested bytes count
	 * will not be 0
	 */
	if (reg_value & XZYNQ_SPI_IXR_MODF) {
		ret = (&dev->spievent);
		goto end;
	}

	if (reg_value & (XZYNQ_SPI_IXR_TXOW | XZYNQ_SPI_IXR_RXNEMPTY)) {
		/* Empty the Rx FIFO */
		while ((in32(dev->vbase + XZYNQ_SPI_ISR_OFFSET)
				& XZYNQ_SPI_IXR_RXNEMPTY) && (dev->rlen < dev->xlen)) {

			/* Errata AR#47575, read status twice */
			if(!(in32(dev->vbase + XZYNQ_SPI_ISR_OFFSET)& XZYNQ_SPI_IXR_RXNEMPTY))
				continue;

			data = in32(dev->vbase + XZYNQ_SPI_RXD_OFFSET);
			switch (dev->dlen) {
			case 1:
				dev->pbuf[dev->rlen] = (uint8_t)data;
				dev->rlen++;
				break;
			case 2:
				*(uint16_t*)(&dev->pbuf[dev->rlen]) = (uint16_t)data;
				dev->rlen += 2;
				break;
			case 3:
			case 4:
				*(uint32_t*)(&dev->pbuf[dev->rlen]) = (uint32_t)data;
				dev->rlen += 4;
				break;
			}
		}

		/* If some data still needs to be sent */
		if (dev->tlen < dev->xlen) {
			while ((dev->tlen < dev->xlen) && !(in32(dev->vbase +
					XZYNQ_SPI_ISR_OFFSET)&XZYNQ_SPI_IXR_TXFULL)) {
				switch (dev->dlen) {
				case 1:
					out32(base + XZYNQ_SPI_TXD_OFFSET, dev->pbuf[dev->tlen]);
					dev->tlen++;
					break;
				case 2:
					out32(base + XZYNQ_SPI_TXD_OFFSET, *(uint16_t*)(&dev->pbuf[dev->tlen]));
					dev->tlen += 2;
					break;
				case 3:
				case 4:
					out32(base + XZYNQ_SPI_TXD_OFFSET, *(uint32_t*)(&dev->pbuf[dev->tlen]));
					dev->tlen += 4;
					break;
				}
			}
			out32(dev->vbase + XZYNQ_SPI_IER_OFFSET, XZYNQ_SPI_IXR_TXOW | XZYNQ_SPI_IXR_MODF);
			xzynq_spi_start_exchange(dev);
		}

		/* Is the transfer complete? */
		if (dev->rlen < dev->xlen)
			out32(dev->vbase + XZYNQ_SPI_IER_OFFSET, XZYNQ_SPI_IXR_RXNEMPTY | XZYNQ_SPI_IXR_MODF);
		else
			ret = (&dev->spievent);
	}
end:
	InterruptUnlock(&spi_lock);
	return ret;
}

int xzynq_attach_intr(xzynq_spi_t *xzynq)
{
	if ((xzynq->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK))
			== -1)
		return -1;

	if ((xzynq->coid = ConnectAttach(0, 0, xzynq->chid, _NTO_SIDE_CHANNEL, 0))
			== -1)
		goto fail0;

	xzynq->spievent.sigev_notify = SIGEV_PULSE;
	xzynq->spievent.sigev_coid = xzynq->coid;
	xzynq->spievent.sigev_code = XZYNQ_SPI_EVENT;
	xzynq->spievent.sigev_priority = XZYNQ_SPI_PRIORITY;

	ThreadCtl(_NTO_TCTL_IO, 0); // required for InterruptLock/Unlock abilities
	memset(&spi_lock, 0, sizeof(spi_lock));
	/* Attach SPI interrupt */
	xzynq->iid_spi = InterruptAttach(xzynq->irq_spi, spi_intr, xzynq,
			sizeof(xzynq_spi_t), _NTO_INTR_FLAGS_TRK_MSK);

	if (xzynq->iid_spi != -1) {
#ifdef DEBUG
		fprintf(stderr, "%s: iid_spi = %08x for irq #%d\n", __FUNCTION__,
				xzynq->iid_spi, xzynq->irq_spi);
#endif
		return 0;
	}

	ConnectDetach(xzynq->coid);
fail0:
	ChannelDestroy(xzynq->chid);

	return -1;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/spi/xzynq/intr.c $ $Rev: 752035 $")
#endif
