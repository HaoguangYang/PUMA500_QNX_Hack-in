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
#include <arm/xzynq.h>

/* open() ... */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hw/clk.h>

#define GPIOCS_EN (1 << 30)	/* gpio based chip selection is enabled */

enum opt_index {BASE, CLOCK, IRQ, NUM_CS, CS_DELAY, GPIOCS0, GPIOCS1, GPIOCS2, END};
static char *xzynq_opts[] =
{
	[BASE]		= "base",	/* Base address for this SPI controller */
	[CLOCK]		= "clock",	/* defined the value of the clock source of the SPI */
	[IRQ]		= "irq",	/* IRQ for this SPI intereface */
	[NUM_CS]	= "num_cs",	/* number of devices supported */
    [CS_DELAY]	= "cs_delay", /* Disable/enable 1 1/2 clock CS to data delay */
	[GPIOCS0]	= "gpiocs0",/* gpio pin number to be used as chipselect 0*/
	[GPIOCS1]	= "gpiocs1",/* gpio pin number to be used as chipselect 1*/
	[GPIOCS2]	= "gpiocs2",/* gpio pin number to be used as chipselect 2*/
	[END] = NULL
};

spi_funcs_t spi_drv_entry =
{
	sizeof(spi_funcs_t),
	xzynq_init, /* init() */
	xzynq_dinit, /* fini() */
	xzynq_drvinfo, /* drvinfo() */
	xzynq_devinfo, /* devinfo() */
	xzynq_setcfg, /* setcfg() */
	xzynq_xfer, /* xfer() */
	NULL
};

/*
 * Note:
 * The devices listed are just examples, users should change
 * this according to their own hardware spec.
 */
static spi_devinfo_t devlist[NUM_OF_SPI_DEVS] =
{
	{
		0x00, // Device ID, for SS0
		"SPI-DEV0", // Description
		{
			(8), // data length 8bit, MSB
			5000000 // Clock rate 5M
		},
	},
	{
		0x01, // Device ID, for SS1
		"SPI-DEV1", // Description
		{
			(8), // data length 8bit, MSB
			5000000 // Clock rate 5M
		},
	},
    {
        0x02, // Device ID, for SS2
        "SPI-DEV2", // Description
        {
            (8), // data length 8bit, MSB
            5000000 // Clock rate 5M
        },
    }
};

#ifdef CONFIG_PMM
#include <hw/pmm.h>
#include <hw/clk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>

pmm_functions_t funcs;

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_pm_standby(void* arg)
{
	int fd, err;
	xzynq_spi_t *dev = (xzynq_spi_t *) arg;
	ctrl_id_t id = XZYNQ_SPI0_CTRL;

	if (dev->irq_spi == XZYNQ_SPI1_INT_ID) {
		id = XZYNQ_SPI1_CTRL;
	}

	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("open");
		slogf(_SLOGC_CONSOLE, _SLOG_INFO,
			"spi: %s can't open /dev/clock", __func__);
		return;
	}

	err = devctl(fd, DCMD_CLOCK_DISABLE, &id, sizeof(clk_id_t), NULL);
	if (err) {
		slogf(_SLOGC_CONSOLE, _SLOG_INFO,
			"spi: %s can't disable the clock", __func__);
	}

	close(fd);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_pm_resume(void* arg)
{
	int fd, err;
	xzynq_spi_t *dev = (xzynq_spi_t *) arg;
	ctrl_id_t id = XZYNQ_SPI0_CTRL;

	if (dev->irq_spi == XZYNQ_SPI1_INT_ID) {
		id = XZYNQ_SPI1_CTRL;
	}

	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("open");
		slogf(_SLOGC_CONSOLE, _SLOG_INFO,
			"spi: %s can't open /dev/clock", __func__);
		return;
	}

	err = devctl(fd, DCMD_CLOCK_ENABLE, &id, sizeof(clk_id_t), NULL);
	if (err) {
		slogf(_SLOGC_CONSOLE, _SLOG_INFO,
			"spi: %s can't disable the clock", __func__);
	}

	close(fd);
}

#endif

void set_port(unsigned port, unsigned mask, unsigned data)
{
	uint32_t c;

	c = in32(port);
	out32(port, (c & ~mask) | (data & mask));
}

static int xzynq_options(xzynq_spi_t *spi, char *optstring)
{
	int opt, rc = 0, err = EOK;
	char *options, *freeptr, *c, *value;

	if (optstring == NULL)
		return 0;

	freeptr = options = strdup(optstring);

	while (options && *options != '\0') {
		c = options;
		if ((opt = getsubopt(&options, xzynq_opts, &value)) == -1)
			goto error;

		switch (opt) {
		case BASE:
			spi->pbase = strtoull(value, 0, 0);
			continue;
		case CLOCK:
			spi->clock = strtoul(value, NULL, 0);
			continue;
		case IRQ:
			spi->irq_spi = strtoul(value, 0, 0);
			continue;
		case NUM_CS:
			spi->num_cs = strtoul(value, 0, 0);
			continue;
		case CS_DELAY:
			spi->cs_delay = strtoul(value, 0, 0);
			continue;
		case GPIOCS0:
			spi->gpiocs[0] = strtoul(value, 0, 0) | GPIOCS_EN;
			continue;
		case GPIOCS1:
			spi->gpiocs[1] = strtoul(value, 0, 0) | GPIOCS_EN;
			continue;
		case GPIOCS2:
			spi->gpiocs[2] = strtoul(value, 0, 0) | GPIOCS_EN;
			continue;
		}
		error: fprintf(stderr, "xzynq-spi: unknown option %s", c);
		err = EINVAL;
		rc = -1;
	}

	free(freeptr);

	return rc;
}

static void xzynq_spi_slave_select(xzynq_spi_t *dev, int devid, int on)
{
	int ctrl = dev->ctrl[devid];

	if (dev->gpiocs[devid]) {
		// TODO set GPIO manually
	} else {
		if (on) {
			ctrl &= ~(XZYNQ_SPI_CR_CS);
			ctrl |= (((1 << devid) << XZYNQ_SPI_CR_CS_OFFSET)
					& XZYNQ_SPI_CR_CS);
		} else {
			ctrl &= ~(XZYNQ_SPI_CR_CS);
		}
	}

	/* Slave select value will be written along the start condition */
	dev->ctrl[devid] = ctrl;
}

void xzynq_spi_start_exchange(xzynq_spi_t *dev)
{
	uint32_t ctrl = in32(dev->vbase + XZYNQ_SPI_CR_OFFSET);

	if (ctrl & XZYNQ_SPI_CR_MANSTRT_EN)
		/* Manual start command */
		out32(dev->vbase + XZYNQ_SPI_CR_OFFSET, ctrl | XZYNQ_SPI_CR_MANSTRT);

	/* Enable controller */
	out32(dev->vbase + XZYNQ_SPI_EN_OFFSET, XZYNQ_SPI_ENABLE);
}

void xzynq_spi_end_exchange(xzynq_spi_t *dev)
{
	/* Disable QSPI */
	out32(dev->vbase + XZYNQ_SPI_EN_OFFSET, ~XZYNQ_SPI_ENABLE);
}

void *xzynq_init(void *hdl, char *options)
{
	xzynq_spi_t *dev;
	uintptr_t base;
	uintptr_t slcr;
	int i;
	uint32_t reg;
	clk_freq_t clk_freq;
	int fd, err;

	dev = calloc(1, sizeof(xzynq_spi_t));

	if (dev == NULL)
		return NULL;

	/* Set defaults */
	dev->pbase = XZYNQ_SPI0_BASEADDR;
	dev->irq_spi = XZYNQ_SPI0_INT_ID;
	dev->num_cs = 1;
	dev->cs_delay = 0;
	dev->gpiocs[0] = 0;
	dev->gpiocs[1] = 0;
	dev->gpiocs[2] = 0;

	/* Get I2C Clock */
	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("Can't get SPI clock");
		goto fail0;
	}

	clk_freq.id = XZYNQ_SPI_CLK;
	err = devctl(fd, DCMD_CLOCK_GET_FREQ, &clk_freq, sizeof(clk_freq_t), NULL);
	if (err) {
		perror("devctl");
		goto fail0;
	}

	if (clk_freq.freq == -1) {
		perror("/dev/clock: Invalid frequency (-1)");
		goto fail0;
	}

	/* Set real frequency */
	dev->clock = clk_freq.freq;
	close(fd);

	if (xzynq_options(dev, options))
		goto fail0;

	/* Map in SPI registers */
	if ((base = mmap_device_io(XZYNQ_SPI_REGLEN, dev->pbase))
			== (uintptr_t) MAP_FAILED)
		goto fail0;

	dev->vbase = base;

	/* Disable controller */
	out32(base + XZYNQ_SPI_EN_OFFSET, 0);

	/* Calculate all device configuration here */
	for (i = 0; i < dev->num_cs; i++) {
		if (xzynq_cfg(dev, &devlist[i].cfg, i) != 0) {
			fprintf(stderr, "error initializing device #%d!", i);
			goto fail1;
		}
	}

	/* Disable interrupts */
	out32(base + XZYNQ_SPI_IDR_OFFSET, 0xFFFFFFFF);

	/* Attach SPI interrupt */
	if (xzynq_attach_intr(dev))
		goto fail1;

	dev->spi.hdl = hdl;

	/* Clear the appropriate Interrupts if any*/
	reg = in32(base + XZYNQ_SPI_ISR_OFFSET);
	out32((base + XZYNQ_SPI_ISR_OFFSET), reg);

	/* Set transfer levels */
	out32(base + XZYNQ_SPI_TXWR_OFFSET, 1);
	out32(base + XZYNQ_SPI_RXWR_OFFSET, 1);

	/* Enable controller */
	out32(base + XZYNQ_SPI_EN_OFFSET, 1);

	if ((slcr = mmap_device_io(XZYNQ_SLCR_LEN, XZYNQ_SLCR_BASE))
			== (uintptr_t) MAP_FAILED) {
		fprintf(stderr, "Couldnt check the clocks\n");
		return dev;
	}

	/* Unlocking SLCR registers */
	out32(slcr + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Making sure of the 166MHz ref clock */
	reg = (0x6 << XZYNQ_SLCR_PER_DIV_SHIFT) |
		(XZYNQ_SLCR_PER_SRC_IOPLL << XZYNQ_SLCR_PER_SRC_SHIFT) |
		XZYNQ_SLCR_PER_CLK_ACT0 | XZYNQ_SLCR_PER_CLK_ACT1;
	out32(slcr + XZYNQ_SLCR_SPI_CLK_CTRL_REG, reg);
#ifdef DEBUG
	reg = in32(slcr + XZYNQ_SLCR_SPI_CLK_CTRL_REG);
	fprintf(stderr, "%s: slcr SPI ctrl = %08x\n", __FUNCTION__, reg);
#endif

	/* Activating SPI clocks */
	reg = in32(slcr + XZYNQ_SLCR_APER_CLK_CTRL_REG);
	reg |= XZYNQ_SLCR_APER_CLK_CTRL_SPI0;
	reg |= XZYNQ_SLCR_APER_CLK_CTRL_SPI1;
	out32(slcr + XZYNQ_SLCR_APER_CLK_CTRL_REG, reg);
#ifdef DEBUG
	reg = in32(slcr + XZYNQ_SLCR_APER_CLK_CTRL_REG);
	fprintf(stderr,"%s: slcr APER reg = %08x\n", __FUNCTION__, reg);
#endif

	/* Locking SLCR registers */
//	out32(slcr + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	munmap_device_io(slcr, XZYNQ_SLCR_LEN);

#ifdef CONFIG_PMM
	funcs.arg = dev;
	funcs.standby = xzynq_pm_standby;
	funcs.resume = xzynq_pm_resume;
	if (dev->irq_spi == XZYNQ_SPI0_INT_ID) {
		pmm_init(&funcs, "spi0");
	} else {
		pmm_init(&funcs, "spi1");
	}
#endif

	return dev;
fail1:
	munmap_device_io(dev->vbase, XZYNQ_SPI_REGLEN);
fail0:
	free(dev);
	return NULL;
}

void xzynq_setup(xzynq_spi_t *dev, uint32_t device)
{
	uintptr_t base = dev->vbase;
	uint32_t id;

	id = device & SPI_DEV_ID_MASK;
	out32(base + XZYNQ_SPI_CR_OFFSET, dev->ctrl[id]);
	out32(base + XZYNQ_SPI_DLY_OFFSET, dev->delay[id]);
}

void xzynq_dinit(void *hdl)
{
	xzynq_spi_t *dev = hdl;
	uint32_t base = dev->vbase;

	/* Disable controller */
	out32(base + XZYNQ_SPI_EN_OFFSET, 0);

	/* Unmap the register, detach the interrupt */
	InterruptDetach(dev->iid_spi);
	munmap_device_io(dev->vbase, XZYNQ_SPI_REGLEN);

	ConnectDetach(dev->coid);
	ChannelDestroy(dev->chid);

	free(hdl);
}

int xzynq_drvinfo(void *hdl, spi_drvinfo_t *info)
{
	info->version = (SPI_VERSION_MAJOR << SPI_VERMAJOR_SHIFT)
			| (SPI_VERSION_MINOR << SPI_VERMINOR_SHIFT) | (SPI_REVISION
			<< SPI_VERREV_SHIFT);
	strcpy(info->name, "XZYNQ SPI");

	return (EOK);
}

int xzynq_setcfg(void *hdl, uint16_t device, spi_cfg_t *cfg)
{
	uint32_t ret;
	xzynq_spi_t *dev = hdl;

	if (device >= dev->num_cs)
		return (EINVAL);

	memcpy(&devlist[device].cfg, cfg, sizeof(spi_cfg_t));

	ret = xzynq_cfg(hdl, &devlist[device].cfg, device);
	if (ret != 0)
		return (EINVAL);

	return (EOK);
}

int xzynq_devinfo(void *hdl, uint32_t device, spi_devinfo_t *info)
{
	xzynq_spi_t *dev = hdl;
	int id = device & SPI_DEV_ID_MASK;

	if (device & SPI_DEV_DEFAULT) {
		/* Info of this device */
		if (id >= 0 && id < dev->num_cs)
			memcpy(info, &devlist[id], sizeof(spi_devinfo_t));
		else
			return (EINVAL);
	} else {
		/* Info of next device */
		if (id == SPI_DEV_ID_NONE)
			id = -1;
		if (id < (dev->num_cs - 1))
			memcpy(info, &devlist[id + 1], sizeof(spi_devinfo_t));
		else
			return (EINVAL);
	}

	return (EOK);
}

void *xzynq_xfer(void *hdl, uint32_t device, uint8_t *buf, int *len)
{
	xzynq_spi_t *dev = hdl;
	uintptr_t base = dev->vbase;
	uint32_t id;
#ifdef DEBUG
	uint32_t i;
#endif
	id = device & SPI_DEV_ID_MASK;
	if (id >= dev->num_cs) {
		*len = -1;
		return buf;
	}

	/* Cannot set more than 64KB of data at one time */
	if (dev->xlen > (64 * 1024)) {
		*len = -1;
		return buf;
	}

	dev->dlen = ((devlist[id].cfg.mode & SPI_MODE_CHAR_LEN_MASK) + 7) >> 3;

	/* Estimate transfer time in us... The calculated dtime is only used for
	 * the timeout, so it doesn't have to be that accurate.  At higher clock
	 * rates, a calculated dtime of 0 would mess-up the timeout calculation, so
	 * round up to 1 us
	 */
	dev->dtime = dev->dlen * 1000 * 1000 / devlist[id].cfg.clock_rate;
	if (dev->dtime == 0)
		dev->dtime = 1;

	/* Setup controller for that device */
	xzynq_setup(dev, device);

	/* Empty RX FIFO */
	while (in32(dev->vbase + XZYNQ_SPI_ISR_OFFSET) & XZYNQ_SPI_IXR_RXNEMPTY)
		in32(dev->vbase + XZYNQ_SPI_RXD_OFFSET);

	xzynq_spi_slave_select(dev, id, 1);

	dev->xlen = *len;
	dev->rlen = 0;
	dev->tlen = 0;
	dev->pbuf = buf;

#ifdef DEBUG
	fprintf(stderr, "%s: Xfer Packet length = %2.2x\nTx: ", __FUNCTION__,
			dev->xlen);
	for (i=0; i<dev->xlen; i++) {
		fprintf(stderr, "%2.2x", dev->pbuf[i]);
		if (i % 2 == 0)
			fprintf(stderr, " ");
	}
	fprintf(stderr, "\n");
#endif

	while ((dev->tlen < dev->xlen) && !(in32(dev->vbase + XZYNQ_SPI_ISR_OFFSET)
			& XZYNQ_SPI_IXR_TXFULL)) {
		switch (dev->dlen) {
		case 1:
			out32(base + XZYNQ_SPI_TXD_OFFSET, dev->pbuf[dev->tlen]);
			dev->tlen++;
			break;
		case 2:
			out32(base + XZYNQ_SPI_TXD_OFFSET,
					*(uint16_t*) (&dev->pbuf[dev->tlen]));
			dev->tlen += 2;
			break;
		case 3:
		case 4:
			out32(base + XZYNQ_SPI_TXD_OFFSET,
					*(uint32_t*) (&dev->pbuf[dev->tlen]));
			dev->tlen += 4;
			break;
		}
	}

	/* Enable Interrupts */
	out32(base + XZYNQ_SPI_IER_OFFSET, XZYNQ_SPI_IXR_TXOW | XZYNQ_SPI_IXR_MODF);
	xzynq_spi_start_exchange(dev);

	/* Wait for exchange to finish */
	if (xzynq_wait(dev, dev->xlen)) {
		fprintf(stderr, "XZYNQ SPI: XFER Timeout!!!\n");
	}

	xzynq_spi_slave_select(dev, id, 0);

	xzynq_spi_end_exchange(dev);

	*len = dev->rlen;

#ifdef DEBUG
	fprintf(stderr, "dev->xlen: %08x\n", dev->xlen);
	fprintf(stderr, "dev->rlen: %08x\n", dev->rlen);
	fprintf(stderr, "\nRx: ");
	for (i = 0; i < dev->rlen; i++) {
		fprintf(stderr, "%2.2x", dev->pbuf[i]);
		if (i % 2 == 0) {
			fprintf(stderr, " ");
		}
	}
	fprintf(stderr, "\n");
#endif

	return buf;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/spi/xzynq/xzynq_spi.c $ $Rev: 752035 $")
#endif
