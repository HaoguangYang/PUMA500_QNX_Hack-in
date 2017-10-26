/*
 * $QNXLicenseC:
 * Copyright 2012, QNX Software Systems.
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

#ifndef __XZYNQ_QSPI_H_INCLUDED
#define __XZYNQ_QSPI_H_INCLUDED

#include <stdint.h>

/* Registers base address */
#define XZYNQ_QSPI_REG_ADDRESS		0xE000D000
#define XZYNQ_QSPI_REG_SIZE			0x100

/* QSPI modes */
#define XZYNQ_QSPI_LINEAR_MODE		1
#define XZYNQ_QSPI_IO_MODE			2

/* Controller clock frequency */
#define XZYNQ_QSPI_MAX_DRATE		100000000

/* Slave select */
#define XZYNQ_QSPI_DEFAULT_SS		0

/* Interrupt number */
#define XZYNQ_QSPI_IRQ				51

/* Messages variables */
#define XZYNQ_QSPI_EVENT			1
#define XZYNQ_QSPI_PRIORITY			21

/* FIFO */
#define XZYNQ_QSPI_FIFO_WIDTH		4
#define XZYNQ_QSPI_FIFO_DEPTH		62

/* Mapped linear addresses for linear mode */
#define XZYNQ_QSPI_LINEAR_ADDRESS	0xFC000000
#define XZYNQ_QSPI_LINEAR_SIZE		0x02000000

/* Registers offset */
#define XZYNQ_QSPI_CR_OFFSET		0x00 /**< 32-bit Control */
#define XZYNQ_QSPI_ISR_OFFSET		0x04 /**< Interrupt Status */
#define XZYNQ_QSPI_IER_OFFSET		0x08 /**< Interrupt Enable */
#define XZYNQ_QSPI_IDR_OFFSET		0x0c /**< Interrupt Disable */
#define XZYNQ_QSPI_IMR_OFFSET		0x10 /**< Interrupt Enabled Mask */
#define XZYNQ_QSPI_ER_OFFSET		0x14 /**< Enable/Disable Register */
#define XZYNQ_QSPI_DR_OFFSET		0x18 /**< Delay Register */
#define XZYNQ_QSPI_TXD_00_OFFSET	0x1C /**< Transmit 4-byte inst/data */
#define XZYNQ_QSPI_RXD_OFFSET		0x20 /**< Data Receive Register */
#define XZYNQ_QSPI_SICR_OFFSET		0x24 /**< Slave Idle Count */
#define XZYNQ_QSPI_TXWR_OFFSET		0x28 /**< Transmit FIFO Watermark */
#define XZYNQ_QSPI_RXWR_OFFSET		0x2C /**< Receive FIFO Watermark */
#define XZYNQ_QSPI_TXD_01_OFFSET	0x80 /**< Transmit 1-byte inst */
#define XZYNQ_QSPI_TXD_10_OFFSET	0x84 /**< Transmit 2-byte inst */
#define XZYNQ_QSPI_TXD_11_OFFSET	0x88 /**< Transmit 3-byte inst */
#define XZYNQ_QSPI_LQSPI_CR_OFFSET	0xA0 /**< Linear QSPI config register */
#define XZYNQ_QSPI_LQSPI_SR_OFFSET	0xA4 /**< Linear QSPI status register */

/* Control register */
#define XZYNQ_QSPI_CR_IFMODE_MASK    0x80000000 /**< Flash mem interface mode */
#define XZYNQ_QSPI_CR_ENDIAN_MASK    0x04000000 /**< Tx/Rx FIFO endianness */
#define XZYNQ_QSPI_CR_MANSTRT_MASK   0x00010000 /**< Manual Transmission Start */
#define XZYNQ_QSPI_CR_MANSTRTEN_MASK 0x00008000 /**< Manual Transmission Start Enable */
#define XZYNQ_QSPI_CR_SSFORCE_MASK   0x00004000 /**< Force Slave Select */
#define XZYNQ_QSPI_CR_SSCTRL_MASK    0x00003C00 /**< Slave Select Decode */
#define XZYNQ_QSPI_CR_SSCTRL_SHIFT   10			/**< Slave Select Decode shift */
#define XZYNQ_QSPI_CR_SSCTRL_MAXIMUM 0x03		/**< Slave Select maximum value */
#define XZYNQ_QSPI_CR_SSDECEN_MASK   0x00000200 /**< Slave Select Decode Enable */
#define XZYNQ_QSPI_CR_DATA_SZ_MASK   0x000000C0 /**< Size of word to be transferred */
#define XZYNQ_QSPI_CR_PRESC_MASK     0x00000038 /**< Prescaler Setting */
#define XZYNQ_QSPI_CR_PRESC_SHIFT    3			/**< Prescaler shift */
#define XZYNQ_QSPI_CR_PRESC_MAXIMUM  0x07		/**< Prescaler maximum value */
#define XZYNQ_QSPI_CR_CPHA_MASK      0x00000004 /**< Phase Configuration */
#define XZYNQ_QSPI_CR_CPOL_MASK      0x00000002 /**< Polarity Configuration */
#define XZYNQ_QSPI_CR_MSTREN_MASK    0x00000001 /**< Master Mode Enable */

/* Deselect all the SS lines and set the transfer size to 32 at reset */
#define XZYNQ_QSPI_CR_RESET_STATE    (XZYNQ_QSPI_CR_IFMODE_MASK | \
				    XZYNQ_QSPI_CR_SSCTRL_MASK | \
				    XZYNQ_QSPI_CR_DATA_SZ_MASK | \
				    XZYNQ_QSPI_CR_MSTREN_MASK)

/* Interrupt Registers */
#define XZYNQ_QSPI_IXR_TXUF_MASK		0x00000040  /**< QSPI Tx FIFO Underflow */
#define XZYNQ_QSPI_IXR_RXFULL_MASK		0x00000020  /**< QSPI Rx FIFO Full */
#define XZYNQ_QSPI_IXR_RXNEMPTY_MASK	0x00000010  /**< QSPI Rx FIFO Not Empty */
#define XZYNQ_QSPI_IXR_TXFULL_MASK		0x00000008  /**< QSPI Tx FIFO Full */
#define XZYNQ_QSPI_IXR_TXNFULL_MASK		0x00000004  /**< QSPI Tx FIFO Overwater */
#define XZYNQ_QSPI_IXR_MODF_MASK		0x00000002  /**< QSPI Mode Fault */
#define XZYNQ_QSPI_IXR_RXOVR_MASK		0x00000001  /**< QSPI Rx FIFO Overrun */
#define XZYNQ_QSPI_IXR_DFLT_MASK		(XZYNQ_QSPI_IXR_TXNFULL_MASK | XZYNQ_QSPI_IXR_MODF_MASK)
#define XZYNQ_QSPI_ISR_ALL_MASK			0x0000007F

/* Enable Register */
#define XZYNQ_QSPI_ER_ENABLE_MASK	0x00000001 /**< QSPI Enable Bit Mask */

/* Delay Register */
#define XZYNQ_QSPI_DR_BTWN_MASK		0x00FF0000	/**< Delay Between Transfers mask */
#define XZYNQ_QSPI_DR_BTWN_SHIFT	16			/**< Delay Between Transfers shift */
#define XZYNQ_QSPI_DR_AFTER_MASK	0x0000FF00	/**< Delay After Transfers mask */
#define XZYNQ_QSPI_DR_AFTER_SHIFT	8			/**< Delay After Transfers shift */
#define XZYNQ_QSPI_DR_INIT_MASK		0x000000FF	/**< Delay Initially mask */

/* Slave Idle Count Registers */
#define XZYNQ_QSPI_SICR_MASK		0x000000FF	/**< Slave Idle Count Mask */

/* Transmit FIFO Watermark Register */
#define XZYNQ_QSPI_TXWR_MASK		0x0000003F	/**< Transmit Watermark Mask */

/* Linear QSPI Configuration Register */
#define XZYNQ_QSPI_LQSPI_CR_LINEAR_MASK		0x80000000 /**< LQSPI mode enable */
#define XZYNQ_QSPI_LQSPI_CR_TWO_MEM_MASK	0x40000000 /**< Both memories or one */
#define XZYNQ_QSPI_LQSPI_CR_SEP_BUS_MASK	0x20000000 /**< Seperate memory bus */
#define XZYNQ_QSPI_LQSPI_CR_U_PAGE_MASK		0x10000000 /**< Upper memory page */
#define XZYNQ_QSPI_LQSPI_CR_CMD_MERGE_MASK	0x04000000 /**< Merge back to back AXI commands */
#define XZYNQ_QSPI_LQSPI_CR_MODE_EN_MASK	0x02000000 /**< Enable mode bits */
#define XZYNQ_QSPI_LQSPI_CR_MODE_ON_MASK	0x01000000 /**< Mode on */
#define XZYNQ_QSPI_LQSPI_CR_MODE_BITS_MASK	0x00FF0000 /**< Mode value for dual I/O or quad I/O */
#define XZYNQ_QSPI_LQSPI_CR_RD_ZEROS_MASK	0x00000800 /**< Zero out all read data */
#define XZYNQ_QSPI_LQSPI_CR_DUMMY_MASK		0x00000700 /**< Number of dummy bytes between addr and return read data */
#define XZYNQ_QSPI_LQSPI_CR_INST_MASK		0x000000FF /**< Read instr code */
#define XZYNQ_QSPI_LQSPI_CR_RST_STATE		0x8400016B /**< Default CR value */

/* Linear QSPI Status Register */
#define XZYNQ_QSPI_LQSPI_SR_CMD_MERGED_MASK	0x00000100 /**< AXI read commands have been merged */
#define XZYNQ_QSPI_LQSPI_SR_FB_RECVD_MASK	0x00000004 /**< AXI fixed burst command received */
#define XZYNQ_QSPI_LQSPI_SR_WR_RECVD_MASK	0x00000002 /**< AXI write command received */
#define XZYNQ_QSPI_LQSPI_SR_UNKN_INST_MASK	0x00000001 /**< Unknown read inst code */

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

typedef struct {
	uint8_t opcode;		/**< Operational code of the instruction */
	uint8_t inst_size;	/**< size of the instruction including address bytes */
	uint8_t tx_offset;	/**< Register address where instruction has to be written */
} xzynq_qspi_inst_t;

typedef struct {
	unsigned	pbase;
	uintptr_t	vbase;
	int			irq;
	int			iid;
	int			chid, coid;
	uint32_t	ctrl;
	uint32_t	bitrate;
	uint32_t	clock;
	int			dlen;
	int			dtime;	/* usec per data, for time out use */
	int			busy;
	uint8_t		*send_buf;
	uint8_t		*recv_buf;
	int			requested_bytes;
	int			remaining_bytes;
	int			slave_select;
	int			is_inst;
	struct sigevent qspievent;
	xzynq_qspi_inst_t *current_inst;
} xzynq_qspi_t;

int xzynq_qspi_setcfg(int fd, int mode, int drate, int cs);
int xzynq_qspi_cmd_read(int fd, uint8_t cmd[4], uint8_t* buf, int len);
int xzynq_qspi_cmd_write(int fd, uint8_t cmd[4], uint8_t const* buf, int len);
int xzynq_qspi_word_exchange(int fd, uint32_t* buf);
int xzynq_qspi_write_1byte(int fd, uint8_t cmd);
int xzynq_qspi_open(void);
int xzynq_qspi_close(int fd);

#endif /* __XZYNQ_QSPI_H_INCLUDED */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
