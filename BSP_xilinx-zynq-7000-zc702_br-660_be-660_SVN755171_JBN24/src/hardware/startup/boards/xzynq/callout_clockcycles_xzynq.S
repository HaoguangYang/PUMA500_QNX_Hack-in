/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, 2010, 2011 QNX Software Systems.  
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
 * Xilinx Zynq specific Clock Cycles support
 */

#include "callout.ah"
#include <arm/xzynq.h>

/*
 * --------------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * --------------------------------------------------------------------------
 */
patch_timer:
	stmdb	sp!, {r4, lr}
	add		r4, r0, r2	// address of callout routine

	ldr		r0, Lvaddr
	cmp		r0, #-1
	bne		1f

	/*
	 * Map registers
	 */
	mov		r0, #XZYNQ_GLOBAL_TIMER_COUNTER_BASE_MAP_SIZE
	ldr		r1, Lpaddr
	bl		callout_io_map
	str		r0, Lvaddr

1:
	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!, {r4, pc}

Lpaddr:	.word	XZYNQ_GLOBAL_TIMER_COUNTER_BASE
Lvaddr:	.word	-1

CALLOUT_START(clock_cycles_xzynq, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Return current timer counter value
	 */
1:
	ldr		r1, [ip, #XZYNQ_GLOBAL_TIMER_COUNTER_REG1]
	ldr		r0, [ip, #XZYNQ_GLOBAL_TIMER_COUNTER_REG0]
	ldr		r2, [ip, #XZYNQ_GLOBAL_TIMER_COUNTER_REG1]
	cmp 	r1, r2
	bne 	1b

	mov		pc, lr
CALLOUT_END(clock_cycles_xzynq)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/boards/xzynq/callout_clockcycles_xzynq.S $ $Rev: 752035 $";
.previous
#endif
#endif 
