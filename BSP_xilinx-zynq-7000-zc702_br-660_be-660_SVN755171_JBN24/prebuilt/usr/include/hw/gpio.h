/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, QNX Software Systems.  
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

#ifndef _HW_GPIO_DRIVER_H_INCLUDED
#define _HW_GPIO_DRIVER_H_INCLUDED
#ifndef __PLATFORM_H_INCLUDED
#include <sys/platform.h>
#endif
#ifndef __NEUTRINO_H_INCLUDED
#include <sys/neutrino.h>
#endif

typedef struct _gpio_functions {
	int (*init)(void);
	void (*fini)(void);

	int (*gpio_set_direction)(int pin, int dir);
	int (*gpio_get_direction)(int pin);
	int (*gpio_set_output_enable)(int pin, int value);
	int (*gpio_get_output_enable)(int pin);
	int (*gpio_get_input)(int pin);
	int (*gpio_set_output)(int pin, int data);
	int (*gpio_get_irq_type)(int pin);
	int (*gpio_set_irq_type)(int pin, int irq_type);
	int (*gpio_irq_clear)(int pin);
	int (*gpio_get_irq_status)(int pin);
	int (*gpio_get_irq_enable)(int pin);
	int (*gpio_irq_enable)(int pin);
	int (*gpio_irq_disable)(int pin);

	int (*mio_get_mux_config)(int pin);
	int (*mio_set_mux_tri_enable)(int pin, int value);
	int (*mio_set_mux_l0_sel)(int pin, int value);
	int (*mio_set_mux_l1_sel)(int pin, int value);
	int (*mio_set_mux_l2_sel)(int pin, int value);
	int (*mio_set_mux_l3_sel)(int pin, int value);
	int (*mio_set_mux_speed)(int pin, int value);
	int (*mio_set_mux_io_type)(int pin, int value);
	int (*mio_set_mux_pullup)(int pin, int value);
	int (*mio_set_mux_dis_rcvr)(int pin, int value);
} gpio_functions_t;

/* Macro used by H/W driver when populating gpio_functions table */
#define GPIO_ADD_FUNC(table, entry, func, limit) \
	if (((size_t)&(((gpio_functions_t *)0)->entry)) + \
	    sizeof (void (*)()) <= (limit)) \
		(table)->entry = (func);

typedef int (*get_gpiofuncs_t)(gpio_functions_t *funcs, int tabsize);

int get_gpiofuncs(gpio_functions_t *funcs, int tabsize);

#endif /* _HW_GPIO_H_INCLUDED */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/gpio/public/hw/gpio.h $ $Rev: 752035 $")
#endif
