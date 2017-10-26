/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems.
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

#include <stdint.h>
#include <hw/inout.h>
#include "timer.h"

#define TIMER_HZ 333000000

void msleep(unsigned long value)
{
	timer_set(value);
	while (timer_remaining() > 0)
		;
}

void timer_set(unsigned long ms)
{
	uint32_t ctrl_reg;

	/* Stop the timer */
	ctrl_reg = in32(XZYNQ_PRIVATE_TIMER_BASEADDR + XZYNQ_PRIVATE_TIMER_CTRL);
	ctrl_reg &= ~XZYNQ_PRIVATE_TIMER_CTRL_EN;
	out32(XZYNQ_PRIVATE_TIMER_BASEADDR + XZYNQ_PRIVATE_TIMER_COUNTER, ctrl_reg);

	/* Set the Counter value */
	out32(XZYNQ_PRIVATE_TIMER_BASEADDR + XZYNQ_PRIVATE_TIMER_COUNTER,
			(ms / 1000) * TIMER_HZ);

	/* Start the timer */
	ctrl_reg = in32(XZYNQ_PRIVATE_TIMER_BASEADDR + XZYNQ_PRIVATE_TIMER_CTRL);
	ctrl_reg = XZYNQ_PRIVATE_TIMER_CTRL_EN;
	out32(XZYNQ_PRIVATE_TIMER_BASEADDR + XZYNQ_PRIVATE_TIMER_CTRL, ctrl_reg);
}

unsigned long timer_remaining()
{
	return in32(XZYNQ_PRIVATE_TIMER_BASEADDR + XZYNQ_PRIVATE_TIMER_COUNTER);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
