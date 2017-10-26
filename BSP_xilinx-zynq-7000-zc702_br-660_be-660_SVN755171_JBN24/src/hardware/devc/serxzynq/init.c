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

#include "externs.h"
#include <sys/mman.h>
#include <string.h>

/*
 * Specify parameters for default devices.
 */
void *
query_default_device(TTYINIT_XZYNQ *dip, void *link)
{
    /*
     * No default device, the base address and irq have be be specified
     */
    return NULL;
}

void
create_device(TTYINIT_XZYNQ *dip, unsigned unit)
{
    DEV_XZYNQ        *dev;

    /*
     * Get a device entry and the input/output buffers for it.
     */
    dev = calloc(1, sizeof(*dev));

    if(dev == NULL)
    {
        perror("UART: Unable to allocate device entry\n");
        exit(1);
    }

    devptr = dev;

    /*
     * Get buffers.
     */
    dev->tty.ibuf.head = dev->tty.ibuf.tail = dev->tty.ibuf.buff = malloc(dev->tty.ibuf.size = dip->tty.isize);
    dev->tty.obuf.head = dev->tty.obuf.tail = dev->tty.obuf.buff = malloc(dev->tty.obuf.size = dip->tty.osize);
    dev->tty.cbuf.head = dev->tty.cbuf.tail = dev->tty.cbuf.buff = malloc(dev->tty.cbuf.size = dip->tty.csize);
    dev->tty.highwater = dev->tty.ibuf.size - (dev->tty.ibuf.size < 128 ? dev->tty.ibuf.size/4 : 100);

    strcpy(dev->tty.name, dip->tty.name);

    dev->tty.baud    = dip->tty.baud;
    dev->tty.flags   = EDIT_INSERT | LOSES_TX_INTR;
    dev->tty.c_cflag = dip->tty.c_cflag;
    dev->tty.c_iflag = dip->tty.c_iflag;
    dev->tty.c_lflag = dip->tty.c_lflag;
    dev->tty.c_oflag = dip->tty.c_oflag;
    dev->tty.verbose = dip->tty.verbose;
    dev->tty.fifo    = dip->tty.fifo;

    dev->fifo        = dip->tty.fifo;
    dev->intr[0]     = dip->intr[0];
    dev->intr[1]     = dip->intr[1];
    dev->clk         = dip->tty.clk;
    dev->div         = dip->tty.div;
    dev->isr         = dip->isr;

    /*
     * Map device registers
     */
    dev->base = mmap_device_io(XZYNQ_XUARTPS_UART_SIZE, dip->tty.port);
    if (dev->base == (uintptr_t)MAP_FAILED) {
        perror("UART: MAP_FAILED\n");
        goto fail1;
    }

    /*
    * Initialize termios cc codes to an ANSI terminal.
    */
    ttc(TTC_INIT_CC, &dev->tty, 0);

    /*
    * Initialize the device's name.
    * Assume that the basename is set in device name.  This will attach
    * to the path assigned by the unit number/minor number combination
    */
    unit = SET_NAME_NUMBER(unit) | NUMBER_DEV_FROM_USER;
    ttc(TTC_INIT_TTYNAME, &dev->tty, unit);

    /*
    * Initialize power management structures before attaching ISR
    */
    ttc(TTC_INIT_POWER, &dev->tty, 0);

    /*
    * Only setup IRQ handler for non-pcmcia devices.
    * Pcmcia devices will have this done later when card is inserted.
    */
    if (dip->tty.port != 0 && dev->intr[0] != -1) {
        ser_stty(dev);
        ser_attach_intr(dev);
    }

    /*
    * Attach the resource manager
    */
    ttc(TTC_INIT_ATTACH, &dev->tty, 0);

    return;

fail1:
    free(dev->tty.obuf.buff);
    free(dev->tty.ibuf.buff);
    free(dev->tty.cbuf.buff);
    free (dev);
    exit(1);

}

void dinit()
{
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/devc/serxzynq/init.c $ $Rev: 752035 $")
#endif
