/*
 * $QNXLicenseC: 
 * Copyright 2011, QNX Software Systems.  
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

// Module Description:  board specific interface

#include <sim_mmc.h>
#include <sim_sdhci.h>

int bs_init(SIM_HBA *hba)
{
	CONFIG_INFO	*cfg;
	SIM_MMC_EXT	*ext;
	sdhci_ext_t *sdhci;
	cfg = (CONFIG_INFO *)&hba->cfg;

	if (!cfg->NumIOPorts) {
		cfg->MemBase[0]   = 0xE0100000;
		cfg->MemLength[0] = 0x100;
        cfg->NumMemWindows = 1;
	} 
	
	if (!cfg->NumIRQs) {
		cfg->IRQRegisters[0] = 56;
		cfg->NumIRQs = 1;
	}

	if (sdhci_init(hba) != MMC_SUCCESS)
		return MMC_FAILURE;

	ext = (SIM_MMC_EXT *)hba->ext;
	sdhci = (sdhci_ext_t *)ext->handle;

	return MMC_SUCCESS;
}

int bs_dinit(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext;
	sdhci_ext_t	*sdhci;

	ext  = (SIM_MMC_EXT *)hba->ext;
	sdhci = (sdhci_ext_t *)ext->handle;

	return (CAM_SUCCESS);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/devb/mmcsd/arm/xzynq.le.v7/sim_bs.c $ $Rev: 752035 $")
#endif
