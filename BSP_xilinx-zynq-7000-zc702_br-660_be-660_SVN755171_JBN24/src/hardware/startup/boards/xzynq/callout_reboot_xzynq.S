/*
 * $QNXLicenseC: 
 * Copyright 2007,2008, 2010, 2011, 2012 QNX Software Systems.  
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
 
/*
 * Xilinx Zynq system reboot
 */

#include "callout.ah"
#include <arm/xzynq.h>

/*
 * -----------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * -----------------------------------------------------------------------
 */
patch_reboot:
	stmdb	sp!,{r4,lr}
	add		r4, r0, r2

	/*
	 * Map reset control registers
	 */
	mov		r0, #XZYNQ_WDT_SIZE
	ldr		r1, Lpaddr
	bl		callout_io_map

	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!,{r4,pc}

Lpaddr:	.word	XZYNQ_WDT_BASEADDR


CALLOUT_START(reboot_xzynq, 0, patch_reboot)
	/* Get the Watchdog Timer base address (patched) */
	mov     ip,		#0x000000ff
	orr     ip, ip, #0x0000ff00
	orr     ip, ip, #0x00ff0000
	orr     ip, ip, #0xff000000

	/* Set counter to low value to reboot fast */
	mov		r1, #0xff
	lsl		r1, r1, #XZYNQ_WDT_CCR_CRV_SHIFT
	mov		r2, #XZYNQ_WDT_CCR_CKEY
	lsl 	r2, r2, #XZYNQ_WDT_CCR_CKEY_SHIFT
	orr 	r1, r1, r2
	str		r1, [ip, #XZYNQ_WDT_CCR_OFFSET]

	/* Restart the counter */
	movw	r1, #XZYNQ_WDT_RESTART_KEY_VAL
	str		r1, [ip, #XZYNQ_WDT_RESTART_OFFSET]

	/* Enable the WDT with reboot option */
	movw	r2, #XZYNQ_WDT_ZMR_ZKEY
	lsl 	r2, r2, #XZYNQ_WDT_ZMR_ZKEY_SHIFT
	orr 	r2, r2, #XZYNQ_WDT_ZMR_WDEN_MASK
	orr 	r2, r2, #XZYNQ_WDT_ZMR_RSTEN_MASK
	ldr		r1, [ip, #XZYNQ_WDT_ZMR_OFFSET]
	orr		r1, r1, r2
	str		r1, [ip, #XZYNQ_WDT_ZMR_OFFSET]

0:
	b		0b
CALLOUT_END(reboot_xzynq)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/boards/xzynq/callout_reboot_xzynq.S $ $Rev: 752035 $";
.previous
#endif
#endif 
