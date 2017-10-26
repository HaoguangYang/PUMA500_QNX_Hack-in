/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems.
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

#include "ipl.h"
#include "sdmmc.h"
#include "sdhc_xzynq.h"
#include "ipl_xzynq.h"
#include <hw/inout.h>
#include <stdio.h>

static
void delay(unsigned dly)
{
	volatile int j;

	while (dly--) {
		for (j = 0; j < 132; j++)
			;
	}
}

static
void *( memcpy_rom)(void * s1, const void * s2, unsigned n)
{
	char *dst = s1;
	const char *src = s2;

	/* Loop and copy.  */
	while (n-- != 0)
		*dst++ = *src++;
	return s1;
}

/*
 * SDMMC command table
 */
static const struct cmd_str {
	int cmd;
	unsigned sdmmc_cmd;
} cmdtab[] = {
// MMC_GO_IDLE_STATE
		{ 0, (0 << 8) | XZYNQ_SD_CMD_RESP_NONE },
		// MMC_SEND_OP_COND (R3)
		{ 1, (1 << 8) | XZYNQ_SD_CMD_RESP_48 },
		// MMC_ALL_SEND_CID (R2)
		{ 2, (2 << 8) | XZYNQ_SD_CMD_CRC | XZYNQ_SD_CMD_RESP_136 },
		// MMC_SET_RELATIVE_ADDR (R6)
		{ 3, (3 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48
				| XZYNQ_SD_CMD_CRC },
		// MMC_SWITCH (R1b)
		{ 5, (5 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48_BUSY
				| XZYNQ_SD_CMD_CRC },
		// MMC_SWITCH (R1b)
		{ 6, (6 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48_BUSY
				| XZYNQ_SD_CMD_CRC },
		// MMC_SEL_DES_CARD (R1)
		{ 7, (7 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48
				| XZYNQ_SD_CMD_CRC },
		// MMC_IF_COND (R1)
		{ 8, (8 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48
				| XZYNQ_SD_CMD_CRC },
		// MMC_SEND_CSD (R2)
		{ 9, (9 << 8) | XZYNQ_SD_CMD_CRC | XZYNQ_SD_CMD_RESP_136 },
		// MMC_SEND_STATUS (R1)
		{ 13, ((0x80 + 13) << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48
				| XZYNQ_SD_CMD_CRC },
		// MMC_SET_BLOCKLEN (R1)
		{ 16, (16 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48
				| XZYNQ_SD_CMD_CRC },
		// MMC_READ_SINGLE_BLOCK (R1)
		{ 17, (17 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48
				| XZYNQ_SD_CMD_CRC | XZYNQ_SD_CMD_DATA },
		// MMC_READ_MULTIPLE_BLOCK (R3)
		{ 18, (18 << 8) | XZYNQ_SD_CMD_RESP_48 },
		// MMC_APP_CMD (R1)
		{ 55, (55 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48
				| XZYNQ_SD_CMD_CRC },
		// SD_SET_BUS_WIDTH (R1)
		{ (55 << 8) | 6, (6 << 8) | XZYNQ_SD_CMD_INDEX | XZYNQ_SD_CMD_RESP_48
				| XZYNQ_SD_CMD_CRC },
		// SD_SEND_OP_COND (R3)
		{ (55 << 8) | 41, (41 << 8) | XZYNQ_SD_CMD_RESP_48 },
		// end of command list
		{ -1 }, };

/* searches for a command in the command table */
static inline struct cmd_str *
get_cmd(int cmd)
{
	struct cmd_str *command = (struct cmd_str *) &cmdtab[0];

	while (command->cmd != -1) {
		if (command->cmd == cmd)
			return command;
		command++;
	}

	return 0;
}

/* sets the sdmmc clock frequency */
static inline void sdmmc_set_frq(xzynq_sdmmc_t *sdmmc, unsigned frq)
{
	unsigned base = sdmmc->sdmmc_pbase;
	unsigned div, clk, reg;

	/* Turning off the clocks */
	reg = in16(base + XZYNQ_SD_CLK_CTL_R);
	reg &= ~(XZYNQ_SD_CLK_XZYNQ_SD_EN | XZYNQ_SD_CLK_INT_EN);
	out16(base + XZYNQ_SD_CLK_CTL_R, reg);

	/* Compute dividor value */
	div = (SDMMC_CLK_BASE / frq) >> 1;

	/* Enable Internal clock and wait for it to stabilized */
	clk = (div << XZYNQ_SD_DIV_SHIFT) | XZYNQ_SD_CLK_INT_EN;
	out16(base + XZYNQ_SD_CLK_CTL_R, clk);
	do {
		clk = in16(base + XZYNQ_SD_CLK_CTL_R);
	} while (!(clk & XZYNQ_SD_CLK_INT_STABLE));

	/* Enable SD clock */
	clk |= XZYNQ_SD_CLK_XZYNQ_SD_EN;
	out16(base + XZYNQ_SD_CLK_CTL_R, clk);
}

/* sets the data bus width */
static inline void sd_set_bus_width(xzynq_sdmmc_t *sdmmc, int width)
{
	unsigned base = sdmmc->sdmmc_pbase;
	unsigned tmp;

	tmp = in32(base + XZYNQ_SD_HOST_CTRL_R); /* Read the register again */

	if (width == 4) {
		out32(base + XZYNQ_SD_HOST_CTRL_R, tmp | XZYNQ_SD_DATA_SD4);
	} else {
		out32(base + XZYNQ_SD_HOST_CTRL_R, tmp & ~XZYNQ_SD_DATA_SD4);
	}
}

/* initializes the SDMMC controller */
int sdmmc_init_ctrl(xzynq_sdmmc_t *sdmmc)
{
	unsigned base = sdmmc->sdmmc_pbase;

	sdmmc->icr = 0x000001aa; /* 2.7 V - 3.6 V */
	sdmmc->ocr = 0x00300000; /* 3.2 V - 3.4 V */

	/* Power off the card */
	out8(base + XZYNQ_SD_PWR_CTRL_R, 0);

	/* Disable interrupts */
	out32(base + XZYNQ_SD_SIG_ENA_R, 0);

	/* Perform soft reset */
	out8(base + XZYNQ_SD_SOFT_RST_R, XZYNQ_SD_RST_ALL);
	/* Wait for reset to comlete */
	while (in8(base + XZYNQ_SD_SOFT_RST_R))
		;

	/* Power on the card */
	out8(base + XZYNQ_SD_PWR_CTRL_R, XZYNQ_SD_POWER_33 | XZYNQ_SD_POWER_ON);

	/* Wait for card detected */
	while (!(in32(base + XZYNQ_SD_PRES_STATE_R) & (XZYNQ_SD_CARD_DB
			| XZYNQ_SD_CARD_INS | XZYNQ_SD_CARD_DPL)))
		;

	/* Set/enable clock to default value */
	sdmmc_set_frq(sdmmc, SDMMC_CLK_DEFAULT);

	/* Re-enable interrupts */
	out32(base + XZYNQ_SD_SIG_ENA_R, 0xFFFFFFFF);
	out32(base + XZYNQ_SD_INT_ENA_R, 0xFFFFFFFF);

	return SDMMC_OK;
}

/* clean up the SDMMC controller */
int sdmmc_fini(xzynq_sdmmc_t *sdmmc)
{
	unsigned base = sdmmc->sdmmc_pbase;
	unsigned short reg;

	/* Power off the card */
	out8(base + XZYNQ_SD_PWR_CTRL_R, 0);

	/* Disable interrupts */
	out32(base + XZYNQ_SD_SIG_ENA_R, 0);

	/* Turning off the clocks */
	reg = in16(base + XZYNQ_SD_CLK_CTL_R);
	reg &= ~(XZYNQ_SD_CLK_XZYNQ_SD_EN | XZYNQ_SD_CLK_INT_EN);
	out16(base + XZYNQ_SD_CLK_CTL_R, reg);

	return SDMMC_OK;
}

/* setup DMA for data read */
static unsigned sd_dma_buffer[129] __attribute__ ((aligned (512)));
static inline void xzynq_setup_dma_read(xzynq_sdmmc_t *sdmmc)
{
	unsigned short mode;

	/* Set the DMA address to the DMA buffer. */
	out32(sdmmc->sdmmc_pbase + XZYNQ_SD_DMA_ADDR_R,
			(unsigned int) sd_dma_buffer);

	/* 512 byte block size. */
	out16(sdmmc->sdmmc_pbase + XZYNQ_SD_BLOCK_SZ_R, sdmmc->cmd.bsize);
	out16(sdmmc->sdmmc_pbase + XZYNQ_SD_BLOCK_CNT_R, sdmmc->cmd.bcnt);

	out8(sdmmc->sdmmc_pbase + XZYNQ_SD_TIMEOUT_CTL_R, 0xA);

	out32(sdmmc->sdmmc_pbase + XZYNQ_SD_ARG_R, sdmmc->cmd.arg);

	/* Set the transfer mode to read, simple DMA, single block
	 * (applicable only to data commands)
	 * This is all that this software supports.
	 */
	mode = XZYNQ_SD_TRNS_BLK_CNT_EN | XZYNQ_SD_TRNS_READ | XZYNQ_SD_TRNS_DMA;
	if (sdmmc->cmd.bcnt > 1) {
		mode |= XZYNQ_SD_TRNS_MULTI;
	}
	out16(sdmmc->sdmmc_pbase + XZYNQ_SD_TRNS_MODE_R, mode);

}

/* issues a command on the SDMMC bus */
static inline int sdmmc_send_cmd(xzynq_sdmmc_t *sdmmc)
{
	struct cmd_str *command;
	int data_txf = 0;
	unsigned temp;
	unsigned base = sdmmc->sdmmc_pbase;
	sdmmc_cmd_t *cmd = &sdmmc->cmd;

	if (0 == (command = get_cmd(cmd->cmd)))
		return SDMMC_ERROR;

	/* check if need data transfer */
	data_txf = command->sdmmc_cmd & XZYNQ_SD_CMD_DATA;

	/* Check that command line is not in use */
	while ((in32(base + XZYNQ_SD_PRES_STATE_R) & XZYNQ_SD_CMD_INHIBIT)
			|| (in32(base + XZYNQ_SD_PRES_STATE_R) & XZYNQ_SD_DATA_INHIBIT))
		;

	/* Check that data line is ready to use */
	while (in32(base + XZYNQ_SD_PRES_STATE_R) & XZYNQ_SD_DATA_ACTIVE)
		;

	xzynq_setup_dma_read(sdmmc);

	/* clear SDMMC interrupt status register, write 1 to clear */
	out32(base + XZYNQ_SD_INT_STAT_R, 0xffffffff);

	/* Initiate the command */
	out16(base + XZYNQ_SD_CMD_R, (unsigned short) command->sdmmc_cmd);

	/* wait for command finish */
	while (!(in32(base + XZYNQ_SD_INT_STAT_R) & (XZYNQ_SD_INT_CMD_CMPL
			| XZYNQ_SD_INT_ERROR)))
		;

	/* check error status */
	temp = in32(base + XZYNQ_SD_INT_STAT_R);
	if (temp & XZYNQ_SD_INT_ERROR) {
		ser_putstr("Command send error:");
		ser_puthex(command->sdmmc_cmd);
		ser_putstr("\n");
		cmd->erintsts = temp;
		return SDMMC_ERROR;
	}
	/* get command response */
	if (cmd->rsp != 0) {
		cmd->rsp[0] = in32(base + XZYNQ_SD_RSP_0);

		if ((command->sdmmc_cmd & XZYNQ_SD_CMD_RESP_136)
				== XZYNQ_SD_CMD_RESP_136) {
			cmd->rsp[1] = in32(base + XZYNQ_SD_RSP_1);
			cmd->rsp[2] = in32(base + XZYNQ_SD_RSP_2);
			cmd->rsp[3] = in32(base + XZYNQ_SD_RSP_3);
			/*
			 * CRC is not included in the response register,
			 * we have to left shift 8 bit to match the 128 CID/CSD structure
			 */
			cmd->rsp[3] = (cmd->rsp[3] << 8) | (cmd->rsp[2] >> 24);
			cmd->rsp[2] = (cmd->rsp[2] << 8) | (cmd->rsp[1] >> 24);
			cmd->rsp[1] = (cmd->rsp[1] << 8) | (cmd->rsp[0] >> 24);
			cmd->rsp[0] = (cmd->rsp[0] << 8);
		}
	}

	if (data_txf) {
		while (1) {
			temp = in32(base + XZYNQ_SD_INT_STAT_R);

			/* Check error status */
			if (temp & XZYNQ_SD_INT_ERROR) {
				/* Perform command reset */
				out8(base + XZYNQ_SD_SOFT_RST_R, XZYNQ_SD_RST_CMD);
				while (in8(base + XZYNQ_SD_SOFT_RST_R))
					;
				return SDMMC_ERROR;
			}

			/* Command complete? */
			if (temp & XZYNQ_SD_INT_CMD_CMPL) {
				if (command->sdmmc_cmd & XZYNQ_SD_CMD_DATA) {
					if (temp & (XZYNQ_SD_INT_DMA | XZYNQ_SD_INT_TRNS_CMPL)) {
						/* DMA transfer complete */
						break;
					}
				} else {
					/* Non-DMA transfer complete */
					break;
				}
			}
		}
		memcpy_rom(sdmmc->cmd.dbuf, &sd_dma_buffer[0], sdmmc->cmd.bsize);
	}

	return SDMMC_OK;
}

/* Gets the card state */
static inline int sdmmc_get_state(xzynq_sdmmc_t *sdmmc)
{
	unsigned rsp;

	CMD_CREATE (sdmmc->cmd, MMC_SEND_STATUS, sdmmc->card.rca << 16, &rsp, 0, 0, 0);

	if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
		return SDMMC_ERROR;

	/* Bits 9-12 define the CURRENT_STATE */
	sdmmc->card.state = rsp & 0x1e00;

	return SDMMC_OK;
}

/* Check OCR for compliance with controller OCR */
static inline int sdmmc_check_ocr(xzynq_sdmmc_t *sdmmc, unsigned ocr)
{
	if (ocr & sdmmc->ocr)
		return SDMMC_OK;

	CMD_CREATE (sdmmc->cmd, MMC_GO_IDLE_STATE, 0, 0, 0, 0, 0);

	sdmmc_send_cmd(sdmmc);

	return SDMMC_ERROR;
}
/* Parse card speed */
static inline unsigned sdmmc_tran_speed(unsigned char ts)
{
	static const unsigned sdmmc_ts_exp[] = { 100, 1000, 10000, 100000, 0, 0, 0,
			0 };
	static const unsigned sdmmc_ts_mul[] = { 0, 1000, 1200, 1300, 1500, 2000,
			2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000 };

	return sdmmc_ts_exp[(ts & 0x7)] * sdmmc_ts_mul[(ts & 0x78) >> 3];
}

/* parse SD CSD */
static inline int sd_parse_csd(xzynq_sdmmc_t *sdmmc, unsigned *resp)
{
	card_t *card = &sdmmc->card;
	sd_csd_t *csd = &sdmmc->csd.sd_csd;
	int bsize, csize, csizem;

	csd->csd_structure = resp[3] >> 30;
	csd->taac = resp[3] >> 16;
	csd->nsac = resp[3] >> 8;
	csd->tran_speed = resp[3];
	csd->ccc = resp[2] >> 20;
	csd->read_bl_len = (resp[2] >> 16) & 0x0F;
	csd->read_bl_partial = (resp[2] >> 15) & 1;
	csd->write_blk_misalign = (resp[2] >> 14) & 1;
	csd->read_blk_misalign = (resp[2] >> 13) & 1;
	csd->dsr_imp = (resp[2] >> 12) & 1;

	if (csd->csd_structure == 0) {
		csd->csd.csd_ver1.c_size = ((resp[2] & 0x3FF) << 2) | (resp[1] >> 30);
		csd->csd.csd_ver1.vdd_r_curr_min = (resp[1] >> 27) & 0x07;
		csd->csd.csd_ver1.vdd_r_curr_max = (resp[1] >> 24) & 0x07;
		csd->csd.csd_ver1.vdd_w_curr_min = (resp[1] >> 21) & 0x07;
		csd->csd.csd_ver1.vdd_w_curr_max = (resp[1] >> 18) & 0x07;
		csd->csd.csd_ver1.c_size_mult = (resp[1] >> 15) & 0x07;
	} else {
		csd->csd.csd_ver2.c_size = ((resp[2] & 0x3F) << 16) | (resp[1] >> 16);
	}
	csd->erase_blk_en = (resp[1] >> 14) & 1;
	csd->sector_size = (resp[1] >> 7) & 0x7F;
	csd->wp_grp_size = (resp[1] >> 0) & 0x7F;
	csd->wp_grp_enable = (resp[0] >> 31);
	csd->r2w_factor = (resp[0] >> 26) & 0x07;
	csd->write_bl_len = (resp[0] >> 22) & 0x0F;
	csd->write_bl_partial = (resp[0] >> 21) & 1;
	csd->file_format_grp = (resp[0] >> 15) & 1;
	csd->copy = (resp[0] >> 14) & 1;
	csd->perm_write_protect = (resp[0] >> 13) & 1;
	csd->tmp_write_protect = (resp[0] >> 12) & 1;
	csd->file_format = (resp[0] >> 10) & 3;

	if (csd->perm_write_protect || csd->tmp_write_protect) {
		ser_putstr("SD Card write_protect\n");
	}

	if (csd->csd_structure == 0) {
		bsize = 1 << csd->read_bl_len;
		csize = csd->csd.csd_ver1.c_size + 1;
		csizem = 1 << (csd->csd.csd_ver1.c_size_mult + 2);
	} else {
		bsize = SDMMC_BLOCKSIZE;
		csize = csd->csd.csd_ver2.c_size + 1;
		csizem = 1024;
	}

	/* force to 512 byte block */
	if (bsize > SDMMC_BLOCKSIZE && (bsize % SDMMC_BLOCKSIZE) == 0) {
		unsigned ts = bsize / SDMMC_BLOCKSIZE;
		csize = csize * ts;
		bsize = SDMMC_BLOCKSIZE;
	}

	card->blk_size = bsize;
	card->blk_num = csize * csizem;
	card->speed = sdmmc_tran_speed(csd->tran_speed);

	sdmmc_set_frq(sdmmc, card->speed);

	if (SDMMC_OK != sdmmc_get_state(sdmmc))
		return SDMMC_ERROR;

	return SDMMC_OK;

}

/* parse SD CID */
static inline void sd_parse_cid(xzynq_sdmmc_t *sdmmc, unsigned *resp)
{
	sd_cid_t *cid = &sdmmc->cid.sd_cid;

	cid->mid = resp[3] >> 24;
	cid->oid[0] = (resp[3] >> 8) & 0xFF;
	cid->oid[1] = (resp[3] >> 16) & 0xFFFF;
	cid->oid[2] = 0;
	cid->pnm[0] = resp[3];
	cid->pnm[1] = resp[2] >> 24;
	cid->pnm[2] = resp[2] >> 16;
	cid->pnm[3] = resp[2] >> 8;
	cid->pnm[4] = resp[2];
	cid->pnm[5] = 0;
	cid->prv = resp[1] >> 24;
	cid->psn = (resp[1] << 8) | (resp[0] >> 24);
	cid->mdt = (resp[0] >> 8) & 0xFFFF;
}

/* parse MMC CSD */
static inline void mmc_parse_csd(xzynq_sdmmc_t *sdmmc, unsigned *resp)
{
	card_t *card = &sdmmc->card;
	mmc_csd_t *csd = &sdmmc->csd.mmc_csd;
	int bsize, csize, csizem;

	csd->csd_structure = resp[3] >> 30;
	csd->mmc_prot = (resp[3] >> 26) & 0x0F;
	csd->tran_speed = resp[3];
	csd->read_bl_len = (resp[2] >> 16) & 0x0F;
	csd->c_size = ((resp[2] & 0x3FF) << 2) | (resp[1] >> 30);
	csd->c_size_mult = (resp[1] >> 15) & 0x07;

	bsize = 1 << csd->read_bl_len;
	csize = csd->c_size + 1;
	csizem = 1 << (csd->c_size_mult + 2);

	card->blk_size = bsize;
	card->blk_num = csize * csizem;
}

/* parse MMC CID */
static inline void mmc_parse_cid(xzynq_sdmmc_t *sdmmc, unsigned *resp)
{
	mmc_cid_t *cid = &sdmmc->cid.mmc_cid;
	mmc_csd_t *csd = &sdmmc->csd.mmc_csd;

	cid->pnm[0] = resp[3];
	cid->pnm[1] = resp[2] >> 24;
	cid->pnm[2] = resp[2] >> 16;
	cid->pnm[3] = resp[2] >> 8;
	cid->pnm[4] = resp[2];
	cid->pnm[5] = resp[1] >> 24;

	if (csd->mmc_prot < 2) {
		cid->mid = resp[3] >> 8;
		cid->pnm[6] = resp[1] >> 16;
		cid->pnm[7] = 0;
		cid->hwr = (resp[1] >> 12) & 0x0F;
		cid->fwr = (resp[1] >> 8) & 0x0F;
	} else {
		cid->mid = resp[3] >> 24;
		cid->oid = (resp[3] >> 8) & 0xFFFF;
		cid->pnm[6] = 0;
	}

	cid->psn = ((resp[1] & 0xFFFF) << 16) | (resp[0] >> 16);
	cid->mcd = (resp[0] >> 12) & 0x0F;
	cid->ycd = ((resp[0] >> 8) & 0x0F) + 1997;
}

/* MMC switch mode */
static inline int mmc_switch_mode(xzynq_sdmmc_t *sdmmc)
{
	// We know the chip is in 4 bit mode and support high speed (50MHz)

	// switch to 4 bit mode
	CMD_CREATE (sdmmc->cmd, MMC_SWITCH, (3 << 24) | (183 << 16) | (2 << 8), 0, 0, 0, 0);
	if (SDMMC_OK == sdmmc_send_cmd(sdmmc))
		sd_set_bus_width(sdmmc, 4);
	else
		return -1;

	// switch to high speed mode
	CMD_CREATE (sdmmc->cmd, MMC_SWITCH, (3 << 24) | (185 << 16) | (1 << 8) | 1, 0, 0, 0, 0);
	if (SDMMC_OK == sdmmc_send_cmd(sdmmc))
		sdmmc_set_frq(sdmmc, 50000000);
	else
		return -1;

	return SDMMC_OK;
}

/* SDMMC card identification and initialisation */
int sdmmc_init_card(xzynq_sdmmc_t *sdmmc)
{
	int i;
	card_t *card = &sdmmc->card;
	unsigned rsp[4], cid[4];

	/* initial relative card address */
	card->rca = 0;

	/* Probe for SDC card */
	CMD_CREATE(sdmmc->cmd, MMC_GO_IDLE_STATE, 0, 0, 0, 0, 0);
	if (SDMMC_OK != sdmmc_send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	delay(1000);

	card->state = MMC_IDLE;

	CMD_CREATE (sdmmc->cmd, MMC_IF_COND, sdmmc->icr, 0, 0, 0, 0);
	if (SDMMC_OK == sdmmc_send_cmd(sdmmc)) {
		card->type = eSDC_V200;
	} else {
		CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, 0, 0, 0, 0, 0);
		if (SDMMC_OK == sdmmc_send_cmd(sdmmc)) {
			card->type = eSDC;
		} else {
			card->type = eMMC;
		}
	}

	/* Start Power Up process and get Operation Condition Register */
	CMD_CREATE (sdmmc->cmd, MMC_GO_IDLE_STATE, 0, 0, 0, 0, 0);
	if (SDMMC_OK != sdmmc_send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	delay(1000);

	switch (card->type) {
	case eSDC_V200:
		CMD_CREATE (sdmmc->cmd, MMC_IF_COND, sdmmc->icr, 0, 0, 0, 0);
		if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
			return SDMMC_ERROR;

		for (i = 0; i < POWER_UP_WAIT; i++) {
			/* when ACMDx(SD_SEND_OP_COND) is to be issued, send CMD55 first */
			CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, card->rca << 16, 0, 0, 0, 0);
			if (SDMMC_OK != sdmmc_send_cmd(sdmmc)) {
				return SDMMC_ERROR;
			}

			CMD_CREATE (sdmmc->cmd, SD_SEND_OP_COND, 0xc0000000 | sdmmc->ocr, rsp, 0, 0, 0);
			if (SDMMC_OK != sdmmc_send_cmd(sdmmc)) {
				return SDMMC_ERROR;
			}

			if ((i == 0) && (SDMMC_OK != sdmmc_check_ocr(sdmmc, rsp[0]))) {
				return SDMMC_ERROR;
			}

			if ((rsp[0] & 0x80000000) == 0x80000000) // check if card power up process finished
				break;

			delay(1000);
		}

		/* test for HC bit set */
		if ((rsp[0] & 0x40000000) == 0x40000000) {
			card->type = eSDC_HC;
		} else // no HC card detected
		{
			card->type = eSDC;
		}
		break;

	case eSDC:
		CMD_CREATE (sdmmc->cmd, SD_SEND_OP_COND, 0x80000000 | sdmmc->ocr, rsp, 0, 0, 0);
		for (i = 0; i < POWER_UP_WAIT; i++) {
			if (SDMMC_OK != sdmmc_send_cmd(sdmmc)) {
				return SDMMC_ERROR;
			}

			if ((i == 0) && (SDMMC_OK != sdmmc_check_ocr(sdmmc, rsp[0]))) {
				return SDMMC_ERROR;
			}

			if (rsp[0] & 0x80000000) // check if card power up process finished
				break;

			delay(1000);
		}
		break;

	case eMMC:
		CMD_CREATE (sdmmc->cmd, MMC_SEND_OP_COND, 0xc0000000 | sdmmc->ocr, rsp, 0, 0, 0);
		for (i = 0; i < POWER_UP_WAIT; i++) {
			if (SDMMC_OK != sdmmc_send_cmd(sdmmc)) {
				return SDMMC_ERROR;
			}

			if ((i == 0) && (SDMMC_OK != sdmmc_check_ocr(sdmmc, rsp[0]))) {
				return SDMMC_ERROR;
			}

			if (rsp[0] & 0x80000000) // check if card power up process finished
				break;

			delay(1000);
		}
		/* test for HC bit set */
		if ((rsp[0] & 0x40000000) == 0x40000000)
			card->type = eMMC_HC;
		break;

	default:
		return SDMMC_ERROR;
	}

	/* check if time out */
	if (i >= POWER_UP_WAIT) {
		return SDMMC_ERROR;
	}

	card->state = MMC_READY;

	/* Identification of Device */
	CMD_CREATE (sdmmc->cmd, MMC_ALL_SEND_CID, 0, rsp, 0, 0, 0);
	if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
		return SDMMC_ERROR;

	card->state = MMC_IDENT;
	cid[0] = rsp[0], cid[1] = rsp[1], cid[2] = rsp[2], cid[3] = rsp[3];

	/* SDMMC relative address */
	switch (card->type) {
	case eSDC:
	case eSDC_HC:
		card->rca = 0x0001;
		CMD_CREATE (sdmmc->cmd, MMC_SET_RELATIVE_ADDR, card->rca << 16, rsp, 0, 0, 0);
		if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
			return SDMMC_ERROR;

		card->rca = rsp[0] >> 16;
		if (SDMMC_OK != sdmmc_get_state(sdmmc))
			return SDMMC_ERROR;
		break;

	case eMMC:
	case eMMC_HC:
		card->rca = 0x0001;
		CMD_CREATE (sdmmc->cmd, MMC_SET_RELATIVE_ADDR, card->rca << 16, rsp, 0, 0, 0);
		if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
			return SDMMC_ERROR;

		if (SDMMC_OK != sdmmc_get_state(sdmmc))
			return SDMMC_ERROR;
		break;

	default:
		return SDMMC_ERROR;
	}

	/* Get card CSD */
	CMD_CREATE (sdmmc->cmd, MMC_SEND_CSD, card->rca << 16, rsp, 0, 0, 0);
	if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
		return SDMMC_ERROR;

	/* Parse CSD and CID */
	switch (card->type) {
	case eSDC:
	case eSDC_HC:
		if (SDMMC_OK != sd_parse_csd(sdmmc, rsp))
			return SDMMC_ERROR;

		sd_parse_cid(sdmmc, cid);
		break;

	case eMMC:
	case eMMC_HC:
		mmc_parse_csd(sdmmc, rsp);
		mmc_parse_cid(sdmmc, cid);
		break;

	default:
		return SDMMC_ERROR;
	}

	/* Select the Card */
	CMD_CREATE (sdmmc->cmd, MMC_SEL_DES_CARD, card->rca << 16, 0, 0, 0, 0);
	if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
		return SDMMC_ERROR;

	if (SDMMC_OK != sdmmc_get_state(sdmmc))
		return SDMMC_ERROR;

	/* Set block size in case the default blocksize differs from 512 */
	card->blk_num *= card->blk_size / SDMMC_BLOCKSIZE;
	card->blk_size = SDMMC_BLOCKSIZE;

	CMD_CREATE (sdmmc->cmd, MMC_SET_BLOCKLEN, card->blk_size, 0, 0, 0, 0);
	if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
		return SDMMC_ERROR;

	if (SDMMC_OK != sdmmc_get_state(sdmmc))
		return SDMMC_ERROR;

	delay(1000);

	/* Switch mode */
	switch (card->type) {
	case eSDC:
	case eSDC_HC:
		/* when ACMDx(SD_SEND_OP_COND) is to be issued, send CMD55 first */
		CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, card->rca << 16, 0, 0, 0, 0);
		if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
			return SDMMC_ERROR;

		/* switch to 4 bits bus */
		CMD_CREATE (sdmmc->cmd, SD_SET_BUS_WIDTH, 2, 0, 0, 0, 0);
		if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
			return SDMMC_ERROR;

		sd_set_bus_width(sdmmc, 4);
		break;

	case eMMC:
	case eMMC_HC:
		if (SDMMC_OK != mmc_switch_mode(sdmmc))
			return SDMMC_ERROR;
		break;

	default: /* impossible */
		return SDMMC_ERROR;
	}

	delay(1000);

	return SDMMC_OK;
}

/* read blocks from SDMMC */
int sdmmc_read(xzynq_sdmmc_t *sdmmc, void *buf, unsigned blkno, unsigned blkcnt)
{
	int cmd, i;
	unsigned arg;

	if ((blkno >= sdmmc->card.blk_num) || ((blkno + blkcnt)
			> sdmmc->card.blk_num) || ((blkno + blkcnt) < blkno)) {
		return SDMMC_ERROR;
	}

	if (blkcnt == 0)
		return SDMMC_OK;

	cmd = MMC_READ_SINGLE_BLOCK;

	for (i=0; i<blkcnt; i++) {
		arg = blkno+i;

		if ((sdmmc->card.type == eMMC) || (sdmmc->card.type == eSDC))
			arg *= SDMMC_BLOCKSIZE;

		CMD_CREATE (sdmmc->cmd, cmd, arg, 0, SDMMC_BLOCKSIZE, 1, buf + i*SDMMC_BLOCKSIZE);

		if (SDMMC_OK != sdmmc_send_cmd(sdmmc)) {
			return SDMMC_ERROR;
		}

		while (sdmmc->card.state != MMC_TRAN) {
			if (SDMMC_OK != sdmmc_get_state(sdmmc)) {
				return SDMMC_ERROR;
			}
		}
	}

	return SDMMC_OK;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
