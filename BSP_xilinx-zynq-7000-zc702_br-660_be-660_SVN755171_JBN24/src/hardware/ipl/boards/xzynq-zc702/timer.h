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

#ifndef _IPL_TIMER_H
#define _IPL_TIMER_H

#define XZYNQ_PRIVATE_TIMER_BASEADDR	0xF8F00600
#define XZYNQ_PRIVATE_TIMER_SIZE		0x10

/* Timer Load register */
#define XZYNQ_PRIVATE_TIMER_LOAD		0x00

/* Timer Counter register */
#define XZYNQ_PRIVATE_TIMER_COUNTER		0x04

/* Timer Control register */
#define XZYNQ_PRIVATE_TIMER_CTRL				0x08
#define XZYNQ_PRIVATE_TIMER_CTRL_EN				(1 << 0)
#define XZYNQ_PRIVATE_TIMER_CTRL_AUTO_RELOAD	(1 << 1)
#define XZYNQ_PRIVATE_TIMER_CTRL_IRQ			(1 << 2)
#define XZYNQ_PRIVATE_TIMER_CTRL_PRESCALER_MSK	(0xFF << 8)

/* Timer Interrupt Status register */
#define XZYNQ_PRIVATE_TIMER_INT_STS 	0x0C

void msleep(unsigned long value);
void timer_set(unsigned long ms);
unsigned long timer_remaining();

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
