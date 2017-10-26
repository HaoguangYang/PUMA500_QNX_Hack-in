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

#ifndef __XZYNQ_SPI_H_INCLUDED
#define __XZYNQ_SPI_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <hw/spi-master.h>

/* #define DEBUG */

// Base addresses
#define XZYNQ_SPI0_BASEADDR		0xE0006000
#define XZYNQ_SPI1_BASEADDR		0xE0007000

#define XZYNQ_SPI_REGLEN		0x1000
#define XZYNQ_SPI_RXFIFOLEN		128
#define XZYNQ_SPI_TXFIFOLEN		128

// Interrupt number
#define XZYNQ_SPI0_INT_ID		58
#define XZYNQ_SPI1_INT_ID		81

#define XZYNQ_SPI_PRIORITY		21
#define XZYNQ_SPI_EVENT			1

// MIO clock
// TODO change clock to be dynamically computed depending on registers
#define XZYNQ_SPI_INPUT_CLOCK	166666666

// Registers offset
#define XZYNQ_SPI_CR_OFFSET		0x00
#define XZYNQ_SPI_ISR_OFFSET	0x04
#define XZYNQ_SPI_IER_OFFSET	0x08
#define XZYNQ_SPI_IDR_OFFSET	0x0C
#define XZYNQ_SPI_IMR_OFFSET	0x10
#define XZYNQ_SPI_EN_OFFSET		0x14
#define XZYNQ_SPI_DLY_OFFSET	0x18
#define XZYNQ_SPI_TXD_OFFSET	0x1C
#define XZYNQ_SPI_RXD_OFFSET	0x20
#define XZYNQ_SPI_SICR_OFFSET	0x24
#define XZYNQ_SPI_TXWR_OFFSET	0x28
#define XZYNQ_SPI_RXWR_OFFSET	0x2C
#define XZYNQ_SPI_MOD_ID_OFFSET	0xFC

// Configuration register masks
#define XZYNQ_SPI_CR_MODE_SEL	0x00000001
#define XZYNQ_SPI_CR_CLK_POL	0x00000002
#define XZYNQ_SPI_CR_CLK_PH		0x00000004
#define XZYNQ_SPI_CR_BAUD_DIV	0x00000038
#define XZYNQ_SPI_CR_FIFO_WIDTH	0x000000C0
#define XZYNQ_SPI_CR_REF_CLK	0x00000100
#define XZYNQ_SPI_CR_PERI_SEL	0x00000200
#define XZYNQ_SPI_CR_CS			0x00003c00
#define XZYNQ_SPI_CR_MAN_CS		0x00004000
#define XZYNQ_SPI_CR_MANSTRT_EN 0x00008000
#define XZYNQ_SPI_CR_MANSTRT	0x00010000
#define XZYNQ_SPI_CR_MFAIL_EN	0x00020000
// Configuration register offsets
#define XZYNQ_SPI_CR_BAUD_DIV_OFFSET	3
#define XZYNQ_SPI_FIFO_WIDTH_OFFSET		6
#define XZYNQ_SPI_CR_CS_OFFSET			10

// Interrupts register
#define XZYNQ_SPI_IXR_RXOVR		0x01
#define XZYNQ_SPI_IXR_MODF		0x02
#define XZYNQ_SPI_IXR_TXOW		0x04
#define XZYNQ_SPI_IXR_TXFULL	0x08
#define XZYNQ_SPI_IXR_RXNEMPTY	0x10
#define XZYNQ_SPI_IXR_RXFULL	0x20
#define XZYNQ_SPI_IXR_TXUF		0x40

// SPI enable register
#define XZYNQ_SPI_ENABLE		0x1

// Delay register
#define XZYNQ_SPI_DLY_INIT_OFFSET	0
#define XZYNQ_SPI_DLY_INIT_MASK		(0xFF << XZYNQ_SPI_DLY_INIT_OFFSET)
#define XZYNQ_SPI_DLY_AFTER_OFFSET	8
#define XZYNQ_SPI_DLY_AFTER_MASK	(0xFF << XZYNQ_SPI_DLY_AFTER_OFFSET)
#define XZYNQ_SPI_DLY_BTWN_OFFSET	16
#define XZYNQ_SPI_DLY_BTWN_MASK		(0xFF << XZYNQ_SPI_DLY_BTWN_OFFSET)
#define XZYNQ_SPI_DLY_D_NSS_OFFSET	24
#define XZYNQ_SPI_DLY_D_NSS_MASK	(0xFF << XZYNQ_SPI_DLY_D_NSS_OFFSET)

#define NUM_OF_SPI_DEVS 3

#define readX(addr)  fprintf(stderr, " [%s] = 0x%8.8x\n", #addr, in32( dev->vbase + addr ))

#define spiRegDump( ) \
	readX( XZYNQ_SPI_CR_OFFSET ); \
	readX( XZYNQ_SPI_ISR_OFFSET ); \
	readX( XZYNQ_SPI_IMR_OFFSET ); \
	readX( XZYNQ_SPI_EN_OFFSET ); \
	readX( XZYNQ_SPI_DLY_OFFSET ); \
	readX( XZYNQ_SPI_SICR_OFFSET ); \
	readX( XZYNQ_SPI_TXWR_OFFSET ); \
	readX( XZYNQ_SPI_RXWR_OFFSET ); \
	readX( XZYNQ_SPI_MOD_ID_OFFSET );

/* The structure which maintains the various parameters 
 * of the SPI module. 
 */ 
typedef struct {
    SPIDEV      spi;        /* This has to be the first element */
    uint64_t    pbase;
    uintptr_t   vbase;
    int         irq_spi;
    int         chid, coid;
    int         iid_spi;
    uint32_t    clock;
    uint8_t     *pbuf;
    int         xlen, tlen, rlen;
    int         dlen;
    int         dtime;    /* usec per data, for time out use */
    int         num_cs;
    int         cs_delay;
    struct sigevent spievent;
    int			gpiocs[NUM_OF_SPI_DEVS];
    int			ctrl[NUM_OF_SPI_DEVS];
    int			delay[NUM_OF_SPI_DEVS];
} xzynq_spi_t;

extern void set_port(unsigned port, unsigned mask, unsigned data);
extern void xzynq_spi_start_exchange(xzynq_spi_t *dev);
extern void *xzynq_init(void *hdl, char *options);
extern int xzynq_attach_intr(xzynq_spi_t *dev);
extern void xzynq_dinit(void *hdl);
extern int xzynq_drvinfo(void *hdl, spi_drvinfo_t *info);
extern int xzynq_devinfo(void *hdl, uint32_t device, spi_devinfo_t *info);
extern int xzynq_setcfg(void *hdl, uint16_t device, spi_cfg_t *cfg);
extern int xzynq_wait(xzynq_spi_t *dev, int len);
extern void *xzynq_xfer(void *hdl, uint32_t device, uint8_t *buf, int *len);
extern int xzynq_cfg(void *hdl, spi_cfg_t *cfg, int cs);

#endif //__XZYNQ_SPI_H_INCLUDED

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/spi/xzynq/xzynq_spi.h $ $Rev: 752035 $")
#endif
