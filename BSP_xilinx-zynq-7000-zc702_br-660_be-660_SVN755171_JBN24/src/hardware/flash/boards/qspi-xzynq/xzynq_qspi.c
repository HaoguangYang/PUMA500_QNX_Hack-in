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

#include <sys/slog.h>
#include "variant.h"
#include "xzynq_qspi.h"
#include "qspi_cmds.h"

/* open() ... */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hw/clk.h>

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
	ctrl_id_t id = XZYNQ_QSPI_CTRL;

	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("open");
		slogf(_SLOGC_CONSOLE, _SLOG_INFO,
			"qspi: %s can't open /dev/clock", __func__);
		return;
	}

	err = devctl(fd, DCMD_CLOCK_DISABLE, &id, sizeof(clk_id_t), NULL);
	if (err) {
		slogf(_SLOGC_CONSOLE, _SLOG_INFO,
			"qspi: %s can't disable the clock", __func__);
	}

	close(fd);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_pm_resume(void* arg)
{
	int fd, err;
	ctrl_id_t id = XZYNQ_QSPI_CTRL;

	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("open");
		slogf(_SLOGC_CONSOLE, _SLOG_INFO,
			"qspi: %s can't open /dev/clock", __func__);
		return;
	}

	err = devctl(fd, DCMD_CLOCK_ENABLE, &id, sizeof(clk_id_t), NULL);
	if (err) {
		slogf(_SLOGC_CONSOLE, _SLOG_INFO,
			"qspi: %s can't disable the clock", __func__);
	}

	close(fd);
}

#endif

static xzynq_qspi_inst_t flash_inst[] = {
	{ FLASH_OPCODE_WREN,		1, XZYNQ_QSPI_TXD_01_OFFSET },
	{ FLASH_OPCODE_WRDS,		1, XZYNQ_QSPI_TXD_01_OFFSET },
	{ FLASH_OPCODE_RDSR1,		4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_RDSR2,		4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_WRSR,		2, XZYNQ_QSPI_TXD_10_OFFSET },
	{ FLASH_OPCODE_PP,			4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_SE,			4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_BE_32K,		4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_BE_4K,		4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_BE,			1, XZYNQ_QSPI_TXD_01_OFFSET },
	{ FLASH_OPCODE_ERASE_SUS,	1, XZYNQ_QSPI_TXD_01_OFFSET },
	{ FLASH_OPCODE_ERASE_RES,	1, XZYNQ_QSPI_TXD_01_OFFSET },
	{ FLASH_OPCODE_RDID,		4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_NORM_READ,	4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_FAST_READ,	4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_DUAL_READ,	4, XZYNQ_QSPI_TXD_00_OFFSET },
	{ FLASH_OPCODE_QUAD_READ,	4, XZYNQ_QSPI_TXD_00_OFFSET },
	/* Add all the instructions supported by the flash device */
};

static void xzynq_qspi_slave_select(xzynq_qspi_t *dev, int on)
{
	int ctrl = dev->ctrl;

	if (on) {
		ctrl &= ~(XZYNQ_QSPI_CR_SSCTRL_MASK);
		ctrl |= (((1<<dev->slave_select)<<XZYNQ_QSPI_CR_SSCTRL_SHIFT) &
				XZYNQ_QSPI_CR_SSCTRL_MASK);
	} else {
		ctrl &= ~(XZYNQ_QSPI_CR_SSCTRL_MASK);
	}

	/* Slave select value will be written along the start condition */
	dev->ctrl = ctrl;
}

static uint32_t xzynq_qspi_setup_exchange(xzynq_qspi_t *dev, int len)
{
	if (len > XZYNQ_QSPI_FIFO_DEPTH*XZYNQ_QSPI_FIFO_WIDTH)
		return -1;

	/* Select slave */
	xzynq_qspi_slave_select(dev, 1);

	/* Enable QSPI */
	out32(dev->vbase + XZYNQ_QSPI_ER_OFFSET, XZYNQ_QSPI_ER_ENABLE_MASK);

	return 0;
}

static void xzynq_qspi_start_exchange(xzynq_qspi_t *dev)
{
	if (dev->ctrl & XZYNQ_QSPI_CR_MANSTRTEN_MASK)
		/* Manual start command */
		out32(dev->vbase + XZYNQ_QSPI_CR_OFFSET, dev->ctrl | XZYNQ_QSPI_CR_MANSTRT_MASK);
}

static void xzynq_qspi_end_exchange(xzynq_qspi_t *dev)
{
	/* Disable QSPI */
	out32(dev->vbase + XZYNQ_QSPI_ER_OFFSET, ~XZYNQ_QSPI_ER_ENABLE_MASK);

	/* Unselect slave */
	xzynq_qspi_slave_select(dev, 0);
}

static void xzynq_qspi_get_write_data(xzynq_qspi_t *dev, uint32_t *data,
		uint8_t size)
{
	if (dev->send_buf) {
		switch (size) {
		case 1:
			*data = *((uint8_t *) dev->send_buf);
			dev->send_buf += 1;
			*data |= 0xFFFFFF00;
			break;
		case 2:
			*data = *((uint16_t *)dev->send_buf);
			dev->send_buf += 2;
			*data |= 0xFFFF0000;
			break;
		case 3:
			*data = *((uint16_t *)dev->send_buf);
			dev->send_buf += 2;
			*data |= (*((uint8_t *)dev->send_buf) << 16);
			dev->send_buf += 1;
			*data |= 0xFF000000;
			break;
		case 4:
			*data = *((uint32_t *)dev->send_buf);
			dev->send_buf += 4;
			break;
		default:
			/* This will never execute */
			break;
		}
	} else
		*data = 0;

	dev->remaining_bytes -= size;
	if (dev->remaining_bytes < 0)
		dev->remaining_bytes = 0;
}

static void xzynq_qspi_get_read_data(xzynq_qspi_t *dev, uint32_t data,
		uint8_t size)
{
	if (dev->recv_buf) {
		switch (size) {
		case 1 ... 4:
			memcpy(dev->recv_buf, ((uint8_t*)&data), size);
			dev->recv_buf += size;
			break;
		default:
			/* This will never execute */
			break;
		}
	}

	dev->requested_bytes -= size;
	if (dev->requested_bytes < 0)
		dev->requested_bytes = 0;
}

static const struct sigevent *xzynq_qspi_intr(void *area, int id)
{
	xzynq_qspi_t *dev = area;
	uint32_t data;
	uint32_t dummy_byte = 0;
	uint32_t reg = in32(dev->vbase + XZYNQ_QSPI_ISR_OFFSET);

	/* Clear all interrupts */
	out32(dev->vbase + XZYNQ_QSPI_ISR_OFFSET, reg);
	/* Disable interrupts for now */
	out32(dev->vbase + XZYNQ_QSPI_IDR_OFFSET, XZYNQ_QSPI_ISR_ALL_MASK);

	/*
	 * If a fault happened return message as requested bytes count
	 * will not be 0
	 */
	if (reg & XZYNQ_QSPI_IXR_MODF_MASK)
		return (&dev->qspievent);

	if (reg & (XZYNQ_QSPI_IXR_TXNFULL_MASK | XZYNQ_QSPI_IXR_RXNEMPTY_MASK)) {
		/* Empty the Rx FIFO */
		while ((in32(dev->vbase + XZYNQ_QSPI_ISR_OFFSET)
				& XZYNQ_QSPI_IXR_RXNEMPTY_MASK) && dev->requested_bytes) {
			data = in32(dev->vbase + XZYNQ_QSPI_RXD_OFFSET);

			if (dev->is_inst) {
				dev->is_inst = 0;
				/* Discard first word if dummy (not a response) */
				switch (dev->current_inst->opcode) {
				case FLASH_OPCODE_RDSR1:
				case FLASH_OPCODE_RDSR2:
				case FLASH_OPCODE_RDID:
					break;
				case FLASH_OPCODE_FAST_READ:
				case FLASH_OPCODE_DUAL_READ:
				case FLASH_OPCODE_QUAD_READ:
					/* For those commands, skip 1 extra dummy byte */
					if (!dummy_byte) {
						dev->is_inst = 1;
						dummy_byte = 1;
						continue;
					} else {
						memcpy(dev->recv_buf, ((uint8_t*)&data)+1, 3);
						dev->recv_buf += 3;
						dev->requested_bytes -= 3;
					}
				default:
					dev->requested_bytes -= dev->current_inst->inst_size;
					continue;
				}
			}

			if (dev->requested_bytes < sizeof(uint32_t))
				xzynq_qspi_get_read_data(dev, data, dev->requested_bytes);
			else
				xzynq_qspi_get_read_data(dev, data, sizeof(uint32_t));
		}

		/* If some data still needs to be sent */
		if (dev->remaining_bytes) {
			while ((dev->remaining_bytes > 0) && !(in32(dev->vbase +
					XZYNQ_QSPI_ISR_OFFSET)&XZYNQ_QSPI_IXR_TXFULL_MASK)) {
				if (dev->remaining_bytes < sizeof(uint32_t))
					xzynq_qspi_get_write_data(dev, &data, dev->remaining_bytes);
				else
					xzynq_qspi_get_write_data(dev, &data, sizeof(uint32_t));

				out32(dev->vbase + XZYNQ_QSPI_TXD_00_OFFSET, data);
			}
			xzynq_qspi_start_exchange(dev);
		}

		/* Is the transfer complete? */
		if (dev->requested_bytes)
			out32(dev->vbase + XZYNQ_QSPI_IER_OFFSET, XZYNQ_QSPI_IXR_RXNEMPTY_MASK);

		return (&dev->qspievent);
	}

	return NULL;
}

static int xzynq_qspi_wait(xzynq_qspi_t const* dev, int len)
{
	struct _pulse pulse;

	while (1) {
		if (len) {
			uint64_t to = dev->dtime;
			to *= len * 1000 * 50; /* 50 times for timeout */
			TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE, NULL, &to, NULL);
		}

		if (MsgReceivePulse(dev->chid, &pulse, sizeof(pulse), NULL) == -1) {
			fprintf(stderr, "norqspi: timeout xfer of %d bytes\n", len);
			errno = EIO;
			return -1;
		}

		if (pulse.code == XZYNQ_QSPI_EVENT)
			return 0;
	}

	return 0;
}

int xzynq_qspi_setcfg(int fd, int mode, int drate, int cs)
{
	xzynq_qspi_t *dev = (xzynq_qspi_t*)fd;
	uint32_t ctrl = 0, i, freq;

	/* Check mode consistency */
	if (mode != XZYNQ_QSPI_IO_MODE) {
		fprintf(stderr, "norqspi: f3s support implies the use of IO mode\n");
		return EINVAL;
	}

	/* Convert divisor value */
	for (i=0; i<=7; i++) {
		freq = dev->clock / (2<<i);
		if (freq <= drate) {
			break;
		}
	}
	ctrl |= (i << XZYNQ_QSPI_CR_PRESC_SHIFT);

	/* Set memory interface */
	ctrl |= XZYNQ_QSPI_CR_IFMODE_MASK;

	/* Slave select decode */
	ctrl |= XZYNQ_QSPI_CR_SSCTRL_MASK;
	dev->slave_select = cs;

	/* Select manual start mode */
	ctrl |= XZYNQ_QSPI_CR_MANSTRTEN_MASK;
	ctrl |= XZYNQ_QSPI_CR_SSFORCE_MASK;

	/* size of the word to decode to 32bits */
	ctrl |= XZYNQ_QSPI_CR_DATA_SZ_MASK;
	dev->dlen = 4;

	/* Enable Master mode */
	ctrl |= XZYNQ_QSPI_CR_MSTREN_MASK;

	/* Set phase and polarity */
	ctrl |= XZYNQ_QSPI_CR_CPHA_MASK;
	ctrl |= XZYNQ_QSPI_CR_CPOL_MASK;

	/* Set thresholds */
	out32(dev->vbase + XZYNQ_QSPI_TXWR_OFFSET, 1); // For TX empty
	out32(dev->vbase + XZYNQ_QSPI_RXWR_OFFSET, 1); // For RX not empty

	dev->ctrl = ctrl;
	dev->dtime = dev->dlen * 8 * 1000 * 1000 / drate;
	if (dev->dtime == 0)
		dev->dtime = 1;

	slogf(1000, 1, "(devf  t%d::%s:%d) drate=%d", pthread_self(), __func__,
			__LINE__, drate);

	return EOK;
}

static int xzynq_qspi_transfer(int fd, uint8_t *send_buf, uint8_t *recv_buf,
		uint32_t len, uint8_t *cmd)
{
	xzynq_qspi_t *dev = (xzynq_qspi_t*)fd;
	uint32_t data;
	uint8_t i, cmd_opcode = 0;

	if (dev->busy)
		return EBUSY;

	if (xzynq_qspi_setup_exchange(dev, len) < 0)
		return EINVAL;

	/* Empty RX FIFO */
	while (in32(dev->vbase + XZYNQ_QSPI_ISR_OFFSET)
			& XZYNQ_QSPI_IXR_RXNEMPTY_MASK)
		in32(dev->vbase + XZYNQ_QSPI_RXD_OFFSET);

	dev->busy = 1;
	dev->recv_buf = recv_buf;
	dev->requested_bytes = len;
	dev->remaining_bytes = len;

	/* Instruction case */
	if (cmd) {
		cmd_opcode = cmd[0];
		for (i=0; i<ARRAY_SIZE(flash_inst); i++) {
			if (flash_inst[i].opcode == cmd_opcode)
				break;
		}
		if (i==ARRAY_SIZE(flash_inst))
			goto transfer;

		dev->current_inst = &flash_inst[i];
		dev->is_inst = 1;
		dev->send_buf = cmd;
		dev->requested_bytes += dev->current_inst->inst_size;
		dev->remaining_bytes += dev->current_inst->inst_size;
		len += dev->current_inst->inst_size;

		/* Get the complete command (flash inst + address/data) */
		data = 0;
		xzynq_qspi_get_write_data(dev, &data, dev->current_inst->inst_size);

		/* Write the command to the FIFO */
		out32(dev->vbase + dev->current_inst->tx_offset, data);

		/* Special case of fast reads */
		if ((cmd_opcode == FLASH_OPCODE_FAST_READ) ||
			(cmd_opcode == FLASH_OPCODE_DUAL_READ) ||
			(cmd_opcode == FLASH_OPCODE_QUAD_READ)) {
			/* Now send 1 dummy byte */
			out32(dev->vbase + XZYNQ_QSPI_TXD_01_OFFSET, 0xFFFFFF00);
		}
	}
transfer:
	dev->send_buf = send_buf;
	/*
	 * Fill the Tx FIFO with as many bytes as it takes (or as many as
	 * we have to send).
	 */
	while ((dev->remaining_bytes > 0) && !(in32(dev->vbase +
			XZYNQ_QSPI_ISR_OFFSET)&XZYNQ_QSPI_IXR_TXFULL_MASK)) {
		if (dev->remaining_bytes < sizeof(uint32_t))
			xzynq_qspi_get_write_data(dev, &data, dev->remaining_bytes);
		else
			xzynq_qspi_get_write_data(dev, &data, sizeof(uint32_t));

		out32(dev->vbase + XZYNQ_QSPI_TXD_00_OFFSET, data);
	}

	/* Enable QSPI interrupts and start transaction */
	out32(dev->vbase + XZYNQ_QSPI_IER_OFFSET, XZYNQ_QSPI_IXR_DFLT_MASK);
	xzynq_qspi_start_exchange(dev);

	/* Wait for completion */
	if (xzynq_qspi_wait(dev, dev->requested_bytes) < 0)
		goto end;

end:
	dev->busy = 0;
	xzynq_qspi_end_exchange(dev);
	return (len - dev->requested_bytes);
}

int xzynq_qspi_cmd_read(int fd, uint8_t cmd[4], uint8_t* buf, int len)
{
	int ret = 0;

	/* Get the RX response */
	ret = xzynq_qspi_transfer(fd, NULL, buf, len, cmd);
	if (ret < len) {
		fprintf(stderr, "norqspi: error receiving RX buffer (%d)\n", ret);
		return ret;
	}

	return EOK;
}

int xzynq_qspi_cmd_write(int fd, uint8_t cmd[4], uint8_t const* buf, int len)
{
	int ret = 0;

	/* Send the TX buffer */
	ret = xzynq_qspi_transfer(fd, (uint8_t*)buf, NULL, len, cmd);
	if (ret < len) {
		fprintf(stderr, "norqspi: error sending TX buffer (%d)\n", ret);
		return ret;
	}

	return EOK;
}

int xzynq_qspi_word_exchange(int fd, uint32_t* buf)
{
	int ret = 0;
	uint32_t data;

	ret = xzynq_qspi_transfer(fd, NULL, (uint8_t*)&data, 0, (uint8_t*)buf);
	if (ret != sizeof(uint32_t)) {
		fprintf(stderr, "norqspi: error exchanging buffer (%d)\n", ret);
		return -1;
	}

	*buf = data;

	return EOK;
}

int xzynq_qspi_write_1byte(int fd, uint8_t cmd)
{
	int ret = 0;

	/* Send the TX buffer */
	ret = xzynq_qspi_transfer(fd, NULL, NULL, 0, &cmd);
	if (ret < 1) {
		fprintf(stderr, "norqspi: error sending 1byte (%d)\n", ret);
		return -1;
	}

	return EOK;
}

int xzynq_qspi_open(void)
{
	static xzynq_qspi_t *dev;
	clk_freq_t clk_freq;
	int fd, err;

	if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
		fprintf(stderr, "norqspi: ThreadCtl Failed\n");
		return -1;
	}

	if ((dev = calloc(1, sizeof(xzynq_qspi_t))) == NULL) {
		fprintf(stderr, "norqspi: Could not allocate xzynq_qspi_t memory\n");
		return -1;
	}

	dev->pbase = XZYNQ_QSPI_REG_ADDRESS;
	dev->irq = XZYNQ_QSPI_IRQ;

	/* Get I2C Clock */
	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("Can't get QSPI clock");
		goto fail0;
	}

	clk_freq.id = XZYNQ_QSPI_CLK;
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

	if ((dev->vbase = mmap_device_io(XZYNQ_QSPI_REG_SIZE, dev->pbase))
			== (uintptr_t) MAP_FAILED)
		goto fail0;

	/* Disable QSPI */
	out32(dev->vbase + XZYNQ_QSPI_ER_OFFSET, 0x0);

	/* Abort transfers */
	while (in32(dev->vbase + XZYNQ_QSPI_ISR_OFFSET)
			& XZYNQ_QSPI_IXR_RXNEMPTY_MASK) {
		in32(dev->vbase + XZYNQ_QSPI_RXD_OFFSET);
		out32(dev->vbase + XZYNQ_QSPI_ISR_OFFSET, XZYNQ_QSPI_IXR_RXNEMPTY_MASK);
	}

	/* Reset control register */
	out32(dev->vbase + XZYNQ_QSPI_CR_OFFSET, XZYNQ_QSPI_CR_RESET_STATE);

	/* Clear all interrupts */
	out32(dev->vbase + XZYNQ_QSPI_ISR_OFFSET, XZYNQ_QSPI_ISR_ALL_MASK);

	/* Disable all interrupts */
	out32(dev->vbase + XZYNQ_QSPI_IDR_OFFSET, XZYNQ_QSPI_ISR_ALL_MASK);

	/* Unmask all interrupts */
	out32(dev->vbase + XZYNQ_QSPI_IMR_OFFSET, XZYNQ_QSPI_ISR_ALL_MASK);

	/* Attach interrupt */
	if ((dev->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK))
			== -1)
		goto fail1;

	if ((dev->coid = ConnectAttach(0, 0, dev->chid, _NTO_SIDE_CHANNEL, 0))
			== -1)
		goto fail2;

	dev->qspievent.sigev_notify = SIGEV_PULSE;
	dev->qspievent.sigev_coid = dev->coid;
	dev->qspievent.sigev_code = XZYNQ_QSPI_EVENT;
	dev->qspievent.sigev_priority = XZYNQ_QSPI_PRIORITY;

	dev->iid = InterruptAttach(dev->irq, xzynq_qspi_intr, dev, 0,
			_NTO_INTR_FLAGS_TRK_MSK);

	if (dev->iid == -1)
		goto fail3;

	/* Disable Linear QSPI */
	out32(dev->vbase + XZYNQ_QSPI_LQSPI_CR_OFFSET, 0);

	/* Enable QSPI */
	out32(dev->vbase + XZYNQ_QSPI_ER_OFFSET, 0x1);

	slogf(1000, 1, "(devf  t%d::%s:%d) irq=%d", pthread_self(), __func__,
			__LINE__, dev->irq);

#ifdef CONFIG_PMM
	funcs.standby = xzynq_pm_standby;
	funcs.resume = xzynq_pm_resume;
	pmm_init(&funcs, "qspi");
#endif

	return ((int) dev);

fail3:
	ConnectDetach(dev->coid);
fail2:
	ChannelDestroy(dev->chid);
fail1:
	munmap_device_io(dev->vbase, XZYNQ_QSPI_REG_SIZE);
fail0:
	free(dev);

	return -1;
}

int xzynq_qspi_close(int fd)
{
	xzynq_qspi_t *dev = (xzynq_qspi_t *) fd;

	out32(dev->vbase + XZYNQ_QSPI_ER_OFFSET, 0);
	InterruptDetach(dev->iid);
	ConnectDetach(dev->coid);
	ChannelDestroy(dev->chid);
	munmap_device_io(dev->vbase, XZYNQ_QSPI_REG_SIZE);
	free(dev);

	return EOK;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
