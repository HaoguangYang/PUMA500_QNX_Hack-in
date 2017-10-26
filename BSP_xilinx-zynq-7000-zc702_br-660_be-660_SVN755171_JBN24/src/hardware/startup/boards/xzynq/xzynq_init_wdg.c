/*
 * $QNXLicenseC: 
 * Copyright 2011,2012  QNX Software Systems.  
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

#include "startup.h"
#include <hw/inout.h>
#include <arm/xzynq.h>
#include "board.h"

// The watchdog timeout value should be specified in board.h.  Set default value to 10 seconds.
#if !defined(WDT_TIMEOUT)
#define WDT_TIMEOUT 10
#endif

#define WDT_SEC_TO_BITS(value)	((((value * XZYNQ_WDT_CLOCK_FREQ) / 512) >> 12)&0xFFF)

/* Enable watch dog */
void xzynq_wdg_enable(void)
{
	out32(XZYNQ_WDT_BASEADDR + XZYNQ_WDT_ZMR_OFFSET, in32(XZYNQ_WDT_BASEADDR
			+ XZYNQ_WDT_ZMR_OFFSET) | XZYNQ_WDT_ZMR_ZKEY_VAL
			| XZYNQ_WDT_ZMR_WDEN_MASK | XZYNQ_WDT_ZMR_RSTEN_MASK);
}

/* Re-load the value of watch-dog timer */
void xzynq_wdg_reload(void)
{
	uint32_t control_val = 0;

	// set timeout value
	control_val |= XZYNQ_WDT_CCR_CKEY_VAL;
	control_val |= XZYNQ_WDT_CCR_PSCALE_0512;
	control_val |= WDT_SEC_TO_BITS(WDT_TIMEOUT) << 2;

	// make sure watchdog is not enabled yet
	out32(XZYNQ_WDT_BASEADDR + XZYNQ_WDT_ZMR_OFFSET, (in32(XZYNQ_WDT_BASEADDR
			+ XZYNQ_WDT_ZMR_OFFSET) & ~XZYNQ_WDT_ZMR_WDEN_MASK) |
			XZYNQ_WDT_ZMR_ZKEY_VAL);

	// load value into control register
	out32(XZYNQ_WDT_BASEADDR + XZYNQ_WDT_CCR_OFFSET, control_val);

	// restart the counter
	out32(XZYNQ_WDT_BASEADDR + XZYNQ_WDT_RESTART_OFFSET,
			XZYNQ_WDT_RESTART_KEY_VAL);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/boards/xzynq/xzynq_init_wdg.c $ $Rev: 753571 $")
#endif
