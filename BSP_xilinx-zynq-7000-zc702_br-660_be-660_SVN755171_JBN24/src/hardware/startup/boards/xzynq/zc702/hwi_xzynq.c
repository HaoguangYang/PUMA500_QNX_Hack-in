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


#include "startup.h"
#include "hwinfo_private.h"
#include <drvr/hwinfo.h>                // for hwi support routines in libdrvr
#include <arm/xzynq.h>

/*
 * Add Zynq devices to the hardware info section of the syspage.
*/

void hwi_xzynq()
{
	unsigned hwi_bus_internal = 0;

	/* add  UART */
	{
		unsigned hwi_off;
		hwiattr_uart_t uart_attr = HWIATTR_UART_T_INITIALIZER;
		/* All the UARTs operate from a fixed functional clock of 50 MHz only */
		struct hwi_inputclk clksrc = {.clk = 50000000, .div = 16};

		/* each UART has an interrupt */
		HWIATTR_UART_SET_NUM_IRQ(&uart_attr, 1);
		HWIATTR_UART_SET_NUM_CLK(&uart_attr, 1);

		/* create uart0 */
		HWIATTR_UART_SET_LOCATION(&uart_attr, XZYNQ_XUARTPS_UART1_BASE, XZYNQ_XUARTPS_UART_SIZE, 0, hwi_find_as(XZYNQ_XUARTPS_UART1_BASE, 1));
		hwi_off = hwidev_add_uart(XZYNQ_HWI_UART, &uart_attr, hwi_bus_internal);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_ivec(hwi_off, 0, XZYNQ_IRQ_UART1);
		hwitag_set_inputclk(hwi_off, 0, &clksrc);
	}

	/* add the WATCHDOG device */
	{
		unsigned hwi_off;
		hwiattr_timer_t attr = HWIATTR_TIMER_T_INITIALIZER;
		const struct hwi_inputclk clksrc_kick = {.clk = 5000, .div = 1};
		HWIATTR_TIMER_SET_NUM_CLK(&attr, 1);
		HWIATTR_TIMER_SET_LOCATION(&attr, XZYNQ_WDT_BASEADDR, XZYNQ_WDT_SIZE, 0, hwi_find_as(XZYNQ_WDT_BASEADDR, 1));
		hwi_off = hwidev_add_timer(XZYNQ_HWI_WDT, &attr,  HWI_NULL_OFF);
		ASSERT(hwi_off != HWI_NULL_OFF);
		hwitag_set_inputclk(hwi_off, 0, (struct hwi_inputclk *)&clksrc_kick);
	}

	/* add DMA controller */
	{
		unsigned hwi_off;
		hwiattr_dma_t attr = HWIATTR_DMA_T_INITIALIZER;
		HWIATTR_DMA_SET_NUM_IRQ(&attr, 1);

		/* create DMA controller */
		HWIATTR_USB_SET_LOCATION(&attr, XZYNQ_DMA_S_BASE_ADDR, XZYNQ_DMA_REG_SIZE, 0, hwi_find_as(XZYNQ_DMA_NS_BASE_ADDR, 1));
		hwi_off = hwidev_add_dma(XZYNQ_HWI_DMA, &attr, hwi_bus_internal);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_avail_ivec(hwi_off, 0, XZYNQ_IRQ_DMA_ABORT);
	}

	/* add RTC */
	hwi_add_device(HWI_ITEM_BUS_UNKNOWN, HWI_ITEM_DEVCLASS_RTC, "NONE", 0);

	/* TODO Add peripherals */
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
