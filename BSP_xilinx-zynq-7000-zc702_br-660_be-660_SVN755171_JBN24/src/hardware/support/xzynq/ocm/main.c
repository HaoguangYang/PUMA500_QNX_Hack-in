/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

/*
 * we are extending the attr
 */

struct ocm_attr;
#define IOFUNC_ATTR_T   struct ocm_attr

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/procmgr.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <arm/xzynq.h>
#include <hw/inout.h>
#include "proto.h"

static resmgr_connect_funcs_t connect_funcs;
static resmgr_io_funcs_t io_funcs;
ocm_attr_t attrs[NUM_OCM_DEVICES];

char *names[NUM_OCM_DEVICES] = {
	"/dev/ocm/ocm1",
	"/dev/ocm/ocm2",
	"/dev/ocm/ocm3",
	"/dev/ocm/ocm4"
};

static char high_value[MAX_CHAR] = "HIGH\n";
static char low_value[MAX_CHAR] = "LOW\n";

int main(int argc, char **argv)
{
	dispatch_t *dpp;
	resmgr_attr_t resmgr_attr;
	resmgr_context_t *ctp;
	int id;
	int i;

	if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
		perror("ThreadCtl");
		return NULL;
	}

	/* initialize dispatch interface */
	if ((dpp = dispatch_create()) == NULL) {
		fprintf(
				stderr,
				"%s: Unable to allocate \
				dispatch handle.\n",
				argv[0]);
		return EXIT_FAILURE;
	}

	/* initialize resource manager attributes */
	memset(&resmgr_attr, 0, sizeof resmgr_attr);
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = 2048;

	/* initialize functions for handling messages */
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
			_RESMGR_IO_NFUNCS, &io_funcs);
	io_funcs.read = io_read;
	io_funcs.write = io_write;
	io_funcs.lseek = io_lseek;

	/* attach our device name (passing in the POSIX defaults
	 from the iofunc_func_init and iofunc_attr_init functions)
	 */
	for (i = 0; i < NUM_OCM_DEVICES; i++) {
		/* initialize attribute structure */
		iofunc_attr_init(&attrs[i].attr, S_IFNAM | 0666, 0, 0);
		attrs[i].device = i;

		if ((id = resmgr_attach(dpp, &resmgr_attr, names[i],
				_FTYPE_ANY, 0, &connect_funcs, &io_funcs,
				&attrs[i])) == -1) {
			fprintf(stderr, "%s: Unable to attach name.\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

	/* Run in the background */
	if (procmgr_daemon(EXIT_SUCCESS, PROCMGR_DAEMON_NOCLOSE
			| PROCMGR_DAEMON_NODEVNULL ) == -1) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "%s:  procmgr_daemon",
				argv[0]);
		return EXIT_FAILURE;
	}

	/* allocate a context structure */
	ctp = resmgr_context_alloc(dpp);

	/* start the resource manager message loop */
	while (1) {
		if ((ctp = resmgr_block(ctp)) == NULL) {
			fprintf(stderr, "block error\n");
			return EXIT_FAILURE;
		}
		resmgr_handler(ctp);
	}
}

int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
	int nleft;
	int nbytes;
	int nparts;
	int status;
	int size;
	char *buffer;
	uint32_t value;
	uintptr_t slrc_virt_base;

	if ((status = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK) {
		return (status);
	}

	// No special xtypes
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
		return (ENOSYS);
	}

	/* Read the status for the device in SLCR */
	slrc_virt_base = mmap_device_io(XZYNQ_SLCR_LEN, XZYNQ_SLCR_BASE);
	value = in32(slrc_virt_base + XZYNQ_SLCR_OCM_CFG_REG) & (1 << ocb->attr->device);
	if (value) {
		buffer = high_value;
	} else {
		buffer = low_value;
	}
	munmap_device_io(slrc_virt_base, XZYNQ_SLCR_LEN);

	/*
	 *  On all reads (first and subsequent), calculate
	 *  how many bytes we can return to the client,
	 *  based upon the number of bytes available (nleft)
	 *  and the client's buffer size
	 */
	size = strlen(buffer);
	nleft = size - ocb->offset;
	nbytes = min (msg->i.nbytes, nleft);

	if (nbytes > 0) {
		/* set up the return data IOV */
		SETIOV (ctp->iov, buffer, size);

		/* set up the number of bytes (returned by client's read()) */
		_IO_SET_READ_NBYTES (ctp, size);

		/*
		 * advance the offset by the number of bytes
		 * returned to the client.
		 */
		ocb->offset += nbytes;
		nparts = 1;
	} else {
		/*
		 * they've asked for zero bytes or they've already previously
		 * read everything
		 */
		_IO_SET_READ_NBYTES (ctp, 0);
		nparts = 0;
	}

	return (_RESMGR_NPARTS (nparts));
}

int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb)
{
	int status;
	uint32_t value;
	char buffer[MAX_CHAR] = { '\0' };
	uintptr_t slrc_virt_base;
	uintptr_t mpcore_virt_base;

	if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK)
		return (status);

	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
		return (ENOSYS);

	/* set up the number of bytes (returned by client's write()) */

	_IO_SET_WRITE_NBYTES (ctp, msg->i.nbytes);

	/* Check the size of the buffer */
	if (msg->i.nbytes > MAX_CHAR)
		return (ENOMEM);

	/*
	 *  Reread the data from the sender's message buffer.
	 *  We're not assuming that all of the data fit into the
	 *  resource manager library's receive buffer.
	 */
	resmgr_msgread(ctp, buffer, msg->i.nbytes, sizeof(msg->i));
	buffer[msg->i.nbytes] = '\0'; /* just in case the text is not NULL terminated */

	/* Map the SLCR/MPCORE Register */
	mpcore_virt_base = mmap_device_io(XZYNQ_MPCORE_LEN, XZYNQ_MPCORE_BASE);
	slrc_virt_base = mmap_device_io(XZYNQ_SLCR_LEN, XZYNQ_SLCR_BASE);

	/* SLCR unlock */
	out32(slrc_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Check the argument */
	value = in32(slrc_virt_base + XZYNQ_SLCR_OCM_CFG_REG);
	if (!strncasecmp(buffer, high_value, strlen(high_value) - 1)) {
		value |= (1 << ocb->attr->device);
	} else if (!strncasecmp(buffer, low_value, strlen(low_value) - 1)) {
		value &= ~(1 << ocb->attr->device);
	} else {
		goto done;
	}

	/*
	 * Complete all outstanding transactions by issuing data (DSB)
	 * and instruction (ISB) synchronization barrier commands.
	 */
	__asm__ __volatile__("dsb");
	__asm__ __volatile__("isb");

	/* Write the new mapping for the OCM */
	out32(slrc_virt_base + XZYNQ_SLCR_OCM_CFG_REG, value);

	/*
	 * Ensure that the access has completed to the SLCR by issuing a
	 * data memory barrier (DMB) instruction. This allows subsequent
	 * accesses to rely on the new address mapping.
	 */
	__asm__ __volatile__("dmb");

	/* Lock the SLCR */
//	out32(slrc_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

done:
	munmap_device_io(slrc_virt_base, XZYNQ_SLCR_LEN);

	return (_RESMGR_NPARTS (0));
}

int io_lseek(resmgr_context_t *ctp, io_lseek_t *msg, RESMGR_OCB_T *ocb)
{
	/* Doesn't manage the whence SEEK_END and SEEK_CUR */
	ocb->offset = msg->i.offset;

	return _RESMGR_STATUS(ctp, 0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/ocm/main.c $ $Rev: 752035 $")
#endif
