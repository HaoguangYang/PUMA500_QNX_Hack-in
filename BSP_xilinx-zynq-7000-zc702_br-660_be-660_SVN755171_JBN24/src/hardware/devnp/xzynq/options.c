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

#include "xzynq.h"

/* No options yet, place holder */
static char *xzynq_opts[] = {
		NULL,
};

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
int xzynq_parse_options(xzynq_dev_t *xzynq, char *optstring, nic_config_t *cfg)
{
	char *value;
	int32_t opt;
	char *options, *freeptr;
	char *c;
	int32_t rc = 0;
	int32_t err = EOK;

	if (optstring == NULL)
		return 0;

	/* getsubopt() is destructive */
	freeptr = options = strdup(optstring);

	while (options && *options != '\0') {
		c = options;
		if ((opt = getsubopt(&options, xzynq_opts, &value)) == -1) {
			if (nic_parse_options(cfg, value) == EOK)
				continue;
			goto error;
		}

		/* Driver specific option will go here */
		switch (opt) {
		default:
			break;
		}

		error: err = EINVAL;
		rc = -1;
	}
	(void)c;

	free(freeptr, M_DEVBUF);
	errno = err;

	return (rc);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/options.c $ $Rev: 752035 $")
#endif
