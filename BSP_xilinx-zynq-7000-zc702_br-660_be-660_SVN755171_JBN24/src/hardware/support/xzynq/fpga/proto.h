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
#include <fcntl.h>
#include <hw/fpga.h>

typedef struct _fpga_dev {

	/* Memory location of devcfg */
	unsigned devcfg_reglen;
	uintptr_t devcfg_regbase;
	unsigned devcfg_physbase;

	/* Memory location of devcfg */
	unsigned scl_reglen;
	uintptr_t scl_regbase;
	unsigned scl_physbase;

	/* Manage interrupts from the FPGA register */
	struct sigevent fpga_intr;

	/* Should writes be secure? */
	int secure;

} fpga_dev_t;

#define FPGA_SEC_ON 1
#define FPGA_SEC_OFF 0
#define PAGE_LENGTH 4096

void fpga_reset(fpga_dev_t *dev);
void fpga_cfg_init(fpga_dev_t *dev);
void fpga_intr_enable(fpga_dev_t *dev, _Uint32t mask);
void fpga_intr_disable(fpga_dev_t *dev, _Uint32t mask);
_Uint32t fpga_intr_get_enable(fpga_dev_t *dev);
_Uint32t fpga_intr_get_status(fpga_dev_t *dev);
void fpga_intr_clear(fpga_dev_t *dev, _Uint32t mask);
void fpga_enable_pcap(fpga_dev_t *dev);
void fpga_disable_pcap(fpga_dev_t *dev);
void fpga_enable_axi(fpga_dev_t *dev);
void fpga_disable_axi(fpga_dev_t *dev);
void fpga_set_control_register(fpga_dev_t *dev, _Uint32t mask);
_Uint32t fpga_get_control_register(fpga_dev_t *dev);
void fpga_set_lock_register(fpga_dev_t *dev, _Uint32t data);
_Uint32t fpga_get_lock_register(fpga_dev_t *dev);
void fpga_set_config_register(fpga_dev_t *dev, _Uint32t data);
_Uint32t fpga_get_config_register(fpga_dev_t *dev);
void fpga_set_status_register(fpga_dev_t *dev, _Uint32t data);
_Uint32t fpga_get_status_register(fpga_dev_t *dev);
void fpga_set_rom_shadow_register(fpga_dev_t *dev, _Uint32t data);
_Uint32t fpga_get_software_id_register(fpga_dev_t *dev);
void fpga_set_misc_control_register(fpga_dev_t *dev, _Uint32t mask);
_Uint32t fpga_get_misc_control_register(fpga_dev_t *dev);
_Uint32t fpga_is_dma_busy(fpga_dev_t *dev);
void fpga_initiate_dma(fpga_dev_t *dev, _Uint32t src_ptr, _Uint32t dest_ptr,
		_Uint32t src_word_length, _Uint32t dest_word_length);
paddr_t xzynq_vtophys(void *addr);
_Uint8t fpga_is_prog_done(fpga_dev_t *dev);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/fpga/proto.h $ $Rev: 752035 $")
#endif
