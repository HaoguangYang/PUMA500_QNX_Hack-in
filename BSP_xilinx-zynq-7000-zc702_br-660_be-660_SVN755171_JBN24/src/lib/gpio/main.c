/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include "xzynq_gpio.h"
#include <stdint.h>

int get_gpiofuncs(gpio_functions_t *functable, int tabsize)
{
    GPIO_ADD_FUNC(functable, init, xzynq_gpio_init, tabsize);
    GPIO_ADD_FUNC(functable, fini, xzynq_gpio_fini, tabsize);

    GPIO_ADD_FUNC(functable, gpio_set_direction, xzynq_gpio_set_direction, tabsize);
    GPIO_ADD_FUNC(functable, gpio_get_direction, xzynq_gpio_get_direction, tabsize);
    GPIO_ADD_FUNC(functable, gpio_set_output_enable, xzynq_gpio_set_output_enable, tabsize);
    GPIO_ADD_FUNC(functable, gpio_get_output_enable, xzynq_gpio_get_output_enable, tabsize);
    GPIO_ADD_FUNC(functable, gpio_get_input, xzynq_gpio_get_input, tabsize);
    GPIO_ADD_FUNC(functable, gpio_set_output, xzynq_gpio_set_output, tabsize);
    GPIO_ADD_FUNC(functable, gpio_get_irq_type, xzynq_gpio_get_irq_type, tabsize);
    GPIO_ADD_FUNC(functable, gpio_set_irq_type, xzynq_gpio_set_irq_type, tabsize);
    GPIO_ADD_FUNC(functable, gpio_irq_clear, xzynq_gpio_irq_clear, tabsize);
    GPIO_ADD_FUNC(functable, gpio_get_irq_status, xzynq_gpio_get_irq_status, tabsize);
    GPIO_ADD_FUNC(functable, gpio_get_irq_enable, xzynq_gpio_get_irq_enable, tabsize);
    GPIO_ADD_FUNC(functable, gpio_irq_enable, xzynq_gpio_irq_enable, tabsize);
    GPIO_ADD_FUNC(functable, gpio_irq_disable, xzynq_gpio_irq_disable, tabsize);

    GPIO_ADD_FUNC(functable, mio_get_mux_config, xzynq_get_mux_config, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_tri_enable, xzynq_set_mux_tri_enable, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_l0_sel, xzynq_set_mux_l0_sel, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_l1_sel, xzynq_set_mux_l1_sel, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_l2_sel, xzynq_set_mux_l2_sel, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_l3_sel, xzynq_set_mux_l3_sel, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_speed, xzynq_set_mux_speed, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_io_type, xzynq_set_mux_io_type, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_pullup, xzynq_set_mux_pullup, tabsize);
    GPIO_ADD_FUNC(functable, mio_set_mux_dis_rcvr, xzynq_set_mux_dis_rcvr, tabsize);
    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/gpio/main.c $ $Rev: 752035 $")
#endif
