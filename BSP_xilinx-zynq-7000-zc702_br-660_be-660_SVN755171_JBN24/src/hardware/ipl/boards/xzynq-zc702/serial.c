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

#include "ipl.h"
#include <hw/inout.h>
#include "ps7_init.h"
#include "ipl_xzynq.h"

static unsigned char xzynq_uart_pollkey();
static unsigned char xzynq_uart_getchar();
static void xzynq_uart_putchar(unsigned char);

static const ser_dev xzynq_dev = { xzynq_uart_getchar, xzynq_uart_putchar,
		xzynq_uart_pollkey };

unsigned char xzynq_uart_pollkey(void)
{
	if (XZYNQ_CONSOLE_SR & XZYNQ_XUARTPS_IXR_RTRIG)
		return 1;
	else
		return 0;
}

unsigned char xzynq_uart_getchar(void)
{
	while (!(xzynq_uart_pollkey()))
		;
	return ((unsigned char) XZYNQ_CONSOLE_FIFO);
}

void xzynq_uart_putchar(unsigned char data1)
{
	while (!(XZYNQ_CONSOLE_SR & XZYNQ_XUARTPS_SR_TXEMPTY))
		;
	XZYNQ_CONSOLE_FIFO = (unsigned short) data1;
}

void init_serial_xzynq()
{
	unsigned char IterBAUDDIV; /* Iterator for available baud divisor values */
	unsigned int BRGR_Value; /* Calculated value for baud rate generator */
	unsigned int CalcBaudRate; /* Calculated baud rate */
	unsigned int BaudError; /* Diff between calculated and requested baudrate */
	unsigned int Best_BRGR = 0; /* Best value for baud rate generator */
	unsigned char Best_BAUDDIV = 0; /* Best value for baud divisor */
	unsigned int Best_Error = 0xFFFFFFFF;
	unsigned int PercentError;
	unsigned int InputClk = UART_FREQ;
	unsigned int BaudRate = XZYNQ_CONSOLE_BR;

	/* Wait for UART to finish transmitting */
	while (!(XZYNQ_CONSOLE_SR & XZYNQ_XUARTPS_SR_TXEMPTY))
		;

	/* Disable UART */
	XZYNQ_CONSOLE_CR &= (unsigned short) ~(XZYNQ_XUARTPS_CR_TX_EN
			| XZYNQ_XUARTPS_CR_RX_EN);

	/* Set baudrate */
	/*
	 * Determine the Baud divider. It can be 4to 254.
	 * Loop through all possible combinations
	 */
	for (IterBAUDDIV = 4; IterBAUDDIV < 255; IterBAUDDIV++) {
		/*
		 * Calculate the value for BRGR register
		 */
		BRGR_Value = InputClk / (BaudRate * (IterBAUDDIV + 1));
		/*
		 * Calculate the baud rate from the BRGR value
		 */
		CalcBaudRate = InputClk / (BRGR_Value * (IterBAUDDIV + 1));
		/*
		 * Avoid unsigned integer underflow
		 */
		if (BaudRate > CalcBaudRate) {
			BaudError = BaudRate - CalcBaudRate;
		} else {
			BaudError = CalcBaudRate - BaudRate;
		}
		/*
		 * Find the calculated baud rate closest to requested baud rate.
		 */
		if (Best_Error > BaudError) {
			Best_BRGR = BRGR_Value;
			Best_BAUDDIV = IterBAUDDIV;
			Best_Error = BaudError;
		}
	}

	/*
	 * Make sure the best error is not too large.
	 */
	PercentError = (Best_Error * 100) / BaudRate;
	if (XZYNQ_MAX_BR_ERROR < PercentError) {
		return;
	}

	XZYNQ_CONSOLE_BAUDGEN = Best_BRGR;
	XZYNQ_CONSOLE_BAUDDIV = Best_BAUDDIV;

	/*
	 * 8 data, 1 stop, 0 parity bits
	 * sel_clk=uart_clk=APB clock
	 */
	XZYNQ_CONSOLE_MR = (XZYNQ_XUARTPS_MR_CHARLEN_8_BIT
			| XZYNQ_XUARTPS_MR_PARITY_NONE | XZYNQ_XUARTPS_MR_STOPMODE_1_BIT);

	/* Enable UART */
	XZYNQ_CONSOLE_CR |= (unsigned short) (XZYNQ_XUARTPS_CR_TX_EN
			| XZYNQ_XUARTPS_CR_RX_EN | XZYNQ_XUARTPS_CR_RXRST
			| XZYNQ_XUARTPS_CR_TXRST);

	/* Setting trigger to 1 byte */
	XZYNQ_CONSOLE_RXWM = 1;

	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *) &xzynq_dev);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
