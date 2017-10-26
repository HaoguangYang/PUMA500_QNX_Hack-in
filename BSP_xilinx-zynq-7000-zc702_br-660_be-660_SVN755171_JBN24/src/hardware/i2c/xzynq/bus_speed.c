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

int xzynq_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed) {
	xzynq_dev_t *dev = hdl;
	uint32_t ctrl_reg;
	uint16_t div_a, div_b;

	/* Check the speed parameter */
	if (speed > 400000 || speed < 8000) {
		fprintf(stderr, "i2c-xzynq:  Invalid bus speed(%d)\n", speed);
		errno = EINVAL;
		return -1;
	}

	/* Divisor A range is 1 - 4 */
	for (div_a = 1; div_a < 4; div_a++) {
		if ((dev->bus_freq / div_a) <= speed)
			break;
	}

	/* Divisor B range is 1 - 64 */
	for (div_b = 1; div_b < 64; div_b++) {
		if (((dev->bus_freq / div_a) / div_b) <= speed)
			break;
	}

	/* Set new values for divisor A/B */
	ctrl_reg = in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET);
	ctrl_reg &= ~(XZYNQ_XIICPS_CR_DIV_A_MASK);
	ctrl_reg &= ~(XZYNQ_XIICPS_CR_DIV_B_MASK);

	ctrl_reg |= ((div_a - 1) << XZYNQ_XIICPS_CR_DIV_A_SHIFT)
			& XZYNQ_XIICPS_CR_DIV_A_MASK;
	ctrl_reg |= ((div_b - 1) << XZYNQ_XIICPS_CR_DIV_B_SHIFT)
			& XZYNQ_XIICPS_CR_DIV_B_MASK;

	out16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);

	dev->speed = (dev->bus_freq / div_a) / div_b;

	if (ospeed)
		*ospeed = dev->speed;

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/bus_speed.c $ $Rev: 752035 $")
#endif
