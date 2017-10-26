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

/*****************************************************************************/
/**
*
* This function resets the XADC Hard Macro in the device.
*
* @param	dev is a pointer to the xadc_dev_t instance
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void
xadc_reset (xadc_dev_t *dev)
{
	out32(dev->regbase + XZYNQ_XADC_MCTL_OFFSET, XZYNQ_XADC_RESET_EN);
	out32(dev->regbase + XZYNQ_XADC_MCTL_OFFSET, XZYNQ_XADC_RESET_DIS);
}

/*****************************************************************************/
/**
*
* This function initializes the XADC. This function
* must be called prior to using the XADC device.
*
* @param	dev is a pointer to the xadc_dev_t instance
*
* @return - None.
*
* @note		The user needs to first call the XAdcPs_LookupConfig() API
*		which returns the Configuration structure pointer which is
*		passed as a parameter to the XAdcPs_CfgInitialize() API.
*
******************************************************************************/
void
xadc_cfg_init (xadc_dev_t *dev)
{
	_Uint32t data;

	/* Unlock the Device Config register */
	out32(dev->regbase + XZYNQ_DEVCFG_UNLOCK_OFFSET, XZYNQ_DEVCFG_UNLOCK_KEY);

	/* Enable the XADC for PS use and set FIFO thresholds*/
	data = in32(dev->regbase + XZYNQ_XADC_CFG_OFFSET);

	out32(dev->regbase + XZYNQ_XADC_CFG_OFFSET, data | XZYNQ_XADC_CFG_ENABLE_MASK | XZYNQ_XADC_CFG_CFIFOTH_MASK | XZYNQ_XADC_CFG_DFIFOTH_MASK);
	data = in32(dev->regbase + XZYNQ_XADC_CFG_OFFSET);

	/* Release XADC from reset */
	out32(dev->regbase + XZYNQ_XADC_MCTL_OFFSET, 0x00);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/xadc/init.c $ $Rev: 752035 $")
#endif
