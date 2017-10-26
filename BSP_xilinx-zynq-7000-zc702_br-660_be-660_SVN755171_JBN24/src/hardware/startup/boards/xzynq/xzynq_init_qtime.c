/*
 * $QNXLicenseC: 
 * Copyright 2012 QNX Software Systems.  
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
 * Xilinx Zynq timer support.
 */

#include "startup.h"
#include <arm/xzynq.h>

#define TIMER_INTERRUPT		29

/*
 * Clock has been hard-coded, it's always 1/2 of the CPU clocks
 * @TODO We should use an API to get the clock freq, because
 * in the case of ZYNC-2 or -3 the freq is different
 */
#define XZYNQ_CLOCK_SCALE  		-15
#define XZYNQ_CLOCK_FREQ		333000000UL
/* XZYNQ_CLOCK_RATE = (1/XZYNQ_CLOCK_FREQ)*10^-XZYNQ_CLOCK_SCALE */
#define XZYNQ_CLOCK_RATE		3003003UL

extern struct callout_rtn timer_load_xzynq;
extern struct callout_rtn timer_value_xzynq;
extern struct callout_rtn timer_reload_xzynq;

extern struct callout_rtn clock_cycles_xzynq;

extern void arm_add_clock_cycles(struct callout_rtn *callout, int incr_bit);

static uintptr_t timer_base = NULL;

static const struct callout_slot timer_callouts[] = {
		{CALLOUT_SLOT(timer_load, _xzynq)},
		{CALLOUT_SLOT(timer_value, _xzynq)},
		{CALLOUT_SLOT(timer_reload, _xzynq)},
	};

/*
 * These functions are used to calibrate the inline delay loop functions.
 * They aren't used after the kernel comes up.
 */
static uint32_t get_timer(void)
{
	return in32(timer_base + XZYNQ_PRIVATE_TIMER_COUNTER);
}

static unsigned timer_start_xzynq(void)
{
	return (uint32_t) get_timer();
}

static unsigned timer_diff_xzynq(unsigned start)
{
	unsigned now = (uint32_t) get_timer();
	return (now - start);
}

void xzynq_init_qtime(void)
{
	struct qtime_entry *qtime = alloc_qtime();
	uint32_t ctrl_reg;

	/*
	 * Map the timer registers.
	 * We are using CPU private timer counter which is
	 * part of MPU Core registers
	 */
	timer_base = startup_io_map(XZYNQ_PRIVATE_TIMER_SIZE,
			XZYNQ_PRIVATE_TIMER_BASEADDR);

	/* Setup the Load register */
	out32(timer_base + XZYNQ_PRIVATE_TIMER_LOAD, 0xFFFFFFFF);

	/*
	 * Select clock input source:
	 * - Enable the timer
	 * - Enable the auto reload
	 * - Enable IRQ
	 * - Prescaler is not set
	 * @TODO: Calculate the best prescaler value to reduce
	 * the round errors
	 */
	ctrl_reg = XZYNQ_PRIVATE_TIMER_CTRL_EN
			| XZYNQ_PRIVATE_TIMER_CTRL_AUTO_RELOAD
			| XZYNQ_PRIVATE_TIMER_CTRL_IRQ;
	out32(timer_base + XZYNQ_PRIVATE_TIMER_CTRL, ctrl_reg);

	timer_start = timer_start_xzynq;
	timer_diff = timer_diff_xzynq;

	qtime->intr = TIMER_INTERRUPT;
	qtime->timer_rate = XZYNQ_CLOCK_RATE;
	qtime->timer_scale = XZYNQ_CLOCK_SCALE;
	qtime->cycles_per_sec = (uint64_t) XZYNQ_CLOCK_FREQ;

	/*
	 * Generic timer registers are banked per-cpu so ensure that the
	 * system clock is only operated on via cpu0
	 */
	qtime->flags |= QTIME_FLAG_TIMER_ON_CPU0;

	add_callout_array(timer_callouts, sizeof(timer_callouts));

	timer_base = startup_io_map(XZYNQ_GLOBAL_TIMER_COUNTER_BASE_MAP_SIZE,
			XZYNQ_GLOBAL_TIMER_COUNTER_BASE);
	ctrl_reg = XZYNQ_GLOBAL_TIMER_CONTROL_TIMER_EN
			| XZYNQ_GLOBAL_TIMER_CONTROL_IRQ;
	out32(timer_base + XZYNQ_GLOBAL_TIMER_CONTROL_REG, ctrl_reg);

	/*
	 * Add clock_cycles callout to directly access 64-bit counter
	 */
	arm_add_clock_cycles(&clock_cycles_xzynq, 0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/boards/xzynq/xzynq_init_qtime.c $ $Rev: 752035 $")
#endif
