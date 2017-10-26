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

i2c_status_t xzynq_recv(void *hdl, void *buf, unsigned int len,
		unsigned int stop) {
	xzynq_dev_t *dev = hdl;

	if (len <= 0) {
		return I2C_STATUS_DONE;
	}
	dev->size_to_receive = len;
	dev->direction = 0;
	dev->buf = buf;
	dev->stop = stop;

	/* Setup the master to receive data */
	setup_master(dev, XZYNQ_ROLE_MASTER_RECEIVE, len, stop);

	/* Start transfer by writing the address */
	out16(dev->regbase + XZYNQ_XIICPS_ADDR_OFFSET, dev->slave_addr);

	/* Write nb of bytes to be receive */
	if (dev->size_to_receive > XZYNQ_XIICPS_FIFO_DEPTH)
		out16(dev->regbase + XZYNQ_XIICPS_TRANS_SIZE_OFFSET,
				XZYNQ_XIICPS_FIFO_DEPTH + 1);
	else
		out16(dev->regbase + XZYNQ_XIICPS_TRANS_SIZE_OFFSET, len);

	/* Clear the interrupts */
	out16(dev->regbase + XZYNQ_XIICPS_ISR_OFFSET, 0xFF);

	/* Enable IRQs */
	out16(dev->regbase + XZYNQ_XIICPS_IER_OFFSET, XZYNQ_XIICPS_IXR_COMP_MASK
			| XZYNQ_XIICPS_IXR_DATA_MASK | XZYNQ_XIICPS_IXR_NACK_MASK
			| XZYNQ_XIICPS_IXR_TO_MASK | XZYNQ_XIICPS_IXR_ARB_LOST_MASK
			| XZYNQ_XIICPS_IXR_RX_OVR_MASK
	);

	/* Wait for the total transfer */
	xzynq_wait_status(dev);

	/* Disable IRQs */
	out16(dev->regbase + XZYNQ_XIICPS_IDR_OFFSET,
			XZYNQ_XIICPS_IXR_ALL_INTR_MASK);

	if (dev->status != I2C_STATUS_DONE) {
		xzynq_reset(dev);
	}

	return dev->status;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/recv.c $ $Rev: 752035 $")
#endif
