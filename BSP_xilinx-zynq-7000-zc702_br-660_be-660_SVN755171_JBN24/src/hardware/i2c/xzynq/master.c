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

i2c_status_t setup_master(xzynq_dev_t *dev, uint8_t role, unsigned int len,
		unsigned int stop) {
	uint32_t ctrl_reg;

	/* Check bus busy only if it's not a repeated start */
	if (!(in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET)
			& XZYNQ_XIICPS_CR_HOLD_MASK)) {
		if (xzynq_wait_bus_not_busy(dev)) {
			return I2C_STATUS_BUSY;
		}
	}

	/* Set MS, ENACK, CLR_FIFO bits, and RW bit. */
	ctrl_reg = in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET);
	ctrl_reg |= XZYNQ_XIICPS_CR_MS_MASK;
	ctrl_reg |= XZYNQ_XIICPS_CR_ACKEN_MASK;
	ctrl_reg |= XZYNQ_XIICPS_CR_CLR_FIFO_MASK;
	if (role == XZYNQ_ROLE_MASTER_RECEIVE)
		ctrl_reg |= XZYNQ_XIICPS_CR_RD_WR_MASK;
	else
		ctrl_reg &= ~XZYNQ_XIICPS_CR_RD_WR_MASK;
	out16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);

	/*
	 * If required, set the HOLD bit.
	 * Otherwise write the first byte of data to the I2C Data register.
	 */
	if (!stop || len > XZYNQ_XIICPS_FIFO_DEPTH) {
		ctrl_reg = in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET);
		ctrl_reg |= XZYNQ_XIICPS_CR_HOLD_MASK;
		out16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);
	} else {
		ctrl_reg = in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET);
		ctrl_reg &= ~XZYNQ_XIICPS_CR_HOLD_MASK;
		out16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);
	}

	return I2C_STATUS_DONE;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/master.c $ $Rev: 752035 $")
#endif
