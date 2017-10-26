/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, 2010, 2011, 2012 QNX Software Systems.  
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
 * Xilinx Zynq specific GPIO interrupt callouts.
 *
 *	r5 - holds the syspageptr				(INTR_GENFLAG_SYSPAGE  set)
 *	r6 - holds the intrinfo_entry pointer	(INTR_GENFLAG_INTRINFO set)
 *	r7 - holds the interrupt mask count		(INTR_GENFLAG_INTRMASK set)
 *
 * The interrupt_id_* routine returns the (controller-relative) level in r4
 */

#include "callout.ah"
#include <arm/xzynq.h>

/*
 * -----------------------------------------------------------------------
 * Patch callout code (for GPIO)
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * -----------------------------------------------------------------------
 */
interrupt_patch_gpio:
	stmdb	sp!, {r4, lr}
	add		r4, r0, r2	// address of callout routine

	ldr		r0, Lvaddr
	cmp		r0, #-1
	bne		1f

	/*
	 * Map registers
	 */
	mov		r0, #XZYNQ_GPIO_SIZE
	ldr		r1, Lpaddr
	bl		callout_io_map
	str		r0, Lvaddr

1:
	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!, {r4, pc}

Lpaddr:	.word	XZYNQ_GPIO_BASE
Lvaddr:	.word	-1

/*
 * -----------------------------------------------------------------------
 * Identify GPIO interrupt source.
 *
 * Returns interrupt number in r4
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_id_xzynq_gpio, 0, interrupt_patch_gpio)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	push	{r5-r9}

	/*
	 * Scan for first set bit in each bank
	 */
	mov		r5, #4 /* 4 banks */
	/* Each banks */
0:
	subs	r5, r5, #1
	blt		2f
	/*
	 * Read Interrupt Mask and Status of the first bank
	 */
	lsl		r8, r5, #0x6 /* bank n * 0x40 = offset bank n */

	movw	r2, #XZYNQ_GPIOPS_INTSTS_OFFSET
	add		r2, r2, r8
	ldr		r6, [ip, r2]

	movw	r7, #XZYNQ_GPIOPS_INTMASK_OFFSET
	add		r7, r7, r8
	ldr		r7, [ip, r7]

	movw	r9, #XZYNQ_GPIOPS_INTDIS_OFFSET
	add		r9, r9, r8

	bic 	r6, r6, r7

	/* Each bits of the bank */
	clz     r4, r6
	cmp		r4, #32
	beq		0b

	/* Get the shift */
	mov		r7, #31
	subs	r6, r7, r4

	/*
	 * Mask the interrupt source
	 */
	mov		r1, #1
	mov		r1, r1, lsl r6
	str		r1, [ip, r9]

	/*
	 * Clear interrupt status
	 */
	str		r1, [ip, r2]

	/*
	 * Calculate the IRQ number with the bank
	 */
	add r5, r5, #1
	mul r4, r6, r5

2:
	pop		{r5-r9}

CALLOUT_END(interrupt_id_xzynq_gpio)

/*
 * -----------------------------------------------------------------------
 * Acknowledge specified GPIO interrupt
 *
 * On entry:
 *	r4 contains the interrupt number
 *	r7 contains the interrupt mask count
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_eoi_xzynq_gpio, 0, interrupt_patch_gpio)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	push	{r4-r9}

    /*
     * Only unmask interrupt if mask count is zero
     */
	teq		r7, #0
	bne		0f

	mov 	r3, #5
	mov		r5, r4, lsr r3 /* r5 = num of the bank */
	mov 	r7, #0x40

	mov 	r6, r5
	/* IT num mod 32 */
	mov		r6, r6, lsr r3
	subs	r1, r4, r6

	mul		r5, r5, r7  /* r5 = bank n * 0x40 */

	ldr		r3, =XZYNQ_GPIOPS_INTEN_OFFSET
	add		r3, r3, r5   /* r3 = offset bank n */

	mov		r2, #1
	mov		r2, r2, lsl r1 /* IT to clear (1 << r4) */
	str		r2, [ip, r3]   /* Ack the irq */
0:
	pop		{r4-r9}

CALLOUT_END(interrupt_eoi_xzynq_gpio)

/*
 * -----------------------------------------------------------------------
 * Mask specified GPIO interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_mask_xzynq_gpio, 0, interrupt_patch_gpio)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	push	{r4-r9, lr}

	mov 	r3, #5
	mov		r5, r1, lsr r3 /* r5 = num of the bank */
	mov 	r7, #0x40

	/* IT num mod 32 */
	mov 	r6, r5
	mov		r6, r6, lsr r3
	subs	r8, r1, r6

	mul		r5, r5, r7  /* r5 = bank n * 0x40 */

	ldr		r3, =XZYNQ_GPIOPS_INTDIS_OFFSET
	add		r3, r3, r5

	mov		r2, #1
	mov		r2, r2, lsl r8 /* IT to mask (1 << r4) */
	str		r2, [ip, r3]   /* Ack the irq */

    mov     r0, #0
	pop		{r4-r9, pc}
CALLOUT_END(interrupt_mask_xzynq_gpio)

/*
 * -----------------------------------------------------------------------
 * Unmask specified GPIO interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_unmask_xzynq_gpio, 0, interrupt_patch_gpio)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	push	{r4-r9, lr}

	mov 	r3, #5
	mov		r5, r1, lsr r3 /* r5 = num of the bank */
	mov 	r7, #0x40

	/* IT num mod 32 */
	mov 	r6, r5
	mov		r6, r6, lsr r3
	subs	r8, r1, r6

	mul		r5, r5, r7  /* r5 = bank n * 0x40 */

	ldr		r3, =XZYNQ_GPIOPS_INTEN_OFFSET
	add		r3, r3, r5

	mov		r2, #1
	mov		r2, r2, lsl r8 /* IT to mask (1 << r4) */
	str		r2, [ip, r3]   /* Ack the irq */

    mov     r0, #0
	pop		{r4-r9, pc}
CALLOUT_END(interrupt_unmask_xzynq_gpio)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/boards/xzynq/callout_interrupt_xzynq_gpio.S $ $Rev: 752035 $";
.previous
#endif
#endif 
