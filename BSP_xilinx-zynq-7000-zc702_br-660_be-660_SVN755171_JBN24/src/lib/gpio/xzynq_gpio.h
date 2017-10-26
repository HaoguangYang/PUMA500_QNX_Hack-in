/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
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

#ifndef XZYNQ_GPIO_H
#define XZYNQ_GPIO_H

#include <hw/gpio.h>
#include <arm/xzynq.h>

#define XZYNQ_NUM_GPIO_BANKS 4

#define XZYNQ_GPIO_BASE 0xE000A000

/** @name Register offsets for the GPIO. Each register is 32 bits.
 *  @{
 */
#define XZYNQ_GPIOPS_DATA_LSW_OFFSET  0x000  /* Mask and Data Register LSW, WO */
#define XZYNQ_GPIOPS_DATA_MSW_OFFSET  0x004  /* Mask and Data Register MSW, WO */
#define XZYNQ_GPIOPS_DATA_OFFSET	 0x040  /* Data Register, RW */
#define XZYNQ_GPIOPS_DIRM_OFFSET	 0x204  /* Direction Mode Register, RW */
#define XZYNQ_GPIOPS_OUTEN_OFFSET	 0x208  /* Output Enable Register, RW */
#define XZYNQ_GPIOPS_INTMASK_OFFSET	 0x20C  /* Interrupt Mask Register, RO */
#define XZYNQ_GPIOPS_INTEN_OFFSET	 0x210  /* Interrupt Enable Register, WO */
#define XZYNQ_GPIOPS_INTDIS_OFFSET	 0x214  /* Interrupt Disable Register, WO*/
#define XZYNQ_GPIOPS_INTSTS_OFFSET	 0x218  /* Interrupt Status Register, RO */
#define XZYNQ_GPIOPS_INTTYPE_OFFSET	 0x21C  /* Interrupt Type Register, RW */
#define XZYNQ_GPIOPS_INTPOL_OFFSET	 0x220  /* Interrupt Polarity Register, RW */
#define XZYNQ_GPIOPS_INTANY_OFFSET	 0x224  /* Interrupt On Any Register, RW */
/* @} */

#define XZYNQ_GPIOPS_IRQ_TYPE_EDGE_RISING	0  /**< Interrupt on Rising edge */
#define XZYNQ_GPIOPS_IRQ_TYPE_EDGE_FALLING	1  /**< Interrupt Falling edge */
#define XZYNQ_GPIOPS_IRQ_TYPE_EDGE_BOTH	2  /**< Interrupt on both edges */
#define XZYNQ_GPIOPS_IRQ_TYPE_LEVEL_HIGH	3  /**< Interrupt on high level */
#define XZYNQ_GPIOPS_IRQ_TYPE_LEVEL_LOW	4  /**< Interrupt on low level */

/** @name Register offsets for each Bank.
 *  @{
 */
#define XZYNQ_GPIOPS_DATA_MASK_OFFSET 0x8  /* Data/Mask Registers offset */
#define XZYNQ_GPIOPS_DATA_BANK_OFFSET 0x4  /* Data Registers offset */
#define XZYNQ_GPIOPS_REG_MASK_OFFSET 0x40  /* Registers offset */
/* @} */

#define XZYNQ_MIO_OFFSET	0x700

#define XZYNQ_MIO_TRI_ENABLE_SHIFT 	0
#define XZYNQ_MIO_L0_SEL_SHIFT		1
#define XZYNQ_MIO_L1_SEL_SHIFT 		2
#define XZYNQ_MIO_L2_SEL_SHIFT 		3
#define XZYNQ_MIO_L3_SEL_SHIFT 		5
#define XZYNQ_MIO_SPEED_SHIFT 		8
#define XZYNQ_MIO_IO_TYPE_SHIFT		9
#define XZYNQ_MIO_PULLUP_SHIFT 		12
#define XZYNQ_MIO_DIS_RCVR_SHIFT 	13

#define XZYNQ_MIO_TRI_ENABLE_MASK 	0x00000001
#define XZYNQ_MIO_L0_SEL_MASK 		0x00000002
#define XZYNQ_MIO_L1_SEL_MASK 		0x00000004
#define XZYNQ_MIO_L2_SEL_MASK 		0x00000018
#define XZYNQ_MIO_L3_SEL_MASK 		0x000000E0
#define XZYNQ_MIO_SPEED_MASK 		0x00000100
#define XZYNQ_MIO_IO_TYPE_MASK 		0x00000E00
#define XZYNQ_MIO_PULLUP_MASK 		0x00001000
#define XZYNQ_MIO_DIS_RCVR_MASK 	0x00002000

int xzynq_get_mux_config(int pin);
int xzynq_set_mux_tri_enable(int pin, int value);
int xzynq_set_mux_l0_sel(int pin, int value);
int xzynq_set_mux_l1_sel(int pin, int value);
int xzynq_set_mux_l2_sel(int pin, int value);
int xzynq_set_mux_l3_sel(int pin, int value);
int xzynq_set_mux_speed(int pin, int value);
int xzynq_set_mux_io_type(int pin, int value);
int xzynq_set_mux_pullup(int pin, int value);
int xzynq_set_mux_dis_rcvr(int pin, int value);

int xzynq_gpio_set_direction(int pin, int dir);
int xzynq_gpio_get_direction(int pin);
int xzynq_gpio_set_output_enable(int pin, int value);
int xzynq_gpio_get_output_enable(int pin);
int xzynq_gpio_get_input(int pin);
int xzynq_gpio_set_output(int pin, int data);
int xzynq_gpio_get_irq_type(int pin);
int xzynq_gpio_set_irq_type(int pin, int irq_type);
int xzynq_gpio_irq_clear(int pin);
int xzynq_gpio_get_irq_status(int pin);
int xzynq_gpio_get_irq_enable(int pin);
int xzynq_gpio_irq_enable(int pin);
int xzynq_gpio_irq_disable(int pin);

int xzynq_gpio_init(void);
void xzynq_gpio_fini(void);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/gpio/xzynq_gpio.h $ $Rev: 752035 $")
#endif
