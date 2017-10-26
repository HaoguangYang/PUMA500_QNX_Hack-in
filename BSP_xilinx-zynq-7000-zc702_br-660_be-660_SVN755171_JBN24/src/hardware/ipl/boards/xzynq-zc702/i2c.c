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

#include "i2c.h"

int i2c_set_clk(uint32_t f_clk_hz)
{
	uint32_t div_a;
	uint32_t div_b;
	uint32_t actual_fscl;
	uint32_t temp;
	uint32_t last_error;
	uint32_t current_error;
	uint32_t ctrl_reg;
	uint32_t best_div_a;
	uint32_t best_div_b;

	if (0 != in32(XZYNQ_XIICPS1_BASE_ADDR +
			XZYNQ_XIICPS_TRANS_SIZE_OFFSET)) {
		return -1;
	}

	/*
	 * Assume div_a is 0 and calculate (divisor_a+1) x (divisor_b+1).
	 */
	temp = 111111115 / (22 * f_clk_hz); /* 111111115: input clock */

	/*
	 * If the answer is negative or 0, the Fscl input is out of range.
	 */
	if (0 == temp) {
		return -1;
	}

	last_error = f_clk_hz;
	best_div_a = 0;
	best_div_b = 0;

	for (div_a = 0; div_a < 4; div_a++) {
		div_b = (temp / (div_a + 1)) - 1;
		actual_fscl = 111111115 / /* 111111115: input clock */
			(22 * (div_a + 1) * (div_b + 1));

		current_error = ((actual_fscl > f_clk_hz) ? (actual_fscl - f_clk_hz) :
				(f_clk_hz - actual_fscl));
		if (last_error > current_error) {
			best_div_a = div_a;
			best_div_b = div_b;
			last_error = current_error;
		}
	}

	/*
	 * Read the control register and mask the Divisors.
	 */
	ctrl_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_CR_OFFSET);
	ctrl_reg &= ~(XZYNQ_XIICPS_CR_DIV_A_MASK | XZYNQ_XIICPS_CR_DIV_B_MASK);
	ctrl_reg |= (best_div_a << XZYNQ_XIICPS_CR_DIV_A_SHIFT) |
		(best_div_b << XZYNQ_XIICPS_CR_DIV_B_SHIFT);

	out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);

	return 0;
}

int i2c_bus_is_busy()
{
	uint32_t sts_reg;
	sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_SR_OFFSET);
	return sts_reg & XZYNQ_XIICPS_SR_BA_MASK;
}


int i2c_transmit_fifo_fill(uint8_t *buffer, int byte_count)
{
	uint8_t avail_bytes;
	int loop_cnt;
	int num_bytes_to_send;

	/*
	 * Determine number of bytes to write to FIFO.
	 */
	avail_bytes = XZYNQ_XIICPS_FIFO_DEPTH -
			in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_TRANS_SIZE_OFFSET);

	if (byte_count > avail_bytes) {
		num_bytes_to_send = avail_bytes;
	} else {
		num_bytes_to_send = byte_count;
	}

	/*
	 * Fill FIFO with amount determined above.
	 */
	for (loop_cnt = 0; loop_cnt < num_bytes_to_send; loop_cnt++) {
		out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_DATA_OFFSET, *buffer++);
		byte_count --;
	}

	return byte_count;
}


static int i2c_setup_master(int role)
{
	uint32_t ctrl_reg;
	uint32_t enabled_intr = 0x0;

	ctrl_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_CR_OFFSET);

	/*
	 * Only check if bus is busy when repeated start option is not set.
	 */
	if ((ctrl_reg & XZYNQ_XIICPS_CR_HOLD_MASK) == 0) {
		if (i2c_bus_is_busy()) {
			return -1;
		}
	}

	/*
	 * Set up master, AckEn, nea and also clear fifo.
	 */
	ctrl_reg |= XZYNQ_XIICPS_CR_ACKEN_MASK | XZYNQ_XIICPS_CR_CLR_FIFO_MASK |
			XZYNQ_XIICPS_CR_NEA_MASK | XZYNQ_XIICPS_CR_MS_MASK;

	if (role == XZYNQ_ROLE_MASTER_RECEIVE) {
		ctrl_reg |= XZYNQ_XIICPS_CR_RD_WR_MASK;
		enabled_intr = XZYNQ_XIICPS_IXR_DATA_MASK | XZYNQ_XIICPS_IXR_RX_OVR_MASK;
	}else {
		ctrl_reg &= ~XZYNQ_XIICPS_CR_RD_WR_MASK;
	}
	enabled_intr |= XZYNQ_XIICPS_IXR_COMP_MASK | XZYNQ_XIICPS_IXR_ARB_LOST_MASK;

	out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_CR_OFFSET, ctrl_reg);

	/*
	 * Disable all the interrupts
	 */
	out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_IDR_OFFSET, XZYNQ_XIICPS_IXR_ALL_INTR_MASK);

	return 0;
}


int i2c_recv(uint8_t *buffer, int byte_count, uint16_t slave_addr)
{
	uint32_t intr_sts_reg;
	uint32_t intrs;
	uint32_t sts_reg;
	uint8_t *recv_buffer;
	int bytes_to_recv;
	int bytes_to_read;
	int trans_size;
	int tmp;

	i2c_setup_master(XZYNQ_ROLE_MASTER_RECEIVE);

	out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ADDR_OFFSET, slave_addr);

	/*
	 * intrs keeps all the error-related interrupts.
	 */
	intrs = XZYNQ_XIICPS_IXR_ARB_LOST_MASK | XZYNQ_XIICPS_IXR_RX_OVR_MASK
			| XZYNQ_XIICPS_IXR_RX_UNF_MASK | XZYNQ_XIICPS_IXR_TO_MASK
			| XZYNQ_XIICPS_IXR_NACK_MASK;

	/*
	 * Clear the interrupt status register before use it to monitor.
	 */
	intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET);
	out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET, intr_sts_reg);

	/*
	 * Set up the transfer size register so the slave knows how much
	 * to send to us.
	 */
	if (byte_count > XZYNQ_XIICPS_FIFO_DEPTH) {
		out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_TRANS_SIZE_OFFSET,
				XZYNQ_XIICPS_FIFO_DEPTH);
	} else {
		out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_TRANS_SIZE_OFFSET, byte_count);
	}

	/*
	 * Pull the interrupt status register to find the errors.
	 */
	intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET);
	recv_buffer = buffer;
	while ((byte_count > 0) && ((intr_sts_reg & intrs)
			== 0)) {
		sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_SR_OFFSET);

		/*
		 * If there is no data in the FIFO, check the interrupt
		 * status register for error, and continue.
		 */
		if ((sts_reg & XZYNQ_XIICPS_SR_RXDV_MASK) == 0) {
			intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET);
			continue;
		}

		/*
		 * The transfer size register shows how much more data slave
		 * needs to send to us.
		 */
		trans_size = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_TRANS_SIZE_OFFSET);

		bytes_to_read = byte_count;

		/*
		 * If expected number of bytes is greater than FIFO size,
		 * the master needs to wait for data comes in and set the
		 * transfer size register for slave to send more.
		 */
		if (byte_count > XZYNQ_XIICPS_FIFO_DEPTH) {
			/* wait slave to send data */
			while ((trans_size > 2)
					&& ((intr_sts_reg & intrs) == 0)) {
				trans_size = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_TRANS_SIZE_OFFSET);
				intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET);
			}

			/*
			 * If timeout happened, it is an error.
			 */
			if (intr_sts_reg & XZYNQ_XIICPS_IXR_TO_MASK) {
				return -1;
			}
			trans_size = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_TRANS_SIZE_OFFSET);

			/*
			 * Take trans size into account of how many more should
			 * be received.
			 */
			bytes_to_recv = byte_count - XZYNQ_XIICPS_FIFO_DEPTH + trans_size;

			/* Tell slave to send more to us */
			if (bytes_to_recv > XZYNQ_XIICPS_FIFO_DEPTH) {
				out32(XZYNQ_XIICPS1_BASE_ADDR +
						XZYNQ_XIICPS_TRANS_SIZE_OFFSET,
						XZYNQ_XIICPS_FIFO_DEPTH);
			} else {
				out32(XZYNQ_XIICPS1_BASE_ADDR +
						XZYNQ_XIICPS_TRANS_SIZE_OFFSET,
						bytes_to_recv);
			}

			bytes_to_read = XZYNQ_XIICPS_FIFO_DEPTH - trans_size;
		}

		tmp = 0;
		intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET);
		while ((tmp < bytes_to_read) && ((intr_sts_reg & intrs) == 0)) {
			sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_SR_OFFSET);
			intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR +
					XZYNQ_XIICPS_ISR_OFFSET);

			if ((sts_reg & XZYNQ_XIICPS_SR_RXDV_MASK) == 0) {
				/* No data in fifo */
				continue;
			}

			*recv_buffer++ = in8(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_DATA_OFFSET);
			byte_count--;

			tmp++;
		}
	}

	if (intr_sts_reg & intrs) {
		return -1;
	}

	return 0;
}

int i2c_send(uint8_t *buffer, int byte_count, uint16_t slave_addr)
{
	uint32_t intr_sts_reg;
	uint32_t sts_reg;
	uint32_t intrs;
	uint8_t *send_buffer;
	int byte_remaining;

	i2c_setup_master(XZYNQ_ROLE_MASTER_SEND);

	out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ADDR_OFFSET, slave_addr);

	/*
	 * intrs keeps all the error-related interrupts.
	 */
	intrs = XZYNQ_XIICPS_IXR_ARB_LOST_MASK | XZYNQ_XIICPS_IXR_TX_OVR_MASK |
			XZYNQ_XIICPS_IXR_TO_MASK | XZYNQ_XIICPS_IXR_NACK_MASK;

	/*
	 * Clear the interrupt status register before use it to monitor.
	 */
	intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET);
	out32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET, intr_sts_reg);

	/*
	 * Transmit first FIFO full of data.
	 */
	send_buffer = buffer;
	byte_remaining = i2c_transmit_fifo_fill(buffer, byte_count);
	buffer += (byte_count - byte_remaining);
			byte_count = byte_remaining;

	intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET);

	/*
	 * Continue sending as long as there is more data and
	 * there are no errors.
	 */
	while ((byte_count > 0) &&
		((intr_sts_reg & intrs) == 0)) {
		sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_SR_OFFSET);

		/*
		 * Wait until transmit FIFO is empty.
		 */
		if ((sts_reg & XZYNQ_XIICPS_SR_TXDV_MASK) != 0) {
			intr_sts_reg = in32(XZYNQ_XIICPS1_BASE_ADDR +
					XZYNQ_XIICPS_ISR_OFFSET);
			continue;
		}

		/*
		 * Send more data out through transmit FIFO.
		 */
		byte_remaining = i2c_transmit_fifo_fill(buffer, byte_count);
		buffer += (byte_count - byte_remaining);
		byte_count = byte_remaining;
	}

	/*
	 * Check for completion of transfer.
	 */
	while ((in32(XZYNQ_XIICPS1_BASE_ADDR + XZYNQ_XIICPS_ISR_OFFSET) &
			XZYNQ_XIICPS_IXR_COMP_MASK) != XZYNQ_XIICPS_IXR_COMP_MASK);

	/*
	 * If there is an error, tell the caller.
	 */
	if (intr_sts_reg & intrs) {
		return -1;
	}

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
