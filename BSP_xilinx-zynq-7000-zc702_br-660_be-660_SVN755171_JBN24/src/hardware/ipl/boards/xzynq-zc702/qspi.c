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

#include <hw/inout.h>
#include <stdio.h>
#include "ipl.h"
#include "ipl_xzynq.h"
#include "qspi.h"

/* Initializes the QSPI controller */
int qspi_init(void)
{
	/* Disabling QSPI controller */
	out32(XZYNQ_QSPI_REG_ADDRESS + XZYNQ_QSPI_ER_OFFSET,
			~(XZYNQ_QSPI_ER_ENABLE_MASK));

	/* Making sure of the configuration */
	out32(XZYNQ_QSPI_REG_ADDRESS + XZYNQ_QSPI_CR_OFFSET, XZYNQ_QSPI_CR_AUTO);

	/* The QSPI must be in Linear mode */
	out32(XZYNQ_QSPI_REG_ADDRESS + XZYNQ_QSPI_LQSPI_CR_OFFSET,
			XZYNQ_QSPI_LQSPI_CR_RST_STATE);

	/* Enabling QSPI controller */
	out32(XZYNQ_QSPI_REG_ADDRESS + XZYNQ_QSPI_ER_OFFSET,
			XZYNQ_QSPI_ER_ENABLE_MASK);

	return 0;
}

/* read blocks from SDMMC */
int qspi_get_ifs(unsigned address)
{
	uint32_t count;
	uint32_t length;
	uint32_t source;

	/* Get the image length (offset 0x2c) */
	length = in32(XZYNQ_QSPI_LINEAR_ADDRESS + XZYNQ_QSPI_IFS_LEN_OFFSET)
			+ XZYNQ_QSPI_IFS_LEN_PAD;
	if (length > XZYNQ_QSPI_IFS_SIZE) {
		ser_putstr("Invalid IFS size image (");
		ser_puthex(length);
		ser_putstr(")...\n");
		return -1;
	}

	source = XZYNQ_QSPI_LINEAR_ADDRESS + XZYNQ_QSPI_IFS_OFFSET;
	for (count = 0; count < length; count += 4) {
		out32(address, in32(source));
		source += 4;
		address += 4;
	}

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
