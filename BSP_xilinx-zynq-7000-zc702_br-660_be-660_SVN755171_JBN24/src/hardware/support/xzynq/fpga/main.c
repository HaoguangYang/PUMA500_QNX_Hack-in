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

#include <proto.h>

static int fpga_io_open(resmgr_context_t *ctp, io_open_t *msg,
		RESMGR_HANDLE_T *handle, void *extra);
static int fpga_io_close(resmgr_context_t *ctp, void *reserved, RESMGR_OCB_T *ocb);
static int fpga_io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);
static int fpga_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb);

static fpga_dev_t *dev;

int main(int argc, char *argv[])
{
	int id;
	resmgr_connect_funcs_t fpga_connect_funcs;
	resmgr_io_funcs_t fpga_io_funcs;
	dispatch_t *dpp;
	resmgr_attr_t rattr;
	dispatch_context_t *ctp;
	iofunc_attr_t fpga_ioattr;

	/* Initialize the dispatch interface */
	dpp = dispatch_create();
	if (!dpp) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"FPGA error: Failed to create dispatch interface\n");
		goto fail;
	}

	/* Initialize the resource manager attributes */
	memset(&rattr, 0, sizeof(rattr));

	/* Initialize the connect functions */
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &fpga_connect_funcs,
			_RESMGR_IO_NFUNCS, &fpga_io_funcs);
	fpga_connect_funcs.open = fpga_io_open;
	fpga_io_funcs.close_ocb = fpga_io_close;
	fpga_io_funcs.write = fpga_io_write;
	fpga_io_funcs.devctl = fpga_io_devctl;
	iofunc_attr_init(&fpga_ioattr, S_IFCHR | 0666, NULL, NULL);

	/* Attach the device name */
	id = resmgr_attach(dpp, &rattr, FPGA_NAME, _FTYPE_ANY, 0,
			&fpga_connect_funcs, &fpga_io_funcs, &fpga_ioattr);
	if (id == -1) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"FPGA error: Failed to attach pathname\n");
		goto fail;
	}

	/* Allocate a context structure */
	ctp = dispatch_context_alloc(dpp);

	dev = malloc(sizeof(fpga_dev_t));
	if (!dev) {
		goto fail;
	}
	dev->secure = FPGA_SEC_OFF;

	/* Memory map the devcfg and SCL registers */
	dev->devcfg_reglen = XZYNQ_DEVCFG_SIZE;
	dev->scl_reglen = XZYNQ_SLCR_LEN;
	dev->devcfg_physbase = XZYNQ_DEVCFG_BASEADDR;
	dev->scl_physbase = XZYNQ_SLCR_BASE;

	dev->devcfg_regbase = mmap_device_io(dev->devcfg_reglen,
			dev->devcfg_physbase);
	if (dev->devcfg_regbase == MAP_DEVICE_FAILED) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"Failed to map devcfg registers for the FPGA\n");
		goto fail;
	}
	dev->scl_regbase = mmap_device_io(dev->scl_reglen, dev->scl_physbase);
	if (dev->scl_regbase == MAP_DEVICE_FAILED) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"Failed to map SCL registers for the FGPA\n");
		goto fail;
	}

	/* Run in the background */
	if (procmgr_daemon(EXIT_SUCCESS, PROCMGR_DAEMON_NOCLOSE
			| PROCMGR_DAEMON_NODEVNULL ) == -1) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"%s:  procmgr_daemon", argv[0]);
		return EXIT_FAILURE;
	}

	while (1) {
		if ((ctp = dispatch_block(ctp)) == NULL) {
			slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
					"FPGA error: Block error\n");
			goto fail;
		}
		dispatch_handler(ctp);
	}

	free(dev);

	return EXIT_SUCCESS;

	fail: if (dev != NULL) {
		free(dev);
	}
	return EXIT_FAILURE;

}

/*
 * Initialize the FPGA for a transfer
 */
static
int fpga_io_open(resmgr_context_t *ctp, io_open_t *msg,
		RESMGR_HANDLE_T *handle, void *extra)
{
	// if device not open for writing, do not disturb the FPGA
	if ( (msg->connect.ioflag & _IO_FLAG_WR) == 0 ) {
		return iofunc_open_default(ctp, msg, handle, extra);
	}

	// set FPGA_RST_CTRL and clear LVL_SHFTR_EN
	fpga_disable_axi(dev);

	/* Prepare the FPGA interface */
	fpga_cfg_init(dev);
	fpga_enable_pcap(dev);
	fpga_reset(dev);

	if (fpga_is_dma_busy(dev) == XZYNQ_SUCCESS) {
		return EBUSY;
	}

	/* ensure PCFG_INIT in STATUS[4] is high so that FPGA housecleaning is done */
	if ((fpga_get_status_register(dev) & XZYNQ_FPGA_STATUS_PCFG_INIT_MASK)
			== 0) {
		return ENOSYS;
	}

	if (dev->secure == FPGA_SEC_ON) {
		fpga_set_control_register(
				dev,
				(XZYNQ_FPGA_CTRL_PCFG_AES_EN_MASK
						| XZYNQ_FPGA_CTRL_PCAP_RATE_EN_MASK));
	} else {
		fpga_set_control_register(dev, fpga_get_control_register(dev)
				& ~XZYNQ_FPGA_CTRL_PCFG_AES_EN_MASK);
	}

	/* clear PCFG_DONE_INT in INT_STS[2] */
	out32(dev->devcfg_regbase + XZYNQ_FPGA_INT_STS_OFFSET, XZYNQ_FPGA_IXR_PCFG_DONE_MASK);

	return (iofunc_open_default(ctp, msg, handle, extra));
}

/*
 * Cleanup after a transfer and enable FPGA
 */
static
int fpga_io_close(resmgr_context_t *ctp, void *reserved, RESMGR_OCB_T *ocb)
{
	int rc = iofunc_close_ocb_default(ctp, reserved, ocb);

	if (rc == EOK) {
		if ( !fpga_is_prog_done(dev) ) {
			return EIO;
		}

		// clear FPGA_RST_CTRL and set LVL_SHFTR_EN
		fpga_enable_axi(dev);
	}

	return rc;
}

/*
 * Flash the FPGA
 */
static
int fpga_io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb)
{
	int status;
	char *index;
	char *end;
	char *buf;
	int page_length;
	int size;
	int bytes_read;

	page_length = PAGE_LENGTH;

	if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK) {
		return status;
	}

	// No special xtypes
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
		return ENOSYS;
	}

	if (msg->i.nbytes == sizeof(msg)) {
		return EIO;
	}

	/*
	 * Allocate a block of contiguous physical memory for the DMA to read from.
	 * Copy the write data into the buffer
	 */
	size = msg->i.nbytes;
	if (size % 4 != 0) {
		size += 4 - (size % 4);
	}
	buf = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_NOCACHE, /* MAP_PRIVATE |*/
	MAP_ANON | MAP_PHYS, NOFD, 0);
	if (!buf) {
		return ENOMEM;
	}
	bytes_read = resmgr_msgread(ctp, buf, msg->i.nbytes, sizeof(*msg));
	if (bytes_read < msg->i.nbytes) {
		return EFAULT;
	}

	/* Iterate through the buffer, copying 1 page at a time */
	index = buf;
	end = buf + msg->i.nbytes;
	while (index < end) {
		if (index + page_length > end) {
			page_length = end - index;
		}

		fpga_initiate_dma(dev, (_Uint32t) xzynq_vtophys(index),
				XZYNQ_FPGA_DMA_INVALID_ADDRESS,
				page_length / 4, 0);

		index += page_length;

		/* Wait for the DMA Transfer to finish. Clear the interrupt */
		while (!(fpga_intr_get_status(dev) & XZYNQ_FPGA_IXR_DMA_DONE_MASK))
			;
		out32(dev->devcfg_regbase + XZYNQ_FPGA_INT_STS_OFFSET,
				XZYNQ_FPGA_IXR_DMA_DONE_MASK);
	}
	munmap(buf, size);

	_IO_SET_WRITE_NBYTES (ctp, size);

	return _RESMGR_NPARTS (0);

}

/*
 * Set meta data about the FPGA
 */
static
int fpga_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
	void *dptr;
	int status, nbytes;

	status = iofunc_devctl_default(ctp, msg, ocb);

	if (status != _RESMGR_DEFAULT) {
		return status;
	}

	nbytes = 0;
	dptr = _DEVCTL_DATA(msg->i);

	switch (msg->i.dcmd) {
	case DCMD_FPGA_RESET:
		fpga_reset(dev);
		break;
	case DCMD_FPGA_ENABLE_SEC:
		dev->secure = FPGA_SEC_ON;
		break;
	case DCMD_FPGA_DISABLE_SEC:
		dev->secure = FPGA_SEC_OFF;
		break;
	case DCMD_FPGA_IS_PROG_DONE:
		*((_Uint8t *) dptr) = fpga_is_prog_done(dev);
		nbytes = sizeof(_Uint8t);
		break;
	default:
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"FPGA error: devctl error\n");
		return ENOTTY;
	}

	if (nbytes == 0) {
		return (EOK);
	} else {
		msg->o.ret_val = 0;
		msg->o.nbytes = nbytes;
		return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + nbytes));
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/fpga/main.c $ $Rev: 752035 $")
#endif
