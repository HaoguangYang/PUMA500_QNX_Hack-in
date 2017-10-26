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

#include <proto.h>

uint32_t xzynq_get_arm_pll_clk(clk_dev_t *dev)
{
	uint32_t clk_ctrl;
	uint32_t feedback_divider;

	clk_ctrl = in32(dev->regbase + XZYNQ_SLCR_ARM_PLL_CTRL_REG);

	feedback_divider = clk_ctrl & XZYNQ_SLCR_ARM_PLL_CTRL_PLL_FDIV_MASK;
	feedback_divider = feedback_divider
			>> XZYNQ_SLCR_ARM_PLL_CTRL_PLL_FDIV_OFFSET;

	/* PLL = PS * feedback divider */
	return feedback_divider * XZYNQ_PS_CLOCK;
}

uint32_t xzynq_get_ddr_pll_clk(clk_dev_t *dev)
{
	uint32_t clk_ctrl;
	uint32_t feedback_divider;

	clk_ctrl = in32(dev->regbase + XZYNQ_SLCR_DDR_PLL_CTRL_REG);

	feedback_divider = clk_ctrl & XZYNQ_SLCR_DDR_PLL_CTRL_PLL_FDIV_MASK;
	feedback_divider = feedback_divider
			>> XZYNQ_SLCR_DDR_PLL_CTRL_PLL_FDIV_OFFSET;

	/* PLL = PS * feedback divider */
	return feedback_divider * XZYNQ_PS_CLOCK;
}

uint32_t xzynq_get_io_pll_clk(clk_dev_t *dev)
{
	uint32_t clk_ctrl;
	uint32_t feedback_divider;

	clk_ctrl = in32(dev->regbase + XZYNQ_SLCR_IO_PLL_CTRL_REG);

	feedback_divider = clk_ctrl & XZYNQ_SLCR_IO_PLL_CTRL_PLL_FDIV_MASK;
	feedback_divider = feedback_divider
			>> XZYNQ_SLCR_IO_PLL_CTRL_PLL_FDIV_OFFSET;

	/* PLL = PS * feedback divider */
	return feedback_divider * XZYNQ_PS_CLOCK;
}

uint32_t xzynq_get_cpu_clk(clk_dev_t *dev)
{
	uint32_t pll_source_value = 0;
	uint32_t pll_ctrl_reg;
	uint32_t pll_source_reg;
	uint32_t pll_divider;
	uint32_t arm_clock;

	/* Get the PLL source */
	pll_ctrl_reg = in32(dev->regbase + XZYNQ_SLCR_ARM_CLK_CTRL_REG);
	pll_source_reg = pll_ctrl_reg & XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_MASK;

	switch (pll_source_reg) {
	case XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_CPU_PLL1:
	case XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_CPU_PLL2:
		pll_source_value = xzynq_get_arm_pll_clk(dev);
		break;

	case XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_DDR_PLL:
		pll_source_value = xzynq_get_ddr_pll_clk(dev);
		break;

	case XZYNQ_SLCR_ARM_CLK_CTRL_SRCSEL_IO_PLL:
		pll_source_value = xzynq_get_io_pll_clk(dev);
		break;
	}

	/* Get the PLL divider */
	pll_divider = pll_ctrl_reg & XZYNQ_SLCR_ARM_CLK_CTRL_DIVISOR_MASK;
	pll_divider = pll_divider >> XZYNQ_SLCR_ARM_CLK_CTRL_DIVISOR_OFFSET;

	arm_clock = pll_source_value / pll_divider;

	return arm_clock;
}

uint32_t xzynq_clk_621_true(clk_dev_t *dev)
{
	uint32_t divider;

	/* Get the divider (4 or 6) source */
	divider = in32(dev->regbase + XZYNQ_SLCR_CLK_621_TRUE_REG);

	return (divider & 0x1);
}

uint32_t xzynq_get_cpu_1x_clk(clk_dev_t *dev)
{
	uint32_t divider;

	if (xzynq_clk_621_true(dev))
		divider = 6;
	else
		divider = 4;

	return xzynq_get_cpu_clk(dev) / divider;
}

void xzynq_clk_control(clk_dev_t *dev, uint32_t offset, uint32_t shift, int action)
{
	uint32_t clk_reg = in32(dev->regbase + offset);

	/* Ensure that SLCR is unlock */
	out32(dev->regbase + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	if (action) {
		out32(dev->regbase + offset, clk_reg | (0x1 << shift));
	} else {
		out32(dev->regbase + offset, clk_reg & ~(0x1 << shift));
	}
}

static uint32_t xzynq_get_peripheral_pll(clk_dev_t *dev, uint32_t offset)
{
	uint32_t clk_reg = in32(dev->regbase + offset);
	uint32_t source = (clk_reg & XZYNQ_SLCR_PERIPH_CLK_CTRL_SRCSEL_MASK);

	switch (source) {
	case XZYNQ_SLCR_PERIPH_CLK_CTRL_SRCSEL_ARM_PLL:
		return xzynq_get_arm_pll_clk(dev);
		break;

	case XZYNQ_SLCR_PERIPH_CLK_CTRL_SRCSEL_DDR_PLL:
		return xzynq_get_ddr_pll_clk(dev);
		break;

	default: /* IO_PLL or other */
		return xzynq_get_io_pll_clk(dev);
	}
}

static uint32_t xzynq_get_peripheral_divider(clk_dev_t *dev, uint32_t offset)
{
	uint32_t clk_reg = in32(dev->regbase + offset);
	return (clk_reg & XZYNQ_SLCR_PERIPH_CLK_CTRL_DIVISOR_MASK) >> XZYNQ_SLCR_PERIPH_CLK_CTRL_DIVISOR_OFFSET;
}

uint32_t xzynq_get_can_clk(clk_dev_t *dev)
{
	uint32_t pll_freq, divider0 = 1, divider1 = 1;
	uint32_t clk_reg = in32(dev->regbase + XZYNQ_SLCR_CAN_CLK_CTRL_REG);

	pll_freq = xzynq_get_peripheral_pll(dev, XZYNQ_SLCR_CAN_CLK_CTRL_REG);
	divider0 = (clk_reg & XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR0_MASK) >> XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR0_OFFSET;
	divider1 = (clk_reg & XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR1_MASK) >> XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR1_OFFSET;

	if (divider0 == 0 || divider1 == 0) {
		return -1;
	}

	/*
	 * FIXME need to make clock twice as slow to match CAN specs
	 */
	return (pll_freq / divider0 / divider1) * 2;
}

void xzynq_set_can_clk(clk_dev_t *dev, uint32_t freq)
{
	int done = 0;
	uint32_t pll_freq, divider0, divider1, tmp_freq;
	uint32_t clk_reg, max_div0, max_div1;

	pll_freq = xzynq_get_peripheral_pll(dev, XZYNQ_SLCR_CAN_CLK_CTRL_REG);

	/*
	 * FIXME need to make clock twice as slow to match CAN specs
	 */
	freq /= 2;

	max_div0 = XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR0_MASK >> XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR0_OFFSET;
	max_div1 = XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR1_MASK >> XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR1_OFFSET;

	/* Calculate best dividers for CAN CLK */
	for (divider1 = 1; divider1 <= max_div1 && done == 0; divider1++) {
		for (divider0 = 1; divider0 <= max_div0; divider0++) {
			tmp_freq = pll_freq / divider0 / divider1;
			if (tmp_freq <= freq) {
				done = 1;
				break;
			}
		}
	}
	divider1--;

	/* Set dividers in SLCR */
	clk_reg = in32(dev->regbase + XZYNQ_SLCR_CAN_CLK_CTRL_REG);
	clk_reg &= ~XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR0_MASK;
	clk_reg |= (divider0 << XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR0_OFFSET);
	out32(dev->regbase + XZYNQ_SLCR_CAN_CLK_CTRL_REG, clk_reg);

	clk_reg = in32(dev->regbase + XZYNQ_SLCR_CAN_CLK_CTRL_REG);
	clk_reg &= ~XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR1_MASK;
	clk_reg |= (divider1 << XZYNQ_SLCR_CAN_CLK_CTRL_DIVISOR1_OFFSET);
	out32(dev->regbase + XZYNQ_SLCR_CAN_CLK_CTRL_REG, clk_reg);

	/* Force to PLL internal */
	out32(dev->regbase + XZYNQ_SLCR_CAN_MIOCLK_CTRL_REG, 0);
}

uint32_t xzynq_get_peripheral_clk(clk_dev_t *dev, clk_id_t id)
{
	uint32_t pll_freq, divider = 1;

	/* Get the current pll freq and divider for the Clock ID */
	switch (id) {
	case XZYNQ_QSPI_CLK:
		pll_freq = xzynq_get_peripheral_pll(dev, XZYNQ_SLCR_LQSPI_CLK_CTRL_REG);
		divider = xzynq_get_peripheral_divider(dev, XZYNQ_SLCR_LQSPI_CLK_CTRL_REG);
		break;

	case XZYNQ_SDIO_CLK:
		pll_freq = xzynq_get_peripheral_pll(dev, XZYNQ_SLCR_SDIO_CLK_CTRL_REG);
		divider = xzynq_get_peripheral_divider(dev, XZYNQ_SLCR_SDIO_CLK_CTRL_REG);
		break;

	case XZYNQ_UART_CLK:
		pll_freq = xzynq_get_peripheral_pll(dev, XZYNQ_SLCR_UART_CLK_CTRL_REG);
		divider = xzynq_get_peripheral_divider(dev, XZYNQ_SLCR_UART_CLK_CTRL_REG);
		break;

	case XZYNQ_SPI_CLK:
		pll_freq = xzynq_get_peripheral_pll(dev, XZYNQ_SLCR_SPI_CLK_CTRL_REG);
		divider = xzynq_get_peripheral_divider(dev, XZYNQ_SLCR_SPI_CLK_CTRL_REG);
		break;

	default:
		return -1;
	}

	/* That should never happen ... */
	if (divider == 0) {
		return -1;
	}

	return pll_freq / divider;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/clock/lib.c $ $Rev: 752035 $")
#endif
