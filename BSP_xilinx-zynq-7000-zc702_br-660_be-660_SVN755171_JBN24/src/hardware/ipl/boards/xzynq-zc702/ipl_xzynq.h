/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems.
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

#ifndef _IPL_XZYNQ_H
#define _IPL_XZYNQ_H

#include <arm/xzynq.h>
#include <stdint.h>

/*
 * Global configuration for the Ethernet load image
 */
#define ARP_RETRY	30
#define ARP_DELAY	1000 /* 1s between each ARP */
#define TFTP_RETRY	3
#define TFTP_DELAY	10000 /* 10s between each TFTP request */

/*
 * Address where the binary will be loaded, then the
 * startup will be copied automatically to the correct
 * address during the image_setup()
 */
#define QNX_LOAD_ADDR	0x16000000
extern uint8_t TFTP_IP_SERVER[4];

/* Macro to stringify a number to a string */
#define __stringify1(x)	#x
#define __stringify(x)	__stringify1(x)

#define __packed        __attribute__((packed))

#define LE_2_BE_32(l) \
    ((((l) & 0x000000FF) << 24) | \
	(((l) & 0x0000FF00) << 8)  | \
	(((l) & 0x00FF0000) >> 8)  | \
	(((l) & 0xFF000000) >> 24))

#define XZYNQ_CONSOLE_BASE		XZYNQ_XUARTPS_UART1_BASE
#define XZYNQ_CONSOLE_BR		115200
#define XZYNQ_MAX_BR_ERROR		3

/* UART registers, offset from base address */
#define XZYNQ_CONSOLE_CR             *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_CR_REG)	/* UART Control register */
#define XZYNQ_CONSOLE_MR             *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_MR_REG)	/* UART Mode register */
#define XZYNQ_CONSOLE_IER            *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_IER_REG)	/* Interrupt Enable register */
#define XZYNQ_CONSOLE_IDR            *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_IDR_REG)	/* Interrupt Disable register */
#define XZYNQ_CONSOLE_IMR            *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_IMR_REG)	/* Interrupt Mask register */
#define XZYNQ_CONSOLE_ISR            *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_ISR_REG)	/* Channel int Status register */
#define XZYNQ_CONSOLE_BAUDGEN        *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_BAUDGEN_REG)	/* Baud rate divider register */
#define XZYNQ_CONSOLE_RXTOUT         *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_RXTOUT_REG)	/* Receiver timeout register */
#define XZYNQ_CONSOLE_RXWM           *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_RXWM_REG)	/* Receiver FIFO trigger level */
#define XZYNQ_CONSOLE_MODEMCR        *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_MODEMCR_REG)	/* Modem control register */
#define XZYNQ_CONSOLE_MODEMSR        *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_MODEMSR_REG)	/* Modem status register */
#define XZYNQ_CONSOLE_SR             *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_SR_REG)	/* Channel status register */
#define XZYNQ_CONSOLE_FIFO           *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_FIFO_REG)	/* Transmit and Receive FIFO */
#define XZYNQ_CONSOLE_BAUDDIV        *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_BAUDDIV_REG)	/* Baud Rate Count Register */
#define XZYNQ_CONSOLE_FLOWDEL        *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_FLOWDEL_REG)	/* Flow Control delay register */
#define XZYNQ_CONSOLE_TXTOUT         *(volatile unsigned int *) (XZYNQ_CONSOLE_BASE + XZYNQ_XUARTPS_TXTOUT_REG)	/* Transmitter timeout register */

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
