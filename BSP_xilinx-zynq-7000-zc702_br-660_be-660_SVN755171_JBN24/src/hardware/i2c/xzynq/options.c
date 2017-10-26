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

#include "proto.h"

int xzynq_options(xzynq_dev_t *dev, int argc, char *argv[]) {
	int c;
	int prev_optind;
	int done = 0;

	/* defaults */
	dev->intr = XZYNQ_IRQ_I2C1;
	dev->iid = -1;
	dev->physbase = XZYNQ_XIICPS1_BASE_ADDR;
	dev->reglen = XZYNQ_XIICPS_REG_SIZE;
	dev->options = 0;

	while (!done) {
		prev_optind = optind;
		c = getopt(argc, argv, "a:i:p:s:v");
		switch (c) {
		case 'i':
			dev->intr = strtol(optarg, &optarg, NULL);
			break;

		case 'p':
			dev->physbase = strtoul(optarg, &optarg, NULL);
			break;

		case 'v':
			dev->options |= XZYNQ_OPT_VERBOSE;
			break;

		case '?':
			if (optopt == '-') {
				++optind;
				break;
			}
			return -1;

		case -1:
			if (prev_optind < optind) /* -- */
				return -1;

			if (argv[optind] == NULL) {
				done = 1;
				break;
			}
			if (*argv[optind] != '-') {
				++optind;
				break;
			}
			return -1;

		case ':':
		default:
			return -1;
		}
	}

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/options.c $ $Rev: 752035 $")
#endif
