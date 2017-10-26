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
 * Xilinx Zynq specific timer support
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
	mov		r0, #XZYNQ_PRIVATE_TIMER_SIZE
	ldr		r1, Lpaddr
	bl		callout_io_map
	str		r0, Lvaddr

1:
	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!, {r4, pc}

Lpaddr:	.word	XZYNQ_PRIVATE_TIMER_BASEADDR
Lvaddr:	.word	-1

/*
 * --------------------------------------------------------------------------
 * Set clock resolution, and enable interrupt triggering as a side effect.
 * The interrupt remains masked until it is enabled via intr_unmask
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_load_xzynq, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/* Disable IRQ/AutoReload/Timer */
	ldr		r2, [ip, #XZYNQ_PRIVATE_TIMER_CTRL]
	bic		r2, r2, #XZYNQ_PRIVATE_TIMER_CTRL_EN
	bic		r2, r2, #XZYNQ_PRIVATE_TIMER_CTRL_AUTO_RELOAD
	bic		r2, r2, #XZYNQ_PRIVATE_TIMER_CTRL_IRQ
	str		r2, [ip, #XZYNQ_PRIVATE_TIMER_CTRL]

	/* Load counter value */
	ldr		r2, [r1, #QT_TIMER_LOAD]
	str		r2, [ip, #XZYNQ_PRIVATE_TIMER_LOAD]
	str		r2, [ip, #XZYNQ_PRIVATE_TIMER_COUNTER]

	/* Start IRQ/AutoReload/Timer */
	ldr		r2, [ip, #XZYNQ_PRIVATE_TIMER_CTRL]
	orr		r2, r2, #XZYNQ_PRIVATE_TIMER_CTRL_EN
	orr		r2, r2, #XZYNQ_PRIVATE_TIMER_CTRL_AUTO_RELOAD
	orr		r2, r2, #XZYNQ_PRIVATE_TIMER_CTRL_IRQ
	str		r2, [ip, #XZYNQ_PRIVATE_TIMER_CTRL]

	mov		pc, lr
CALLOUT_END(timer_load_xzynq)


/*
 * --------------------------------------------------------------------------
 * Read the current timer value, relative to the last clock tick
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 *
 * FIXME: this doesn't deal with the counter wrapping, eg. ClockCycles just
 *		  at the point where the clock interrupt is triggerred.
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_value_xzynq, 0, patch_timer)
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
	ldr		r0, [ip, #XZYNQ_PRIVATE_TIMER_COUNTER]
	ldr		r1, [r1, #QT_TIMER_LOAD]
	sub		r0, r1, r0

	mov		pc, lr
CALLOUT_END(timer_value_xzynq)


/*
 * --------------------------------------------------------------------------
 * Clear timer interrupt.
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_reload_xzynq, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/* Ack the IRQ by writting 1 */
	mov		r2,	#1
	str		r2, [ip, #XZYNQ_PRIVATE_TIMER_INT_STS]

	mov		pc, lr
CALLOUT_END(timer_reload_xzynq)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/boards/xzynq/callout_timer_xzynq.S $ $Rev: 752035 $";
.previous
#endif
#endif 
