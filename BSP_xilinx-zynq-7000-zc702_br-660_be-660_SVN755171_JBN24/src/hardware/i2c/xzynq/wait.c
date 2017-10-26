/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
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

#include "proto.h"

uint32_t xzynq_wait_bus_not_busy(xzynq_dev_t *dev) {
	/* Wait no more than 1s */
	unsigned tries = 1000;

	while (in16(dev->regbase + XZYNQ_XIICPS_SR_OFFSET)
			& XZYNQ_XIICPS_SR_BA_MASK) {
		delay(1);

		if (tries-- <= 0)
			return -1;
	}

	return 0;
}

const struct sigevent *xzynq_intr(void *area, int id) {
	xzynq_dev_t *dev = area;
	uint16_t status_reg, ctrl_reg;
	uint32_t bytes_to_send, available;

	status_reg = in16(dev->regbase + XZYNQ_XIICPS_ISR_OFFSET);

	if (status_reg & XZYNQ_XIICPS_IXR_DATA_MASK) {
		if (dev->size_to_receive > XZYNQ_XIICPS_FIFO_DEPTH) {
			/* Process data that we have just received */
			while (in16(dev->regbase + XZYNQ_XIICPS_SR_OFFSET)
					& XZYNQ_XIICPS_SR_RXDV_MASK) {
				*(dev->buf)++ = in16(dev->regbase + XZYNQ_XIICPS_DATA_OFFSET);
				dev->size_to_receive--;
			}

			/* Request next data */
			if (dev->size_to_receive > XZYNQ_XIICPS_FIFO_DEPTH) {
				out16(dev->regbase + XZYNQ_XIICPS_TRANS_SIZE_OFFSET,
						XZYNQ_XIICPS_FIFO_DEPTH + 1);
			} else {
				out16(dev->regbase + XZYNQ_XIICPS_TRANS_SIZE_OFFSET,
						dev->size_to_receive);

				/* Clear the hold bit to send the STOP */
				ctrl_reg = in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET);
				ctrl_reg &= ~XZYNQ_XIICPS_CR_HOLD_MASK;
				out16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);
			}
		}
	}

	if (status_reg & XZYNQ_XIICPS_IXR_COMP_MASK) {
		/* Receive direction */
		if (!dev->direction) {
			while (in16(dev->regbase + XZYNQ_XIICPS_SR_OFFSET)
					& XZYNQ_XIICPS_SR_RXDV_MASK) {
				*(dev->buf)++ = in16(dev->regbase + XZYNQ_XIICPS_DATA_OFFSET);
				dev->size_to_receive--;
			}

			dev->status = I2C_STATUS_DONE;

			/* Send direction */
		} else {
			if (dev->size_to_send > 0) {
				available = XZYNQ_XIICPS_FIFO_DEPTH - in16(dev->regbase
						+ XZYNQ_XIICPS_TRANS_SIZE_OFFSET);

				if (dev->size_to_send > available) {
					bytes_to_send = available;
				} else {
					bytes_to_send = dev->size_to_send;
				}

				while (bytes_to_send--) {
					out16(dev->regbase + XZYNQ_XIICPS_DATA_OFFSET,
							*(dev->buf)++);
					dev->size_to_send--;
				}
			} else {
				dev->status = I2C_STATUS_DONE;
			}
		}
	}

	if (status_reg & XZYNQ_XIICPS_IXR_ARB_LOST_MASK) {
		dev->status = I2C_STATUS_ARBL;
	}

	if (status_reg & XZYNQ_XIICPS_IXR_NACK_MASK) {
		dev->status = I2C_STATUS_NACK;
	}

	if (status_reg & XZYNQ_XIICPS_IXR_TO_MASK) {
		dev->status = I2C_STATUS_ERROR;
	}

	/* Clear the interrupt */
	out16(dev->regbase + XZYNQ_XIICPS_ISR_OFFSET, status_reg);

	if (dev->status != I2C_STATUS_DONE) {
		return NULL;
	}

	if (dev->stop) {
		ctrl_reg = in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET);
		ctrl_reg &= ~XZYNQ_XIICPS_CR_HOLD_MASK;
		out16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);
	}

	return &dev->intrevent;
}

void xzynq_wait_status(xzynq_dev_t *dev) {
	uint64_t ntime = 500000000ULL;

	dev->status = I2C_STATUS_ERROR;
	TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_INTR, NULL, &ntime, NULL);
	InterruptWait_r(0, NULL);
	InterruptUnmask(dev->intr, dev->iid);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/wait.c $ $Rev: 752035 $")
#endif
