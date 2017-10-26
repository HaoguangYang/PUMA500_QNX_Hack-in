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

#include "xzynq_spi.h"

int xzynq_cfg(void *hdl, spi_cfg_t *cfg, int cs)
{
	xzynq_spi_t *dev = hdl;
	uint32_t ctrl, delay;
	uint32_t calculated_speed, i = 0;

	if ((cfg == NULL) || (cs >= NUM_OF_SPI_DEVS))
		return -1;

	/* Manual start disabled
	 * Only 1 of 3 selects
	 * Use of SPI reference clock (166MHz)
	 */
	ctrl = 0;

	/* Delays settings from SS to data */
	delay = dev->cs_delay & XZYNQ_SPI_DLY_INIT_MASK;

	/* Select CS control */
	if (dev->gpiocs[cs])
		ctrl |= XZYNQ_SPI_CR_MAN_CS;
	else
		ctrl |= (cs << XZYNQ_SPI_CR_CS_OFFSET);

	/* FIFO width */
	i = cfg->mode & SPI_MODE_CHAR_LEN_MASK;
	if ((i!=8) && (i!=16) && (i!=24) && (i!=32))
		return -1;
	i >>= 3; // divide by 8
	i -= 1;
	ctrl = (i << XZYNQ_SPI_FIFO_WIDTH_OFFSET);

	/* Calculate the SPI target operational speed.
	 * The SPI module is supplied with a 166MHz reference clock.
	 * The SPI transfer speed has to be set by dividing this
	 * reference clock appropriately.
	 */
	for (i = 1; i <= 7; i++) {
		calculated_speed = (dev->clock) >> (1+i);
		if (calculated_speed <= cfg->clock_rate)
			break;
	}

	if (i > 7)
		i = 7;

	ctrl |= ((i << XZYNQ_SPI_CR_BAUD_DIV_OFFSET)&XZYNQ_SPI_CR_BAUD_DIV);

	/* SPI clock is quiescent high */
	if (cfg->mode & SPI_MODE_CKPOL_HIGH)
		ctrl |= XZYNQ_SPI_CR_CLK_POL;

	/* SPI clock active outside the word */
	if (cfg->mode & SPI_MODE_CKPHASE_HALF)
		ctrl |= XZYNQ_SPI_CR_CLK_PH;

	/* Set mode as Master */
	ctrl |= XZYNQ_SPI_CR_MODE_SEL;

	/* Set manual start */
	ctrl |= XZYNQ_SPI_CR_MANSTRT_EN;

#ifdef DEBUG
	fprintf(stderr,"%s: rate %d clk %d\n", __FUNCTION__, cfg->clock_rate, dev->clock);
	fprintf(stderr,"%s: calculated speed: %d\n", __FUNCTION__,calculated_speed);
	fprintf(stderr,"%s: config %d ctrl = %#08x\n", __FUNCTION__, cs, ctrl);
	fprintf(stderr,"%s: config %d delay = %#08x\n", __FUNCTION__, cs, delay);
#endif

	dev->ctrl[cs] = ctrl;
	dev->delay[cs] = delay;

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/spi/xzynq/config.c $ $Rev: 752035 $")
#endif
