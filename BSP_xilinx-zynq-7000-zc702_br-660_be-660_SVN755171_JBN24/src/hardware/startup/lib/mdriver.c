/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems. All Rights Reserved.
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

ptrdiff_t			cpu_mdriver_map(void);
void				*cpu_mdriver_prepare(struct mdriver_entry *);

static ptrdiff_t	func_diff;

static void
mdriver_fini(void) {
	struct mdriver_entry	*md = lsp.mdriver.p;
	int						num = lsp.mdriver.size / sizeof(*md);
	int						i;

	for(i = 0; i < num; ++i, ++md) {
		if(md->intr != _NTO_INTR_SPARE) {
			if(md->handler(MDRIVER_STARTUP_FINI, md->data)) {
				md->intr = _NTO_INTR_SPARE;
			} else {
				md->handler = (int (*)())((uintptr_t)md->handler + func_diff);
				md->data = (void *)md->internal;
				md->internal = -1;
			}
		}
	}
}

static void
mdriver_prepare(void) {
	struct mdriver_entry	*md = lsp.mdriver.p;
	int						num = lsp.mdriver.size / sizeof(*md);
	int						i;

	if(num > 0) {
		func_diff = cpu_mdriver_map();
		for(i = 0; i < num; ++i, ++md) {
			if(md->intr != _NTO_INTR_SPARE) {
				if(md->handler(MDRIVER_STARTUP_PREPARE, md->data)) {
					md->intr = _NTO_INTR_SPARE;
				} else {
					md->internal = (uintptr_t)cpu_mdriver_prepare(md);
				}
			}
		}
	}
	mdriver_hook = mdriver_fini;
}

static void
mdriver_docheck(void) {
	struct mdriver_entry	*md = lsp.mdriver.p;
	int						num = lsp.mdriver.size / sizeof(*md);
	int						i;

	for(i = 0; i < num; ++i, ++md) {
		if(md->intr != _NTO_INTR_SPARE) {
			if(md->handler(MDRIVER_STARTUP, md->data)) {
				md->intr = _NTO_INTR_SPARE;
			}
		}
	}
}

int
mdriver_add(char *name, int intr, int (*handler)(int state, void *data),
				paddr32_t data_paddr, unsigned data_size) {
	uint8_t					*p;
	struct mdriver_entry	*md;
	unsigned				name_off;
	void					*data;

	if(!interrupt_valid(intr)) {
		crash("invalid interrupt number");
	}
	//NYI: should map it, support NULL_PADDR32?
	data = MAKE_1TO1_PTR(data_paddr);

	if(handler(MDRIVER_INIT, data)) {
		return -1;
	}

	name_off = add_string(name);

	p = grow_syspage_section(&lsp.mdriver, sizeof(*md));
	md = (struct mdriver_entry *)(p + lsp.mdriver.size - sizeof(*md));

	md->name = name_off;
	md->intr = intr;
	md->handler = handler;
	md->data = data;
	md->data_paddr = data_paddr;
	md->data_size = data_size;

	mdriver_check = mdriver_docheck;
	mdriver_hook = mdriver_prepare;

	return md - (struct mdriver_entry *)p;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/lib/mdriver.c $ $Rev: 744481 $")
#endif
