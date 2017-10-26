/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include <proto.h>

void fpga_enable_pcap(fpga_dev_t *dev)
{
	_Uint32t reg_val;
	reg_val = in32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET);

	out32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET, (reg_val
			| XZYNQ_FPGA_CTRL_PCAP_MODE_MASK
			| XZYNQ_FPGA_CTRL_PCAP_PR_MASK));
}

void fpga_disable_pcap(fpga_dev_t *dev)
{
	_Uint32t reg_val;

	reg_val = in32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET);

	out32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET, (reg_val
			& (~XZYNQ_FPGA_CTRL_PCAP_MODE_MASK)
			& (~XZYNQ_FPGA_CTRL_PCAP_PR_MASK)));
}

void fpga_enable_axi(fpga_dev_t *dev)
{
	out32(dev->scl_regbase + XZYNQ_SLCR_UNLOCK_OFFSET,
			XZYNQ_SLCR_UNLOCK_KEY);
	out32(dev->scl_regbase + XZYNQ_SLCR_LVL_SHFTR_EN_REG, 0xF); //TODO: Make these #defines
	out32(dev->scl_regbase + XZYNQ_SLCR_FPGA_RST_CTRL_REG, 0x00000000);
//	out32(dev->scl_regbase + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);
}

void fpga_disable_axi(fpga_dev_t *dev)
{
	out32(dev->scl_regbase + XZYNQ_SLCR_UNLOCK_OFFSET,
			XZYNQ_SLCR_UNLOCK_KEY);
	out32(dev->scl_regbase + XZYNQ_SLCR_FPGA_RST_CTRL_REG, 0x0000000F);
	out32(dev->scl_regbase + XZYNQ_SLCR_LVL_SHFTR_EN_REG, 0x0);
	out32(dev->scl_regbase + XZYNQ_SLCR_LVL_SHFTR_EN_REG, 0x0A);
//	out32(dev->scl_regbase + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);
}

void fpga_set_control_register(fpga_dev_t *dev, _Uint32t mask)
{
	_Uint32t reg_val;

	reg_val = in32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET);

	out32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET, (reg_val | mask));

}

_Uint32t fpga_get_control_register(fpga_dev_t *dev)
{
	/*
	 * Read the Control Register and return the value.
	 */
	return in32(dev->devcfg_regbase + XZYNQ_FPGA_CTRL_OFFSET);
}

void fpga_set_lock_register(fpga_dev_t *dev, _Uint32t data)
{
	out32(dev->devcfg_regbase + XZYNQ_FPGA_LOCK_OFFSET, data);

}

_Uint32t fpga_get_lock_register(fpga_dev_t *dev)
{
	/*
	 * Read the Lock Register and return the value.
	 */
	return in32(dev->devcfg_regbase + XZYNQ_FPGA_LOCK_OFFSET);
}

void fpga_set_config_register(fpga_dev_t *dev, _Uint32t data)
{
	out32(dev->devcfg_regbase + XZYNQ_FPGA_CFG_OFFSET, data);

}

_Uint32t fpga_get_config_register(fpga_dev_t *dev)
{
	return in32(dev->devcfg_regbase + XZYNQ_FPGA_CFG_OFFSET);

}

void fpga_set_status_register(fpga_dev_t *dev, _Uint32t data)
{
	out32(dev->devcfg_regbase + XZYNQ_FPGA_STATUS_OFFSET, data);

}

_Uint32t fpga_get_status_register(fpga_dev_t *dev)
{
	/*
	 * Read the Status Register and return the value.
	 */
	return in32(dev->devcfg_regbase + XZYNQ_FPGA_STATUS_OFFSET);
}

void fpga_set_rom_shadow_register(fpga_dev_t *dev, _Uint32t data)
{
	out32(dev->devcfg_regbase + XZYNQ_FPGA_ROM_SHADOW_OFFSET, data);

}

_Uint32t fpga_get_software_id_register(fpga_dev_t *dev)
{
	/*
	 * Read the Software ID Register and return the value.
	 */
	return in32(dev->devcfg_regbase + XZYNQ_FPGA_SW_ID_OFFSET);
}

void fpga_set_misc_control_register(fpga_dev_t *dev, _Uint32t mask)
{
	_Uint32t reg_val;

	reg_val = in32(dev->devcfg_regbase + XZYNQ_FPGA_MCTRL_OFFSET);

	out32(dev->devcfg_regbase + XZYNQ_FPGA_MCTRL_OFFSET, (reg_val | mask));
}

_Uint32t fpga_get_misc_control_register(fpga_dev_t *dev)
{
	/*
	 * Read the Miscellaneous Control Register and return the value.
	 */
	return in32(dev->devcfg_regbase + XZYNQ_FPGA_MCTRL_OFFSET);
}

_Uint32t fpga_is_dma_busy(fpga_dev_t *dev)
{

	_Uint32t reg_val;

	/* Read the PCAP status register for DMA status */
	reg_val = in32(dev->devcfg_regbase + XZYNQ_FPGA_STATUS_OFFSET);

	if ((reg_val & XZYNQ_FPGA_STATUS_DMA_CMD_Q_F_MASK)
			== XZYNQ_FPGA_STATUS_DMA_CMD_Q_F_MASK) {
		return XZYNQ_SUCCESS;
	}

	return XZYNQ_FAILURE;
}

paddr_t xzynq_vtophys(void *addr) //TODO: Add to header
{
	off64_t offset;

	if (mem_offset64(addr, NOFD, 1, &offset, 0) == -1) {
		return (-1);
	}
	return (offset);
}

void fpga_initiate_dma(fpga_dev_t *dev, _Uint32t src_ptr, _Uint32t dest_ptr,
		_Uint32t src_word_length, _Uint32t dest_word_length)
{
	out32(dev->devcfg_regbase + XZYNQ_FPGA_DMA_SRC_ADDR_OFFSET, src_ptr);

	out32(dev->devcfg_regbase + XZYNQ_FPGA_DMA_DEST_ADDR_OFFSET, dest_ptr);

	out32(dev->devcfg_regbase + XZYNQ_FPGA_DMA_SRC_LEN_OFFSET,
			src_word_length);

	out32(dev->devcfg_regbase + XZYNQ_FPGA_DMA_DEST_LEN_OFFSET,
			dest_word_length);
}

_Uint8t fpga_is_prog_done(fpga_dev_t *dev)
{
	return ((fpga_intr_get_status(dev) & XZYNQ_FPGA_IXR_PCFG_DONE_MASK)
			== XZYNQ_FPGA_IXR_PCFG_DONE_MASK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/fpga/lib.c $ $Rev: 752035 $")
#endif
