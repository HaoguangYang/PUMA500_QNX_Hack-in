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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <errno.h>
#include <arm/xzynq.h>
#include <sys/procmgr.h>
#include <drvr/hwinfo.h>
#include <string.h>
#include <hw/clk.h>

#define CLK_DISABLE 0
#define CLK_ENABLE  1

typedef struct _clk_dev {
	unsigned reglen;
	uintptr_t regbase;
	unsigned physbase;
} clk_dev_t;

int clk_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb);

uint32_t xzynq_get_cpu_clk(clk_dev_t *dev);
uint32_t xzynq_get_arm_pll_clk(clk_dev_t *dev);
uint32_t xzynq_get_ddr_pll_clk(clk_dev_t *dev);
uint32_t xzynq_get_io_pll_clk(clk_dev_t *dev);
uint32_t xzynq_clk_621_true(clk_dev_t *dev);
uint32_t xzynq_get_cpu_1x_clk(clk_dev_t *dev);
void xzynq_clk_control(clk_dev_t *dev, uint32_t offset, uint32_t shift, int action);
uint32_t xzynq_get_peripheral_clk(clk_dev_t *dev, clk_id_t id);
uint32_t xzynq_get_can_clk(clk_dev_t *dev);
void xzynq_set_can_clk(clk_dev_t *dev, uint32_t freq);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/clock/proto.h $ $Rev: 752035 $")
#endif
