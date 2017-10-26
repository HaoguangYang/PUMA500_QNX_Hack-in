/*
 * $QNXLicenseC: 
 * Copyright 2013 QNX Software Systems.
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
#ifndef XZYNQ_H
#define XZYNQ_H

#define XZYNQ_DMA_MAX_WAIT 4000

#define XZYNQ_DMA_CHANNELS_PER_DEV		8

/* register map */
#define XZYNQ_DMA_DS_OFFSET		0x000 /* DMA Status Register */
#define XZYNQ_DMA_DPC_OFFSET	0x004 /* DMA Program Counter Rregister */
#define XZYNQ_DMA_INTEN_OFFSET	0X020 /* DMA Interrupt Enable Register */
#define XZYNQ_DMA_ES_OFFSET		0x024 /* DMA Event Status Register */
#define XZYNQ_DMA_INTSTATUS_OFFSET	0x028 /* DMA Interrupt Status Register */
#define XZYNQ_DMA_INTCLR_OFFSET	0x02c /* DMA Interrupt Clear Register */
#define XZYNQ_DMA_FSM_OFFSET 	0x030 /* DMA Fault Status DMA Manager Register */
#define XZYNQ_DMA_FSC_OFFSET	0x034 /* DMA Fault Status DMA Chanel Register */
#define XZYNQ_DMA_FTM_OFFSET	0x038 /* DMA Fault Type DMA Manager Register */
#define XZYNQ_DMA_FTC0_OFFSET	0x040 /* DMA Fault Type for DMA Channel 0 */
/*
 * The offset for the rest of the FTC registers is calculated as
 * FTC0 + dev_chan_num * 4
 */
#define XZYNQ_DMA_FTCn_OFFSET(ch)	(XZYNQ_DMA_FTC0_OFFSET + (ch) * 4)

#define XZYNQ_DMA_CS0_OFFSET		0x100 /* Channel Status for DMA Channel 0 */
/*
 * The offset for the rest of the CS registers is calculated as
 * CS0 + * dev_chan_num * 0x08
 */
#define XZYNQ_DMA_CSn_OFFSET(ch)	(XZYNQ_DMA_CS0_OFFSET + (ch) * 8)

#define XZYNQ_DMA_CPC0_OFFSET	0x104 /* Channel Program Counter for DMA Channel 0 */
/*
 * The offset for the rest of the CPC registers is calculated as
 * CPC0 + dev_chan_num * 0x08
 */
#define XZYNQ_DMA_CPCn_OFFSET(ch)	(XZYNQ_DMA_CPC0_OFFSET + (ch) * 8)

#define XZYNQ_DMA_SA_0_OFFSET	0x400 /* Source Address Register for DMA Channel 0 */
/* The offset for the rest of the SA registers is calculated as
 * SA_0 + dev_chan_num * 0x20
 */
#define XZYNQ_DMA_SA_n_OFFSET(ch)	(XZYNQ_DMA_SA_0_OFFSET + (ch) * 0x20)

#define XZYNQ_DMA_DA_0_OFFSET	0x404 /* Destination Address Register for DMA Channel 0 */
/* The offset for the rest of the DA registers is calculated as
 * DA_0 + dev_chan_num * 0x20
 */
#define XZYNQ_DMA_DA_n_OFFSET(ch)	(XZYNQ_DMA_DA_0_OFFSET + (ch) * 0x20)

#define XZYNQ_DMA_CC_0_OFFSET	0x408 /* Channel Control Register for DMA Channel 0 */
/*
 * The offset for the rest of the CC registers is calculated as
 * CC_0 + dev_chan_num * 0x20
 */
#define XZYNQ_DMA_CC_n_OFFSET(ch)	(XZYNQ_DMA_CC_0_OFFSET + (ch) * 0x20)

#define XZYNQ_DMA_LC0_0_OFFSET	0x40C /* Loop Counter 0 for DMA Channel 0 */
/*
 * The offset for the rest of the LC0 registers is calculated as
 * LC_0 + dev_chan_num * 0x20
 */
#define XZYNQ_DMA_LC0_n_OFFSET(ch)	(XZYNQ_DMA_LC0_0_OFFSET + (ch) * 0x20)
#define XZYNQ_DMA_LC1_0_OFFSET	0x410 /* Loop Counter 1 for DMA Channel 0 */
/*
 * The offset for the rest of the LC1 registers is calculated as
 * LC_0 + dev_chan_num * 0x20
 */
#define XZYNQ_DMA_LC1_n_OFFSET(ch)	(XZYNQ_DMA_LC1_0_OFFSET + (ch) * 0x20)

#define XZYNQ_DMA_DBGSTATUS_OFFSET	0xD00 /* Debug Status Register */
#define XZYNQ_DMA_DBGCMD_OFFSET		0xD04 /* Debug Command Register */
#define XZYNQ_DMA_DBGINST0_OFFSET	0xD08 /* Debug Instruction 0 Register */
#define XZYNQ_DMA_DBGINST1_OFFSET	0xD0C /* Debug Instruction 1 Register */

#define XZYNQ_DMA_CR0_OFFSET		0xE00 /* Configuration Register 0 */
#define XZYNQ_DMA_CR1_OFFSET		0xE04 /* Configuration Register 1 */
#define XZYNQ_DMA_CR2_OFFSET		0xE08 /* Configuration Register 2 */
#define XZYNQ_DMA_CR3_OFFSET		0xE0C /* Configuration Register 3 */
#define XZYNQ_DMA_CR4_OFFSET		0xE10 /* Configuration Register 4 */
#define XZYNQ_DMA_CRDN_OFFSET		0xE14 /* Configuration Register Dn */

#define XZYNQ_DMA_PERIPH_ID_0_OFFSET	0xFE0 /* Peripheral Identification Register 0 */
#define XZYNQ_DMA_PERIPH_ID_1_OFFSET	0xFE4 /* Peripheral Identification Register 1 */
#define XZYNQ_DMA_PERIPH_ID_2_OFFSET	0xFE8 /* Peripheral Identification Register 2 */
#define XZYNQ_DMA_PERIPH_ID_3_OFFSET	0xFEC /* Peripheral Identification Register 3 */

#define XZYNQ_DMA_PCELL_ID_0_OFFSET		0xFF0 /* PrimeCell Identification Register 0 */
#define XZYNQ_DMA_PCELL_ID_1_OFFSET		0xFF4 /* PrimeCell Identification Register 1 */
#define XZYNQ_DMA_PCELL_ID_2_OFFSET		0xFF8 /* PrimeCell Identification Register 2 */
#define XZYNQ_DMA_PCELL_ID_3_OFFSET		0xFFC /* PrimeCell Identification Register 3 */

/*
 * Some useful register masks
 */
#define XZYNQ_DMA_DS_DMA_STATUS			0x0F /* DMA status mask */
#define XZYNQ_DMA_DS_DMA_STATUS_STOPPED	0x00 /* debug status busy mask */
#define XZYNQ_DMA_DBGSTATUS_BUSY		0x01 /* debug status busy mask */
#define XZYNQ_DMA_CS_ACTIVE_MASK		0x07 /* channel status active mask, last 3 bits of CS register */
#define XZYNQ_DMA_CR1_I_CACHE_LEN_MASK	0x07 /* i_cache_len mask */

/*
 * XZYNQ_DMA_DBGINST0 - constructs the word for the Debug Instruction-0 Register.
 * @b1: Instruction byte 1
 * @b0: Instruction byte 0
 * @ch: Channel number
 * @dbg_th: Debug thread encoding: 0 = DMA manager thread, 1 = DMA channel
 */
#define XZYNQ_DMA_DBGINST0(b1, b0, ch, dbg_th) \
	((((b1)&0xFF) << 24) | (((b0)&0xFF) << 16) | (((ch) & 0x7) << 8) | ((dbg_th & 0x1)))

#endif /* XZYNQ_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
