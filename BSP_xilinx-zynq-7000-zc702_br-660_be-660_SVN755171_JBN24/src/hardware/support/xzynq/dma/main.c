/*
 * $QNXLicenseC: 
 * Copyright 2013, QNX Software Systems.
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

#include "dma.h"
#include <sys/mman.h>

/* DMA context */
dma_ctx_t dma_ctx;

/* Lib init parameters */
static uint32_t regphys = DMA_BASE;
static uint32_t dma_size = DMA_SIZE;
static uint32_t irq = 0;
static char * dma_opts[] = { "irq", "regbase", NULL };

/* Mutex */
static pthread_mutex_t processinit_mutex = PTHREAD_MUTEX_INITIALIZER;
static int n_users_in_process = 0;

#define MAX_DESCRIPTORS                     1024

void register_init()
{
	uint32_t reg, channel;
	dma_ctx_t *p = &dma_ctx;

	if (dmasync_is_first_process()) {
		reg = in32(p->base_address + XZYNQ_DMA_CR1_OFFSET);
		p->cache_length = reg & XZYNQ_DMA_CR1_I_CACHE_LEN_MASK;
		if (p->cache_length < 2 || p->cache_length > 5)
			p->cache_length = 0;
		else
			p->cache_length = 1 << p->cache_length;

		memset(p->chans, 0, sizeof(dma_channel_t[DMA_N_CH]));

		for (channel = 0; channel < DMA_N_CH; channel++)
			p->chans[channel].channel_id = channel;
	}
}

int parse_init_options(const char *options)
{
	char *value;
	int opt;
	char *temp_ptr = NULL;

	if (options) {
		temp_ptr = strdup(options);
	}

	/* Getting the DMA Base addresss and irq from the Hwinfo Section if available */
	unsigned hwi_off = hwi_find_device(HWI_ITEM_DEVCLASS_DMA, 0);
	if (hwi_off != HWI_NULL_OFF) {
		hwi_tag *tag = hwi_tag_find(hwi_off, HWI_TAG_NAME_location, 0);
		if (tag) {
			regphys = tag->location.base;
			dma_size = tag->location.len;
			irq = hwitag_find_ivec(hwi_off, NULL);
		}
	}

	while (temp_ptr && *temp_ptr != '\0') {
		if ((opt = getsubopt(&temp_ptr, dma_opts, &value)) == -1)
			return -1;

		switch (opt) {
		case 0:
			irq = strtoul(value, 0, 0);
			break;
		case 1:
			regphys = strtoul(value, 0, 0);
			break;
		default:
			return -1;
		}
	}

	return EOK;
}

int parse_channel_options(dma_channel_t * chan_ptr, const char *options)
{
	/* RFU */
	return EOK;
}

int dma_init(const char *options)
{
	dma_ctx_t *p = &dma_ctx;

	if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
		perror("ThreadCtl");
		return NULL;
	}

	pthread_mutex_lock(&processinit_mutex);
	n_users_in_process++;

	/* Only need to initialize once per process */
	if (n_users_in_process == 1) {

		if (parse_init_options(options) != 0) {
			goto fail1;
		}

		/* init multi-process safety */
		if (dmasync_init() != 0) {
			goto fail1;
		}

		if (dmairq_init(irq) != 0) {
			goto fail3;
		}

		p->base_address = mmap_device_io(dma_size, regphys);
		if (p->base_address == (uintptr_t) MAP_FAILED) {
			goto fail2;
		}

		p->cachectl.fd = NOFD;
		cache_init(0, &p->cachectl, NULL);

		/* allow only 1 process to init the dma shared components at a time */
		pthread_mutex_lock(dmasync_libinit_mutex_get());
		dmasync_process_cnt_incr();

		register_init();

		pthread_mutex_unlock(dmasync_libinit_mutex_get()); // done share init
	}

	pthread_mutex_unlock(&processinit_mutex);

	return EOK;

	/* Cleanup on error */
	fail3: munmap_device_io(p->base_address, dma_size);
	fail2: dmasync_fini();
	fail1: n_users_in_process--;
	pthread_mutex_unlock(&processinit_mutex);
	return EFAIL;
}

void dma_fini()
{
	dma_ctx_t *p = &dma_ctx;
	pthread_mutex_lock(&processinit_mutex);
	n_users_in_process--;

	// if we are the last lib user in this process, then cleanup
	if (n_users_in_process == 0) {

		pthread_mutex_lock(dmasync_libinit_mutex_get());
		dmasync_process_cnt_decr();

		pthread_mutex_unlock(dmasync_libinit_mutex_get());
		dmairq_fini();
		cache_fini(&p->cachectl);
		munmap_device_io(p->base_address, dma_size);
		dmasync_fini();
	}
	pthread_mutex_unlock(&processinit_mutex);
}

void dma_query_channel(void *handle, dma_channel_query_t *chinfo)
{
	dma_channel_t * chan_ptr = handle;

	chinfo->chan_idx = chan_ptr->channel_id;
	chinfo->irq = irq_chan[chan_ptr->channel_id];
}

int dma_driver_info(dma_driver_info_t *info)
{
	info->dma_version_major = DMALIB_VERSION_MAJOR;
	info->dma_version_minor = DMALIB_VERSION_MINOR;
	info->dma_rev = DMALIB_REVISION;
	info->lib_rev = 0;
	info->description = DMA_DESCRIPTION_STR;
	info->num_channels = DMA_N_CH;

	return EOK;
}

int dma_channel_info(unsigned channel, dma_channel_info_t *info)
{
	info->max_xfer_size = 0xfffc;
	info->xfer_unit_sizes = 0x4;
	info->caps = DMA_CAP_EVENT_ON_COMPLETE | DMA_CAP_MEMORY_TO_MEMORY;
	info->mem_lower_limit = 0;
	info->mem_upper_limit = 0xffffffff;
	info->mem_nocross_boundary = 0;
	return EOK;
}

void *
dma_channel_attach(const char *optstring, const struct sigevent *event)
{
	dma_ctx_t *p = &dma_ctx;
	dma_channel_t * chan_ptr;
	rsrc_request_t req;
	unsigned channel_id;

	/* Get free channel from resource database manager */
	memset(&req, 0, sizeof(rsrc_request_t));
	req.start = DMA_CH_LO;
	req.end = DMA_CH_HI;
	req.length = 1;
	req.flags = RSRCDBMGR_DMA_CHANNEL | RSRCDBMGR_FLAG_RANGE;
	if (rsrcdbmgr_attach(&req, 1) != EOK) {
		goto fail1;
	}
	channel_id = req.start;

	/* Clear interrupts on this channel just in case it may still be set
	 * from a previous driver crash or something.
	 */
	out32(p->base_address + XZYNQ_DMA_INTCLR_OFFSET, (1 << channel_id));

	/* Create channel control struct, and populate with channel data structures */
	chan_ptr = &p->chans[channel_id];

	/* Init channel struct */
	chan_ptr->channel_id = channel_id;

	/* Attach OS event to the channel interrupt */
	dmairq_event_add(channel_id, event);

	/* Create buffers for dma program */
	chan_ptr->prog_buf_pool[0].buf = mmap(0, XZYNQ_DMA_CHAN_BUF_LEN, PROT_READ
			| PROT_WRITE | PROT_NOCACHE, MAP_PHYS | MAP_ANON | MAP_PRIVATE,
			NOFD, 0);
	chan_ptr->prog_buf_pool[1].buf = mmap(0, XZYNQ_DMA_CHAN_BUF_LEN, PROT_READ
			| PROT_WRITE | PROT_NOCACHE, MAP_PHYS | MAP_ANON | MAP_PRIVATE,
			NOFD, 0);

	return chan_ptr;

fail1:
	return NULL;
}

void dma_channel_release(void * handle)
{
	dma_ctx_t *p = &dma_ctx;
	dma_channel_t * chan_ptr = handle;
	rsrc_request_t req = { 0 };
	unsigned channel_id = chan_ptr->channel_id;
	uint32_t reg;

	/* Clear interrupts on this channel */
	out32(p->base_address + XZYNQ_DMA_INTCLR_OFFSET, (1 << channel_id));

	/* Release buffers for dma program */
	if (chan_ptr->prog_buf_pool[0].buf)
		munmap(chan_ptr->prog_buf_pool[0].buf, XZYNQ_DMA_CHAN_BUF_LEN);
	if (chan_ptr->prog_buf_pool[1].buf)
		munmap(chan_ptr->prog_buf_pool[1].buf, XZYNQ_DMA_CHAN_BUF_LEN);

	/* Unmap dma_cmd */
	if (chan_ptr->dma_cmd) {
		munmap(chan_ptr->dma_cmd, sizeof(dma_cmd_t));
		chan_ptr->dma_cmd = NULL;
	}

	/* Deactivate dma-events, default to software triggered */
	pthread_mutex_lock(dmasync_regmutex_get());
	reg = in32(p->base_address + XZYNQ_DMA_INTEN_OFFSET);
	reg &= ~(1 << channel_id);
	out32(p->base_address + XZYNQ_DMA_INTEN_OFFSET, reg);
	pthread_mutex_unlock(dmasync_regmutex_get());

	dmairq_callback_remove(chan_ptr->channel_id);

	dmairq_event_remove(chan_ptr->channel_id);

	req.length = 1;
	req.start = req.end = chan_ptr->channel_id;
	req.flags = RSRCDBMGR_DMA_CHANNEL;
	rsrcdbmgr_detach(&req, 1);
}

int dma_setup_xfer(void *handle, const dma_transfer_t *tinfo)
{
	dma_ctx_t *p = &dma_ctx;
	dma_channel_t * chan_ptr = handle;
	dma_cmd_t * cmd = chan_ptr->dma_cmd;
	uint32_t status;

	/* Check if this channel is already working */
	if (cmd) {
		return EBUSY;
	} else {
		cmd = mmap(0, sizeof(dma_cmd_t), PROT_READ | PROT_WRITE | PROT_NOCACHE,
				MAP_PHYS | MAP_ANON | MAP_PRIVATE, NOFD, 0);
		chan_ptr->dma_cmd = cmd;
	}

	/* Create the command for that transfer */
	cmd->chan_ctrl.src_burst_size = tinfo->xfer_unit_size;
	cmd->chan_ctrl.src_burst_len = tinfo->xfer_unit_size;
	cmd->chan_ctrl.src_inc = 1;
	cmd->chan_ctrl.dst_burst_size = tinfo->xfer_unit_size;
	cmd->chan_ctrl.dst_burst_len = tinfo->xfer_unit_size;
	cmd->chan_ctrl.dst_inc = 1;

	memcpy(&cmd->bd.src_addr, tinfo->src_addrs, sizeof(dma_addr_t));
	memcpy(&cmd->bd.dst_addr, tinfo->dst_addrs, sizeof(dma_addr_t));
	cmd->bd.len = tinfo->xfer_bytes;

	/* Check if the user provided a DMA program */
	if (tinfo->dma_prog && tinfo->dma_prog_len) {
		/* Allocating the prog buffer accordingly */
		cmd->user_dma_prog = mmap(0, sizeof(dma_cmd_t), PROT_READ | PROT_WRITE | PROT_NOCACHE,
						MAP_PHYS | MAP_ANON | MAP_PRIVATE, NOFD, 0);
		if (!cmd->user_dma_prog) {
			fprintf(stderr, "Couldn't map user dma program\n");
			return EINVAL;
		}

		memcpy(cmd->user_dma_prog, tinfo->dma_prog, tinfo->dma_prog_len);
		cmd->user_dma_prog_len = tinfo->dma_prog_len;
	} else {
		status = xzynq_gen_dma_prog(p, chan_ptr->channel_id, cmd);
		if (status) {
			return EINVAL;
		}
	}

	// TODO contloop

	return EOK;
}

int dma_xfer_start(void * handle)
{
	dma_ctx_t *p = &dma_ctx;
	dma_channel_t * chan_ptr = handle;
	dma_cmd_t * cmd = chan_ptr->dma_cmd;
	int status;
	uint32_t *dma_prog = NULL;
	uint32_t inten;
	off64_t dma_prog_phys;

	cmd->dma_status = -1;

	if (cmd->user_dma_prog)
		dma_prog = cmd->user_dma_prog;
	else if (cmd->gen_dma_prog)
		dma_prog = cmd->gen_dma_prog;

	if (dma_prog) {
		/* enable the interrupt */
		inten = in32(p->base_address + XZYNQ_DMA_INTEN_OFFSET);
		inten |= 0x01 << chan_ptr->channel_id; /* set the correpsonding bit */
		out32(p->base_address + XZYNQ_DMA_INTEN_OFFSET, inten);

		LOG("ch%d: src %x, dst %x, len %x\n", chan_ptr->channel_id, (uint32_t)cmd->bd.src_addr.paddr,
				(uint32_t)cmd->bd.dst_addr.paddr, cmd->bd.len);

		if (cmd->chan_ctrl.src_inc) {
			CACHE_FLUSH(&p->cachectl, cmd->bd.src_addr.vaddr,
					cmd->bd.src_addr.paddr, cmd->bd.len);
		}
		if (cmd->chan_ctrl.dst_inc) {
			CACHE_INVAL(&p->cachectl, cmd->bd.src_addr.vaddr,
					cmd->bd.src_addr.paddr, cmd->bd.len);
		}

		status = mem_offset64(dma_prog, NOFD, 1, &dma_prog_phys, 0);
		if (status != 0) {
			fprintf(stderr, "Unable to get dma program physical address\n");
			goto end;
		}

		status = xzynq_exec_DMAGO(p->base_address, chan_ptr->channel_id,
				dma_prog_phys);
	} else {
		munmap(p->chans[chan_ptr->channel_id].dma_cmd, sizeof(dma_cmd_t));
		p->chans[chan_ptr->channel_id].dma_cmd = NULL;
		status = EFAIL;
	}

end:
	return status;
}

int dma_xfer_abort(void * handle)
{
	dma_ctx_t *p = &dma_ctx;
	dma_channel_t * chan_ptr = handle;

	/* Kill the channel thread */
	xzynq_exec_DMAKILL(p->base_address, chan_ptr->channel_id, 1);

	return EOK;
}

int dma_xfer_complete(void * handle)
{
	dma_ctx_t *p = &dma_ctx;
	dma_channel_t * chan_ptr = handle;
	uint32_t reg;

	reg = in32(p->base_address + XZYNQ_DMA_CSn_OFFSET(chan_ptr->channel_id));

	if (reg & XZYNQ_DMA_CS_ACTIVE_MASK)
		return 0;
	else
		return 1;
}

int get_dmafuncs(dma_functions_t *functable, int tabsize)
{
	DMA_ADD_FUNC(functable, init, dma_init, tabsize);
	DMA_ADD_FUNC(functable, fini, dma_fini, tabsize);
	DMA_ADD_FUNC(functable, driver_info, dma_driver_info, tabsize);
	DMA_ADD_FUNC(functable, channel_info, dma_channel_info, tabsize);
	DMA_ADD_FUNC(functable, channel_attach, dma_channel_attach, tabsize);
	DMA_ADD_FUNC(functable, channel_release, dma_channel_release, tabsize);
	DMA_ADD_FUNC(functable, setup_xfer, dma_setup_xfer, tabsize);
	DMA_ADD_FUNC(functable, xfer_start, dma_xfer_start, tabsize);
	DMA_ADD_FUNC(functable, xfer_abort, dma_xfer_abort, tabsize);
	DMA_ADD_FUNC(functable, xfer_complete, dma_xfer_complete, tabsize);
	DMA_ADD_FUNC(functable, query_channel, dma_query_channel, tabsize);
	return EOK;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
