/*
 * $QNXLicenseC:
 * Copyright 2007, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable license fees to QNX
 * Software Systems before you may reproduce, modify or distribute this software,
 * or any work that includes all or part of this software.   Free development
 * licenses are available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review this entire
 * file for other proprietary rights or license notices, as well as the QNX
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/
 * for other information.
 * $
 */

#ifndef _CLK_LIB_H_INCLUDED
#define _CLK_LIB_H_INCLUDED

#include <stdint.h>

typedef enum clk_id {
	XZYNQ_CPU_CLK,
	XZYNQ_ARM_PLL_CLK,
	XZYNQ_DDR_PLL_CLK,
	XZYNQ_IO_PLL_CLK,
	XZYNQ_CPU_1x_CLK,
	XZYNQ_CPU_2x_CLK,
	XZYNQ_CPU_3x2x_CLK,
	XZYNQ_CPU_6x4x_CLK,
	XZYNQ_GEM0_CLK,
	XZYNQ_GEM1_CLK,
	XZYNQ_GEM0_RCLK,
	XZYNQ_GEM1_RCLK,
	XZYNQ_SDIO_CLK,
	XZYNQ_I2C_CLK,
	XZYNQ_UART_CLK,
	XZYNQ_CAN_CLK,
	XZYNQ_QSPI_CLK,
	XZYNQ_SPI_CLK,
} clk_id_t;

typedef enum ctrl_id {
	XZYNQ_CPU_CTRL,
	XZYNQ_ARM_PLL_CTRL,
	XZYNQ_DDR_PLL_CTRL,
	XZYNQ_IO_PLL_CTRL,
	XZYNQ_CPU_1x_CTRL,
	XZYNQ_CPU_2x_CTRL,
	XZYNQ_CPU_3x2x_CTRL,
	XZYNQ_CPU_6x4x_CTRL,
	XZYNQ_GEM0_CTRL,
	XZYNQ_GEM1_CTRL,
	XZYNQ_GEM0_RCTRL,
	XZYNQ_GEM1_RCTRL,
	XZYNQ_SDIO0_CTRL,
	XZYNQ_SDIO1_CTRL,
	XZYNQ_I2C0_CTRL,
	XZYNQ_I2C1_CTRL,
	XZYNQ_UART0_CTRL,
	XZYNQ_UART1_CTRL,
	XZYNQ_CAN0_CTRL,
	XZYNQ_CAN1_CTRL,
	XZYNQ_QSPI_CTRL,
	XZYNQ_SPI0_CTRL,
	XZYNQ_SPI1_CTRL
} ctrl_id_t;

typedef struct _clk_freq {
	clk_id_t id;
	uint32_t freq;
} clk_freq_t;

/*
 * The following devctls are used by a client application
 * to control the Clock interface.
 */
#include <devctl.h>

#define _DCMD_CLOCK   _DCMD_MISC

#define DCMD_CLOCK_SET_FREQ				__DIOT(_DCMD_CLOCK, 0, clk_freq_t)
#define DCMD_CLOCK_GET_FREQ				__DIOTF(_DCMD_CLOCK, 1, clk_freq_t)
#define DCMD_CLOCK_ENABLE				__DIOTF(_DCMD_CLOCK, 2, ctrl_id_t)
#define DCMD_CLOCK_DISABLE				__DIOTF(_DCMD_CLOCK, 3, ctrl_id_t)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/clock/public/hw/clk.h $ $Rev: 752035 $")
#endif
