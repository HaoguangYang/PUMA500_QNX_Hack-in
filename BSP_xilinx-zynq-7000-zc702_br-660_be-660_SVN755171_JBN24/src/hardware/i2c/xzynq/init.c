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

/* open() ... */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hw/clk.h>

void xzynq_reset(xzynq_dev_t* dev) {
	uint32_t ctrl_reg;

	/* Disable IRQs */
	out16(dev->regbase + XZYNQ_XIICPS_IDR_OFFSET,
			XZYNQ_XIICPS_IXR_ALL_INTR_MASK);

	/* Set max timeout */
	out8(dev->regbase + XZYNQ_XIICPS_TIME_OUT_OFFSET, 0xFF);

	/*
	 * Reset the controller:
	 * 	- Clear the FIFO
	 * 	- Clear the HOLD bit to don't hold the bus busy
	 */
	ctrl_reg = in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET);
	ctrl_reg &= ~XZYNQ_XIICPS_CR_HOLD_MASK;
	ctrl_reg |= XZYNQ_XIICPS_CR_CLR_FIFO_MASK;
	out16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);

	/* Clear Interrupt register */
	out16(dev->regbase + XZYNQ_XIICPS_ISR_OFFSET, 0xFFFF);
}

void *
xzynq_init(int argc, char *argv[]) {
	xzynq_dev_t *dev;
	clk_freq_t clk_freq;
	int fd, err;

	if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
		perror("ThreadCtl");
		return NULL;
	}

	dev = malloc(sizeof(xzynq_dev_t));
	if (!dev)
		return NULL;

	if (xzynq_options(dev, argc, argv) == -1)
		goto fail;

	dev->regbase = mmap_device_io(dev->reglen, dev->physbase);
	if (dev->regbase == (uintptr_t) MAP_FAILED) {
		perror("mmap_device_io");
		goto fail;
	}

	/* Get I2C Clock */
	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("Can't get I2C clock");
		goto fail;
	}

	clk_freq.id = XZYNQ_I2C_CLK;
	err = devctl(fd, DCMD_CLOCK_GET_FREQ, &clk_freq, sizeof(clk_freq_t), NULL);
	if (err) {
		perror("devctl");
		goto fail;
	}

	if (clk_freq.freq == -1) {
		perror("/dev/clock: Invalid frequency (-1)");
		goto fail;
	}

	/* Set real frequency */
	dev->bus_freq = clk_freq.freq;
	close(fd);

	/* Initialize interrupt handler */
	SIGEV_INTR_INIT(&dev->intrevent);
	dev->iid = InterruptAttach(dev->intr, xzynq_intr, dev, 0, 0);
	if (dev->iid == -1) {
		perror("InterruptAttach");
		goto fail_int_attach;
	}

	/* Reset the i2c controller */
	xzynq_reset(dev);

	return dev;

fail_int_attach:
	munmap_device_io(dev->regbase, dev->reglen);
fail:
	free(dev);
	return NULL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/init.c $ $Rev: 752035 $")
#endif
