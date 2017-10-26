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

int xzynq_set_slave_addr(void *hdl, unsigned int addr, i2c_addrfmt_t fmt) {
	xzynq_dev_t *dev = hdl;
	uint32_t ctrl_reg;
	uint32_t addr_reg;

	/*
	 * Write to the control register to set up
	 * SCL speed and addressing mode.
	 */
	ctrl_reg = in16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET);
	switch (fmt) {
	case I2C_ADDRFMT_7BIT:
		ctrl_reg |= XZYNQ_XIICPS_CR_NEA_MASK;
		addr_reg = addr & 0x007F;
		break;
	case I2C_ADDRFMT_10BIT:
		ctrl_reg &= ~XZYNQ_XIICPS_CR_NEA_MASK;
		addr_reg = addr & 0x03FF;
		break;
	default:
		return -1;
	}

	dev->slave_addr = addr;
	out16(dev->regbase + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/slave_addr.c $ $Rev: 752035 $")
#endif
