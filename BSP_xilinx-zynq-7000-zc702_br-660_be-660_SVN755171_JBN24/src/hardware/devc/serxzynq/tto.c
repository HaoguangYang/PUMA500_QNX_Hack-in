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

int
tto(TTYDEV *ttydev, int action, int arg1)
{
    TTYBUF           *bup = &ttydev->obuf;
    DEV_XZYNQ          *dev = (DEV_XZYNQ *)ttydev;
    uintptr_t        base = dev->base;
    unsigned char    c;

    switch (action) {
        case TTO_STTY:
            ser_stty(dev);
            return 0;

        case TTO_LINESTATUS:
            return (in32(base + XZYNQ_XUARTPS_SR_REG));

        case TTO_CTRL:
        case TTO_DATA:
        case TTO_EVENT:
            break;

        default:
        	return 0;
	}

	while (bup->cnt > 0) {
		/*
		 * If the OSW_PAGED_OVERRIDE flag is set then allow
		 * transmit of character even if output is suspended via
		 * the OSW_PAGED flag. This flag implies that the next
		 * character in the obuf is a software flow control
		 * charater (STOP/START).
		 * Note: tx_inject sets it up so that the contol
		 *       character is at the start (tail) of the buffer.
		 */
		if (dev->tty.flags & (OHW_PAGED | OSW_PAGED) && !(dev->tty.xflags
				& OSW_PAGED_OVERRIDE))
			break;

		/*
		 * Get the next character to print from the output buffer
		 */
		dev_lock(&dev->tty);
		c = tto_getchar(&dev->tty);
		dev_unlock(&dev->tty);

		dev->tty.un.s.tx_tmr = 3; /* Timeout 3 */
		/* TODO Use timer interrupts */
		while (in32(base + XZYNQ_XUARTPS_SR_REG) & XZYNQ_XUARTPS_SR_TXFULL);
		out32(base + XZYNQ_XUARTPS_FIFO_REG, c);

		/* Clear the OSW_PAGED_OVERRIDE flag as we only want
		 * one character to be transmitted in this case.
		 */
		if (dev->tty.xflags & OSW_PAGED_OVERRIDE) {
			atomic_clr(&dev->tty.xflags, OSW_PAGED_OVERRIDE);
			break;
		}
	}

    return (tto_checkclients(&dev->tty));
}

void ser_stty(DEV_XZYNQ *dev)
{
    uintptr_t base = dev->base;
    unsigned cr, mr, clk, div, best_div, baud;
    unsigned brgr, best_brgr, error, best_error = 0xFFFFFFFF;
    uint32_t timeout;

    dev->cr = cr = in32(base + XZYNQ_XUARTPS_CR_REG);
    dev->mr = mr = in32(base + XZYNQ_XUARTPS_MR_REG);
    dev->div = div = best_div = in32(base + XZYNQ_XUARTPS_BAUDDIV_REG);
    dev->brgr = brgr = best_brgr = in32(base + XZYNQ_XUARTPS_BAUDGEN_REG);
    clk = dev->clk;

   /*
     * Calculate baud rate divisor, data size, stop bits and parity
     */
    for (div=4; div<255; div++) {
    	brgr = clk / (dev->tty.baud*(div+1));

    	baud = clk / (brgr*(div+1));

    	if (dev->tty.baud > baud)
    		error = dev->tty.baud - baud;
    	else
    		error = baud - dev->tty.baud;

    	if (error < best_error) {
    		best_brgr = brgr;
    		best_div = div;
    		best_error = error;
    	}

    	if (error == 0)
    		break;
    }

    switch (dev->tty.c_cflag & CSIZE) {
        case CS8:
            mr &= ~(3<<1);
        	mr |= XZYNQ_XUARTPS_MR_CHARLEN_8_BIT;
            break;

        case CS7:
            mr &= ~(3<<1);
        	mr |= XZYNQ_XUARTPS_MR_CHARLEN_7_BIT;
            break;
    }

    if (dev->tty.c_cflag & CSTOPB) {
        mr &= ~(3<<8);
        mr |= XZYNQ_XUARTPS_MR_STOPMODE_2_BIT;
    } else {
        mr &= ~(3<<8);
        mr |= XZYNQ_XUARTPS_MR_STOPMODE_1_BIT;
    }

    mr &= ~(5<<3);
    if (dev->tty.c_cflag & PARENB) {
        if (dev->tty.c_cflag & PARODD)
            mr |= XZYNQ_XUARTPS_MR_PARITY_ODD;
    } else {
    	mr |= XZYNQ_XUARTPS_MR_PARITY_NONE;
    }

	cr |= XZYNQ_XUARTPS_CR_TX_EN | XZYNQ_XUARTPS_CR_RX_EN;

	if ((dev->cr == cr) && (dev->mr == mr) && (dev->div == best_div) &&
			(dev->brgr == best_brgr))
        return;

    dev->cr = cr;
    dev->mr = mr;
    dev->div = best_div;
    dev->brgr = best_brgr;

    /*
     * Wait for Tx FIFO and shift register empty if the UART is enabled
     */
    timeout = 100000;
    if ((in32(base + XZYNQ_XUARTPS_CR_REG) & XZYNQ_XUARTPS_CR_TX_EN)) {
        while (!(in32(base + XZYNQ_XUARTPS_SR_REG) & XZYNQ_XUARTPS_SR_TXEMPTY) && timeout--);
    }

    /* Reset UART */
    out32(base + XZYNQ_XUARTPS_CR_REG, XZYNQ_XUARTPS_CR_TXRST|XZYNQ_XUARTPS_CR_RXRST);

    while (in32(base + XZYNQ_XUARTPS_CR_REG)&(XZYNQ_XUARTPS_CR_TXRST|XZYNQ_XUARTPS_CR_RXRST));

    /* Set UART FIFO triggers */
	out32(base + XZYNQ_XUARTPS_RXWM_REG, (dev->fifo&0x3f));
	out32(base + XZYNQ_XUARTPS_TXWM_REG, ((dev->fifo>>10)&0x3f));

	/* Set baudrate */
	out32(base + XZYNQ_XUARTPS_BAUDGEN_REG, dev->brgr);
	out32(base + XZYNQ_XUARTPS_BAUDDIV_REG, dev->div);

	/* Set mode register */
	out32(base + XZYNQ_XUARTPS_MR_REG, dev->mr);

	/* Activate UART */
    out32(base + XZYNQ_XUARTPS_CR_REG, dev->cr);

    /* RX Timeout */
    out32(base + XZYNQ_XUARTPS_RXTOUT_REG, 10);

    /* Activate RX interrupt */
	out32(base + XZYNQ_XUARTPS_IER_REG, (XZYNQ_XUARTPS_IXR_RTRIG|XZYNQ_XUARTPS_IXR_TOUT));
	out32(base + XZYNQ_XUARTPS_IDR_REG, ~(XZYNQ_XUARTPS_IXR_RTRIG|XZYNQ_XUARTPS_IXR_TOUT));

	/* Clear interrupts */
	out32(base + XZYNQ_XUARTPS_ISR_REG, 0xFFFFFFFF);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/devc/serxzynq/tto.c $ $Rev: 752035 $")
#endif
