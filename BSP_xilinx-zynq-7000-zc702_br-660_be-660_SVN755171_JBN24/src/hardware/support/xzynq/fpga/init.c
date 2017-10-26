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

#include <proto.h>

/*
 * TODO: Comments
 */
void fpga_cfg_init(fpga_dev_t *dev)
{
	/* Unlock the Device Config register */
	out32(dev->devcfg_regbase + XZYNQ_DEVCFG_UNLOCK_OFFSET,
			XZYNQ_DEVCFG_UNLOCK_KEY);
}

/*
 * See section 6.4.5  PL configuration in UG585 Zynq-7000 AP SoC TRM
 * Beware: fpga_disable_axi must be called prior to this to
 *         set FPGA_RST_CTRL and clear LVL_SHFTR_EN
 */
void fpga_reset(fpga_dev_t *dev)
{
	_Uint32t ctrl;
	_Uint32t status;

	ctrl = in32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET);

	/* clear PCFG_PROG_B in CTRL[30] to put FPGA in reset */
	out32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET,
			ctrl & ~XZYNQ_FPGA_CTRL_PCFG_PROG_B_MASK);

	/* Wait for PCFG_INIT in STATUS[4] to go low */
	do {
		status = in32(dev->devcfg_regbase + XZYNQ_FPGA_STATUS_OFFSET);
	} while (status & XZYNQ_FPGA_STATUS_PCFG_INIT_MASK);

	/* set PCFG_PROG_B in CTRL[30] to take FPGA out of reset */
	out32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET,
			ctrl | XZYNQ_FPGA_CTRL_PCFG_PROG_B_MASK);

	/* Wait for PCFG_INIT in STATUS[4] to go high when housecleaning done */
	do {
		status = in32(dev->devcfg_regbase + XZYNQ_FPGA_STATUS_OFFSET);
	} while (!(status & XZYNQ_FPGA_STATUS_PCFG_INIT_MASK));

	/* clear all INT_STS bits */
	out32(dev->devcfg_regbase + XZYNQ_FPGA_INT_STS_OFFSET,
			in32(dev->devcfg_regbase + XZYNQ_FPGA_INT_STS_OFFSET));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/fpga/init.c $ $Rev: 752035 $")
#endif
