/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems.
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

#ifndef __PROTO_H_INCLUDED
#define __PROTO_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <hw/i2c.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <arm/xzynq.h>

typedef struct _xzynq_dev {

	unsigned reglen;
	uintptr_t regbase;
	unsigned physbase;

	unsigned slave_addr;

	int direction;
	int intr;
	int iid;
	struct sigevent intrevent;
	i2c_status_t status;
	int stop;

	uint8_t *buf;
	unsigned size_to_send;
	unsigned size_to_receive;

	unsigned speed;
	unsigned bus_freq;

	unsigned options;
} xzynq_dev_t;

#define XZYNQ_ROLE_MASTER_SEND   	0x00000001
#define XZYNQ_ROLE_MASTER_RECEIVE   0x00000002

#define XZYNQ_OPT_VERBOSE        0x00000002
#define XZYNQ_FIFO_DEPTH         16

/************************** Constant Definitions *****************************/

/** @name Register Map
 *
 * Register offsets for the IIC.
 * @{
 */
#define XZYNQ_XIICPS_CR_OFFSET			0x00  /**< 32-bit Control */
#define XZYNQ_XIICPS_SR_OFFSET			0x04  /**< Status */
#define XZYNQ_XIICPS_ADDR_OFFSET		0x08  /**< IIC Address */
#define XZYNQ_XIICPS_DATA_OFFSET		0x0C  /**< IIC FIFO Data */
#define XZYNQ_XIICPS_ISR_OFFSET			0x10  /**< Interrupt Status */
#define XZYNQ_XIICPS_TRANS_SIZE_OFFSET	0x14  /**< Transfer Size */
#define XZYNQ_XIICPS_SLV_PAUSE_OFFSET	0x18  /**< Slave monitor pause */
#define XZYNQ_XIICPS_TIME_OUT_OFFSET	0x1C  /**< Time Out */
#define XZYNQ_XIICPS_IMR_OFFSET			0x20  /**< Interrupt Enabled Mask */
#define XZYNQ_XIICPS_IER_OFFSET			0x24  /**< Interrupt Enable */
#define XZYNQ_XIICPS_IDR_OFFSET			0x28  /**< Interrupt Disable */
/* @} */

/** @name Control Register
 *
 * This register contains various control bits that
 * affects the operation of the IIC controller. Read/Write.
 * @{
 */
#define XZYNQ_XIICPS_CR_DIV_A_MASK		0x0000C000 /**< Clock Divisor A */
#define XZYNQ_XIICPS_CR_DIV_A_SHIFT		14 			/**< Clock Divisor A shift */
#define XZYNQ_XIICPS_DIV_A_MAX			4 			/**< Maximum value of Divisor A */
#define XZYNQ_XIICPS_CR_DIV_B_MASK		0x00003F00 /**< Clock Divisor B */
#define XZYNQ_XIICPS_CR_DIV_B_SHIFT		8  			/**< Clock Divisor B shift */
#define XZYNQ_XIICPS_CR_CLR_FIFO_MASK	0x00000040 /**< Clear FIFO, auto clears*/
#define XZYNQ_XIICPS_CR_SLVMON_MASK		0x00000020 /**< Slave monitor mode */
#define XZYNQ_XIICPS_CR_HOLD_MASK		0x00000010 /**<  Hold bus 1=Hold scl,
														 0=terminate transfer */
#define XZYNQ_XIICPS_CR_ACKEN_MASK		0x00000008  /**< Enable TX of ACK when
						 	 	 	 	 	 	 	 	 Master receiver*/
#define XZYNQ_XIICPS_CR_NEA_MASK		0x00000004  /**< Addressing Mode 1=7 bit,
						 	 	 	 	 	 	 	 	 0=10 bit */
#define XZYNQ_XIICPS_CR_MS_MASK			0x00000002  /**< Master mode bit 1=Master,
						 	 	 	 	 	 	 	 	 0=Slave */
#define XZYNQ_XIICPS_CR_RD_WR_MASK		0x00000001  /**< Read or Write Master
						 	 	 	 	 	 	 	 	 transfer  0=Transmitter,
						 	 	 	 	 	 	 	 	 1=Receiver*/
#define XZYNQ_XIICPS_CR_RESET_VALUE		0   /**< Reset value of the Control
						 	 	 	 	 	 	 register */

/* @} */

/** @name IIC Status Register
 *
 * This register is used to indicate status of the IIC controller. Read only
 * @{
 */
#define XZYNQ_XIICPS_SR_BA_MASK	0x00000100  /**< Bus Active Mask */
#define XZYNQ_XIICPS_SR_RXOVF_MASK	0x00000080  /**< Receiver Overflow Mask */
#define XZYNQ_XIICPS_SR_TXDV_MASK	0x00000040  /**< Transmit Data Valid Mask */
#define XZYNQ_XIICPS_SR_RXDV_MASK	0x00000020  /**< Receiver Data Valid Mask */
#define XZYNQ_XIICPS_SR_RXRW_MASK	0x00000008  /**< Receive read/write Mask */
/* @} */

/** @name IIC Address Register
 *
 * Normal addressing mode uses add[6:0]. Extended addressing mode uses add[9:0].
 * A write access to this register always initiates a transfer if the IIC is in
 * master mode. Read/Write
 * @{
 */
#define XZYNQ_XIICPS_ADDR_MASK	0x000003FF  /**< IIC Address Mask */
/* @} */

/** @name IIC Data Register
 *
 * When written to, the data register sets data to transmit. When read from, the
 * data register reads the last received byte of data. Read/Write
 * @{
 */
#define XZYNQ_XIICPS_DATA_MASK	0x000000FF  /**< IIC Data Mask */
/* @} */

/** @name IIC Interrupt Registers
 *
 * <b>IIC Interrupt Status Register</b>
 *
 * This register holds the interrupt status flags for the IIC controller. Some
 * of the flags are level triggered
 * - i.e. are set as long as the interrupt condition exists.  Other flags are
 *   edge triggered, which means they are set one the interrupt condition occurs
 *   then remain set until they are cleared by software.
 *   The interrupts are cleared by writing a one to the interrupt bit position
 *   in the Interrupt Status Register. Read/Write.
 *
 * <b>IIC Interrupt Enable Register</b>
 *
 * This register is used to enable interrupt sources for the IIC controller.
 * Writing a '1' to a bit in this register clears the corresponding bit in the
 * IIC Interrupt Mask register.  Write only.
 *
 * <b>IIC Interrupt Disable Register </b>
 *
 * This register is used to disable interrupt sources for the IIC controller.
 * Writing a '1' to a bit in this register sets the corresponding bit in the
 * IIC Interrupt Mask register. Write only.
 *
 * <b>IIC Interrupt Mask Register</b>
 *
 * This register shows the enabled/disabled status of each IIC controller
 * interrupt source. A bit set to 1 will ignore the corresponding interrupt in
 * the status register. A bit set to 0 means the interrupt is enabled.
 * All mask bits are set and all interrupts are disabled after reset. Read only.
 *
 * All four registers have the same bit definitions. They are only defined once
 * for each of the Interrupt Enable Register, Interrupt Disable Register,
 * Interrupt Mask Register, and Interrupt Status Register
 * @{
 */

#define XZYNQ_XIICPS_IXR_ARB_LOST_MASK  0x00000200	 /**< Arbitration Lost Interrupt
						   mask */
#define XZYNQ_XIICPS_IXR_RX_UNF_MASK    0x00000080	 /**< FIFO Recieve Underflow
						   Interrupt mask */
#define XZYNQ_XIICPS_IXR_TX_OVR_MASK    0x00000040	 /**< Transmit Overflow
						   Interrupt mask */
#define XZYNQ_XIICPS_IXR_RX_OVR_MASK    0x00000020	 /**< Receive Overflow Interrupt
						   mask */
#define XZYNQ_XIICPS_IXR_SLV_RDY_MASK   0x00000010	 /**< Monitored Slave Ready
						   Interrupt mask */
#define XZYNQ_XIICPS_IXR_TO_MASK        0x00000008	 /**< Transfer Time Out
						   Interrupt mask */
#define XZYNQ_XIICPS_IXR_NACK_MASK      0x00000004	 /**< NACK Interrupt mask */
#define XZYNQ_XIICPS_IXR_DATA_MASK      0x00000002	 /**< Data Interrupt mask */
#define XZYNQ_XIICPS_IXR_COMP_MASK      0x00000001	 /**< Transfer Complete
						   Interrupt mask */
#define XZYNQ_XIICPS_IXR_DEFAULT_MASK   0x000002FF	 /**< Default ISR Mask */
#define XZYNQ_XIICPS_IXR_ALL_INTR_MASK  0x000002FF	 /**< All ISR Mask */
/* @} */

/** @name IIC Transfer Size Register
 *
 * The register's meaning varies according to the operating mode as follows:
 *   - Master transmitter mode: number of data bytes still not transmitted minus
 *     one
 *   - Master receiver mode: number of data bytes that are still expected to be
 *     received
 *   - Slave transmitter mode: number of bytes remaining in the FIFO after the
 *     master terminates the transfer
 *   - Slave receiver mode: number of valid data bytes in the FIFO
 *
 * This register is cleared if CLR_FIFO bit in the control register is set.
 * Read/Write
 * @{
 */
#define XZYNQ_XIICPS_TRANS_SIZE_MASK  0x0000003F /**< IIC Transfer Size Mask */
#define XZYNQ_XIICPS_FIFO_DEPTH          16	  /**< Number of bytes in the FIFO */
#define XZYNQ_XIICPS_DATA_INTR_DEPTH     14    /**< Number of bytes at DATA intr */
/* @} */

/** @name IIC Slave Monitor Pause Register
 *
 * This register is associated with the slave monitor mode of the I2C interface.
 * It is meaningful only when the module is in master mode and bit SLVMON in the
 * control register is set.
 *
 * This register defines the pause interval between consecutive attempts to
 * address the slave once a write to an I2C address register is done by the
 * host. It represents the number of sclk cycles minus one between two attempts.
 *
 * The reset value of the register is 0, which results in the master repeatedly
 * trying to access the slave immediately after unsuccessful attempt.
 * Read/Write
 * @{
 */
#define XZYNQ_XIICPS_SLV_PAUSE_MASK    0x0000000F  /**< Slave monitor pause mask */
/* @} */

/** @name IIC Time Out Register
 *
 * The value of time out register represents the time out interval in number of
 * sclk cycles minus one.
 *
 * When the accessed slave holds the sclk line low for longer than the time out
 * period, thus prohibiting the I2C interface in master mode to complete the
 * current transfer, an interrupt is generated and TO interrupt flag is set.
 *
 * The reset value of the register is 0x1f.
 * Read/Write
 * @{
 */
#define XZYNQ_XIICPS_TIME_OUT_MASK    0x000000FF    /**< IIC Time Out mask */
#define XZYNQ_XIICPS_TO_RESET_VALUE   0x0000001F    /**< IIC Time Out reset value */
/* @} */

void *xzynq_init(int argc, char *argv[]);
void xzynq_fini(void *hdl);
int xzynq_options(xzynq_dev_t *dev, int argc, char *argv[]);

const struct sigevent *xzynq_i2c_intr(void *area, int id);
int xzynq_set_slave_addr(void *hdl, unsigned int addr, i2c_addrfmt_t fmt);
int xzynq_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed);
int xzynq_version_info(i2c_libversion_t *version);
int xzynq_driver_info(void *hdl, i2c_driver_info_t *info);
int xzynq_devctl(void *hdl, int cmd, void *msg, int msglen, int *nbytes,
		int *info);
i2c_status_t xzynq_recv(void *hdl, void *buf, unsigned int len,
		unsigned int stop);
i2c_status_t xzynq_send(void *hdl, void *buf, unsigned int len,
		unsigned int stop);
uint32_t xzynq_wait_bus_not_busy(xzynq_dev_t *dev);
const struct sigevent *xzynq_intr(void *area, int id);
void xzynq_wait_status(xzynq_dev_t *dev);
i2c_status_t xzynq_sendbyte(xzynq_dev_t *dev, uint8_t byte);
i2c_status_t
		xzynq_recvbyte(xzynq_dev_t *dev, uint8_t *byte, int nack, int stop);
void xzynq_reset(xzynq_dev_t* dev);
i2c_status_t setup_master(xzynq_dev_t *dev, uint8_t role, unsigned int len,
		unsigned int stop);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/proto.h $ $Rev: 752035 $")
#endif
