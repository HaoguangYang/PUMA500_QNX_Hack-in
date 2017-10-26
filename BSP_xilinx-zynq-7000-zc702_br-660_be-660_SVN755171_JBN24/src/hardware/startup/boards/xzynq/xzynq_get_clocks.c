/*
 * $QNXLicenseC: 
 * Copyright 2012, QNX Software Systems.  
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
#include <arm/xzynq.h>

#define	ONE_MHZ					1000000

/* PS clock frequency (cannot be changed on the ZC702) */
/* FIXME: move to board-specific declaration */
#define XZYNQ_CLOCK_PS_FREQ		33333333

static uint32_t xzynq_get_ps_clk(void)
{
	return XZYNQ_CLOCK_PS_FREQ;
}

static uint32_t xzynq_get_arm_pll_clk(void)
{
	uint32_t clk_ctrl;
	uint32_t feedback_divider;
	uint32_t ps_clock_freq;

	ps_clock_freq = xzynq_get_ps_clk();

	clk_ctrl = in32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_ARM_PLL_CTRL_REG);

	feedback_divider = clk_ctrl & XZYNQ_SLCR_ARM_PLL_CTRL_PLL_FDIV_MASK;
	feedback_divider = feedback_divider
			>> XZYNQ_SLCR_ARM_PLL_CTRL_PLL_FDIV_OFFSET;

	/* PLL = PS * feedback divider */
	return feedback_divider * ps_clock_freq;
}

static uint32_t xzynq_get_ddr_pll_clk(void)
{
	uint32_t clk_ctrl;
	uint32_t feedback_divider;
	uint32_t ps_clock_freq;

	ps_clock_freq = xzynq_get_ps_clk();

	clk_ctrl = in32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_DDR_PLL_CTRL_REG);

	feedback_divider = clk_ctrl & XZYNQ_SLCR_DDR_PLL_CTRL_PLL_FDIV_MASK;
	feedback_divider = feedback_divider
			>> XZYNQ_SLCR_DDR_PLL_CTRL_PLL_FDIV_OFFSET;

	/* PLL = PS * feedback divider */
	return feedback_divider * ps_clock_freq;
}

static uint32_t xzynq_get_io_pll_clk(void)
{
	uint32_t clk_ctrl;
	uint32_t feedback_divider;
	uint32_t ps_clock_freq;

	ps_clock_freq = xzynq_get_ps_clk();

	clk_ctrl = in32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_IO_PLL_CTRL_REG);

	feedback_divider = clk_ctrl & XZYNQ_SLCR_IO_PLL_CTRL_PLL_FDIV_MASK;
	feedback_divider = feedback_divider
			>> XZYNQ_SLCR_IO_PLL_CTRL_PLL_FDIV_OFFSET;

	/* PLL = PS * feedback divider */
	return feedback_divider * ps_clock_freq;
}

uint32_t xzynq_get_cpu_clk(void)
{
	uint32_t pll_source_value = 0;
	uint32_t pll_ctrl_reg;
	uint32_t pll_source_reg;
	uint32_t pll_divider;
	uint32_t arm_clock;

	/* Get the PLL source */
	pll_ctrl_reg = in32(XZYNQ_SLCR_BASE + XZYNQ_SLCR_ARM_CLK_CTRL_REG);
	pll_source_reg = pll_ctrl_reg & XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_MASK;

	switch (pll_source_reg) {
	case XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_CPU_PLL1:
	case XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_CPU_PLL2:
		pll_source_value = xzynq_get_arm_pll_clk();
		break;

	case XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_DDR_PLL:
		pll_source_value = xzynq_get_ddr_pll_clk();
		break;

	case XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_IO_PLL:
		pll_source_value = xzynq_get_io_pll_clk();
		break;
	}

	/* Get the PLL divider */
	pll_divider = pll_ctrl_reg & XZYNQ_SLCR_ARM_CLK_CTRL_DIVISOR_MASK;
	pll_divider = pll_divider >> XZYNQ_SLCR_ARM_CLK_CTRL_DIVISOR_OFFSET;

	arm_clock = pll_source_value / pll_divider;

	if (debug_flag) {
		kprintf("pll source = %d value=%d\n", pll_source_reg >> 4,
				pll_source_value);
		kprintf("pll divider = %d\n", pll_divider);
		kprintf("arm_clock = %d\n", arm_clock);
	}

	return arm_clock;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/boards/xzynq/xzynq_get_clocks.c $ $Rev: 752035 $")
#endif
