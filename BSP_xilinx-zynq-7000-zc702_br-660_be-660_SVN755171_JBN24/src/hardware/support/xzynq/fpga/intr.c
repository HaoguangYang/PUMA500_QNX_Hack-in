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

void fpga_intr_enable(fpga_dev_t *dev, _Uint32t mask)
{
	_Uint32t reg_val;

	/*
	 * Enable the specified interrupts in the Interrupt Mask Register.
	 */
	reg_val = in32(dev->devcfg_regbase + XZYNQ_FPGA_INT_MASK_OFFSET);
	reg_val &= ~(mask & XZYNQ_FPGA_IXR_ALL_MASK);
	out32(dev->devcfg_regbase + XZYNQ_FPGA_INT_MASK_OFFSET, reg_val);
}

void fpga_intr_disable(fpga_dev_t *dev, _Uint32t mask)
{
	_Uint32t reg_val;

	/*
	 * Disable the specified interrupts in the Interrupt Mask Register.
	 */
	reg_val = in32(dev->devcfg_regbase + XZYNQ_FPGA_INT_MASK_OFFSET);
	reg_val |= (mask & XZYNQ_FPGA_IXR_ALL_MASK);
	out32(dev->devcfg_regbase + XZYNQ_FPGA_INT_MASK_OFFSET, reg_val);
}

_Uint32t fpga_intr_get_enable(fpga_dev_t *dev)
{
	/*
	 * Return the value read from the Interrupt Mask Register.
	 */
	return (~in32(dev->devcfg_regbase + XZYNQ_FPGA_INT_MASK_OFFSET));
}

_Uint32t fpga_intr_get_status(fpga_dev_t *dev)
{
	/*
	 * Return the value read from the Interrupt Status register.
	 */
	return in32(dev->devcfg_regbase + XZYNQ_FPGA_INT_STS_OFFSET);
}

void fpga_intr_clear(fpga_dev_t *dev, _Uint32t mask)
{
	_Uint32t reg_val;

	/*
	 * Clear the specified interrupts in the Interrupt Status register.
	 */
	reg_val = in32(dev->devcfg_regbase + XZYNQ_FPGA_INT_STS_OFFSET);
	reg_val |= (mask & XZYNQ_FPGA_IXR_ALL_MASK);
	out32(dev->devcfg_regbase + XZYNQ_FPGA_INT_STS_OFFSET, reg_val);

}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/fpga/intr.c $ $Rev: 752035 $")
#endif
