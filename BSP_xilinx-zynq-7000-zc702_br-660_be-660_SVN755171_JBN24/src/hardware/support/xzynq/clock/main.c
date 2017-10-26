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

static clk_dev_t *dev;

int main(int argc, char *argv[])
{
	int id;
	resmgr_connect_funcs_t connect_funcs;
	resmgr_io_funcs_t io_funcs;
	dispatch_t *dpp;
	resmgr_attr_t rattr;
	dispatch_context_t *ctp;
	iofunc_attr_t ioattr;

	/* Initialize the dispatch interface */
	dpp = dispatch_create();
	if (!dpp) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"Clock error: Failed to create dispatch interface\n");
		goto fail;
	}

	/* Initialize the resource manager attributes */
	memset(&rattr, 0, sizeof(rattr));

	/* Initialize the connect functions */
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
			_RESMGR_IO_NFUNCS, &io_funcs);
	io_funcs.devctl = clk_io_devctl;
	iofunc_attr_init(&ioattr, S_IFCHR | 0666, NULL, NULL);

	/* Attach the device name */
	id = resmgr_attach(dpp, &rattr, "/dev/clock", _FTYPE_ANY, 0,
			&connect_funcs, &io_funcs, &ioattr);
	if (id == -1) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"Clock error: Failed to attach pathname\n");
		goto fail;
	}

	/* Allocate a context structure */
	ctp = dispatch_context_alloc(dpp);

	dev = malloc(sizeof(clk_dev_t));
	if (!dev) {
		goto fail;
	}

	/*
	 * mmap SLCR for clock configuration
	 */
	dev->reglen = XZYNQ_SLCR_LEN;
	dev->physbase = XZYNQ_SLCR_BASE;

	dev->regbase = mmap_device_io(dev->reglen, dev->physbase);
	if (dev->regbase == MAP_DEVICE_FAILED) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"Failed to map Clock registers\n");
		goto fail;
	}

	/* Run in the background */
	if (procmgr_daemon(EXIT_SUCCESS, PROCMGR_DAEMON_NOCLOSE
			| PROCMGR_DAEMON_NODEVNULL ) == -1) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "%s:  procmgr_daemon",
				argv[0]);
		goto fail;
	}

	while (1) {
		if ((ctp = dispatch_block(ctp)) == NULL) {
			slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
					"Clock error: Block error\n");
			goto fail;
		}
		dispatch_handler(ctp);
	}

fail:
	free(dev);

	return EXIT_SUCCESS;
}

int clk_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
	int status, nbytes;
	clk_freq_t *clk;
	ctrl_id_t ctrl_id;

	status = iofunc_devctl_default(ctp, msg, ocb);
	if (status != _RESMGR_DEFAULT) {
		return status;
	}

	nbytes = 0;

	switch (msg->i.dcmd) {
	case DCMD_CLOCK_ENABLE:
		ctrl_id = *(ctrl_id_t*)_DEVCTL_DATA(msg->i);

		switch (ctrl_id) {
		case XZYNQ_GEM0_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_GEM0_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_GEM0_OFFSET, CLK_ENABLE);
			break;
		case XZYNQ_GEM1_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_GEM1_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_GEM1_OFFSET, CLK_ENABLE);
			break;
		case XZYNQ_GEM0_RCTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_GEM0_RCLK_CTRL_REG,
					XZYNQ_SLCR_RCLK_CTRL_CLKACT_GEM0_OFFSET, CLK_ENABLE);
			break;
		case XZYNQ_GEM1_RCTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_GEM1_RCLK_CTRL_REG,
					XZYNQ_SLCR_RCLK_CTRL_CLKACT_GEM1_OFFSET, CLK_ENABLE);
			break;
		case XZYNQ_CAN0_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_CAN_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_CAN0_OFFSET, CLK_ENABLE);
			break;
		case XZYNQ_CAN1_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_CAN_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_CAN1_OFFSET, CLK_ENABLE);
			break;
		case XZYNQ_QSPI_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_LQSPI_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_LQSPI_OFFSET, CLK_ENABLE);
			break;
		case XZYNQ_SPI0_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_SPI_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_SPI0_OFFSET, CLK_ENABLE);
			break;
		case XZYNQ_SPI1_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_SPI_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_SPI1_OFFSET, CLK_ENABLE);
			break;
		default:
			/* Not supported */
			break;
		}

		break;

	case DCMD_CLOCK_DISABLE:
		ctrl_id = *(ctrl_id_t*)_DEVCTL_DATA(msg->i);

		switch (ctrl_id) {
		case XZYNQ_GEM0_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_GEM0_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_GEM0_OFFSET, CLK_DISABLE);
			break;
		case XZYNQ_GEM1_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_GEM1_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_GEM1_OFFSET, CLK_DISABLE);
			break;
		case XZYNQ_GEM0_RCTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_GEM0_RCLK_CTRL_REG,
					XZYNQ_SLCR_RCLK_CTRL_CLKACT_GEM0_OFFSET, CLK_DISABLE);
			break;
		case XZYNQ_GEM1_RCTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_GEM1_RCLK_CTRL_REG,
					XZYNQ_SLCR_RCLK_CTRL_CLKACT_GEM1_OFFSET, CLK_DISABLE);
			break;
		case XZYNQ_CAN0_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_CAN_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_CAN0_OFFSET, CLK_DISABLE);
			break;
		case XZYNQ_CAN1_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_CAN_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_CAN1_OFFSET, CLK_DISABLE);
			break;
		case XZYNQ_QSPI_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_LQSPI_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_LQSPI_OFFSET, CLK_DISABLE);
			break;
		case XZYNQ_SPI0_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_SPI_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_SPI0_OFFSET, CLK_DISABLE);
			break;
		case XZYNQ_SPI1_CTRL:
			xzynq_clk_control(dev, XZYNQ_SLCR_SPI_CLK_CTRL_REG,
					XZYNQ_SLCR_CLK_CTRL_CLKACT_SPI1_OFFSET, CLK_DISABLE);
			break;
		default:
			/* Not supported */
			break;
		}

		break;

	case DCMD_CLOCK_SET_FREQ:
		clk = _DEVCTL_DATA(msg->i);
		switch (clk->id) {
			case XZYNQ_CAN_CLK:
				xzynq_set_can_clk(dev, clk->freq);
				break;
			default:
				/* Not supported */
				break;
		}
		break;

	case DCMD_CLOCK_GET_FREQ:
		clk = _DEVCTL_DATA(msg->i);

		/* Get the frequency by ID */
		switch (clk->id) {
		case XZYNQ_CPU_CLK:
			clk->freq = xzynq_get_cpu_clk(dev);
			break;
		case XZYNQ_ARM_PLL_CLK:
			clk->freq = xzynq_get_arm_pll_clk(dev);
			break;
		case XZYNQ_DDR_PLL_CLK:
			clk->freq = xzynq_get_ddr_pll_clk(dev);
			break;
		case XZYNQ_IO_PLL_CLK:
			clk->freq = xzynq_get_io_pll_clk(dev);
			break;
		case XZYNQ_CPU_1x_CLK:
			clk->freq = xzynq_get_cpu_1x_clk(dev);
			break;
		case XZYNQ_CPU_2x_CLK:
			clk->freq = xzynq_get_cpu_1x_clk(dev) * 2;
			break;
		case XZYNQ_CPU_3x2x_CLK:
			if (xzynq_clk_621_true(dev))
				clk->freq = xzynq_get_cpu_1x_clk(dev) * 3;
			else
				clk->freq = xzynq_get_cpu_1x_clk(dev) * 2;
			break;
		case XZYNQ_CPU_6x4x_CLK:
			if (xzynq_clk_621_true(dev))
				clk->freq = xzynq_get_cpu_1x_clk(dev) * 6;
			else
				clk->freq = xzynq_get_cpu_1x_clk(dev) * 4;
			break;
		case XZYNQ_I2C_CLK:
			clk->freq = xzynq_get_io_pll_clk(dev);
			break;
		case XZYNQ_CAN_CLK:
			clk->freq = xzynq_get_can_clk(dev);
			break;
		/*
		 * By default, we suppose this is a peripheral clock, if it
		 * doesn't, the function will return -1
		 */
		default:
			clk->freq = xzynq_get_peripheral_clk(dev, clk->id);
		}

		nbytes = sizeof(clk_freq_t);
		break;
	}

	if (nbytes == 0) {
		return (EOK);
	} else {
		msg->o.ret_val = 0;
		msg->o.nbytes = nbytes;
		return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + nbytes));
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/clock/main.c $ $Rev: 752035 $")
#endif
