/*
 * $QNXLicenseC:
 * Copyright 2007, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable license fees to QNX
 * Software Systems before you may reproduce, modify or distribute this software,
 * or any work that includes all or part of this software.   Free development
 * licenses are available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review this entire
 * file for other proprietary rights or license notices, as well as the QNX
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/
 * for other information.
 * $
 */




#include "startup.h"

ptrdiff_t
cpu_mdriver_map(void) {
	paddr32_t	start;
	paddr32_t	end;
	uintptr_t	new;

	if(!(shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)) {
		return 0;
	}
	start = TRUNCPG(shdr->ram_paddr);
	end   = ROUNDPG(shdr->ram_paddr+shdr->startup_size);
	new   = (uintptr_t)callout_memory_map(end - start, start, PROT_EXEC|PROT_READ|PROT_WRITE);
	return new - start;
}

void *
cpu_mdriver_prepare(struct mdriver_entry *md) {
	paddr32_t	start;
	paddr32_t	end;
	uintptr_t	new;

	if(!(shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)) {
		return md->data;
	}
	start = TRUNCPG(md->data_paddr);
	end   = ROUNDPG(md->data_paddr+md->data_size);
	new   = (uintptr_t)callout_memory_map(end - start, start,
					PROT_NOCACHE|PROT_READ|PROT_WRITE);
	return (void *)(new + (md->data_paddr & (__PAGESIZE-1)));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/lib/arm/cpu_mdriver.c $ $Rev: 744481 $")
#endif
