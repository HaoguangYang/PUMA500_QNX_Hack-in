/*
 * $QNXLicenseC: 
 * Copyright 2010, QNX Software Systems.  
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
 
#include "f3s_qspi.h"

/*
 * This is the open callout for the QSPI serial NOR flash driver.
 */
int32_t f3s_qspi_open(f3s_socket_t *socket, uint32_t flags)
{
	static int fd = 0;
	int ret = EOK;

	/* check if not initialized */
	if (!fd) {
		fd = xzynq_qspi_open();
		if (fd < 0)
			return (EAGAIN);

		ret = xzynq_qspi_setcfg(fd, XZYNQ_QSPI_IO_MODE,
				XZYNQ_QSPI_MAX_DRATE, XZYNQ_QSPI_DEFAULT_SS);
		if (ret)
			return ret;

		socket->name = (unsigned char*)"QSPI serial flash";
		socket->window_size = socket->array_size = TOTAL_SIZE_BYTES;
		socket->socket_handle = (void *)fd;
	}

	return (EOK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
