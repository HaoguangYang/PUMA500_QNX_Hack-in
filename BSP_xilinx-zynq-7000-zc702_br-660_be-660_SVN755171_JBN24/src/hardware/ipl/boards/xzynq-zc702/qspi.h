/*
 * $QNXLicenseC:
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

#ifndef _QSPI_H_
#define _QSPI_H_

/*
 * QSPI Mapping
 */
/* BOOT.BIN */
#define XZYNQ_QSPI_BOOT_OFFSET		0x00000000
#define XZYNQ_QSPI_BOOT_SIZE		0x00100000
/* QNX-IFS */
#define XZYNQ_QSPI_IFS_OFFSET		0x00100000
#define XZYNQ_QSPI_IFS_LEN_OFFSET	0x0010002c
#define XZYNQ_QSPI_IFS_LEN_PAD		0x00000008
#define XZYNQ_QSPI_IFS_SIZE			0x00700000
/* User partition */
#define XZYNQ_QSPI_USER_OFFSET		0x00800000
#define XZYNQ_QSPI_USER_SIZE		0x00800000

/* Registers base address */
#define XZYNQ_QSPI_REG_ADDRESS		0xE000D000

/* Mapped linear addresses for linear mode */
#define XZYNQ_QSPI_LINEAR_ADDRESS	0xFC000000
#define XZYNQ_QSPI_LINEAR_SIZE		0x02000000

/* Registers offset */
#define XZYNQ_QSPI_CR_OFFSET		0x00 /**< 32-bit Control */
#define XZYNQ_QSPI_ER_OFFSET		0x14 /**< Enable/Disable Register */
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

/* Auto configuration value */
#define XZYNQ_QSPI_CR_SSCTRL_DEFAULT 0x00003800
#define XZYNQ_QSPI_CR_AUTO		(XZYNQ_QSPI_CR_IFMODE_MASK | \
								(1 << XZYNQ_QSPI_CR_SSCTRL_SHIFT) | \
								XZYNQ_QSPI_CR_DATA_SZ_MASK | \
								XZYNQ_QSPI_CR_MSTREN_MASK)

/* Enable Register */
#define XZYNQ_QSPI_ER_ENABLE_MASK	0x00000001 /**< QSPI Enable Bit Mask */

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

int qspi_init(void);
int qspi_get_ifs(unsigned address);

#endif /* #ifndef _QSPI_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
