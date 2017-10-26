/*
 * $QNXLicenseC: 
 * Copyright 2011, 2012 QNX Software Systems.  
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
#include "board.h"

#define XZYNQ_GPIO_BASE 			0xE000A000
#define XZYNQ_GPIOPS_DATA_OFFSET	0x040  /* Data Register, RW */
#define XZYNQ_GPIOPS_DIRM_OFFSET	0x204  /* Direction Mode Register, RW */
#define XZYNQ_GPIOPS_OUTEN_OFFSET	0x208  /* Output Enable Register, RW */

#define XZYNQ_GPIOPS_MIO7			(1<<7)

void zc702_init_usb(void)
{
	uint32_t gpio_base;

	gpio_base = startup_io_map(XZYNQ_GPIO_SIZE, XZYNQ_GPIO_BASE);

	/* Making the MIO is set properly */
	out32(gpio_base + XZYNQ_GPIOPS_DIRM_OFFSET, XZYNQ_GPIOPS_MIO7);
	out32(gpio_base + XZYNQ_GPIOPS_OUTEN_OFFSET, XZYNQ_GPIOPS_MIO7);

	/* Reset USB ULPI (MIO7) */
	out32(gpio_base + XZYNQ_GPIOPS_DATA_OFFSET, XZYNQ_GPIOPS_MIO7);
	out32(gpio_base + XZYNQ_GPIOPS_DATA_OFFSET, ~XZYNQ_GPIOPS_MIO7);
	out32(gpio_base + XZYNQ_GPIOPS_DATA_OFFSET, XZYNQ_GPIOPS_MIO7);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
