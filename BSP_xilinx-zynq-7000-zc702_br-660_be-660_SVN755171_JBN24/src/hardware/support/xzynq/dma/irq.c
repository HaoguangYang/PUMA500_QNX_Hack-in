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
#include "dma.h"

static uint32_t channel_mask; //channels that belong to THIS process
static const struct sigevent * event_array[DMA_N_CH];
static dmairq_callback_t callback_array[DMA_N_CH];
static int id_abort, id_chan[DMA_N_CH];

const uint32_t irq_chan[DMA_N_CH] = { XZYNQ_IRQ_DMA_CHAN0, XZYNQ_IRQ_DMA_CHAN1,
		XZYNQ_IRQ_DMA_CHAN2, XZYNQ_IRQ_DMA_CHAN3, XZYNQ_IRQ_DMA_CHAN4,
		XZYNQ_IRQ_DMA_CHAN5, XZYNQ_IRQ_DMA_CHAN6, XZYNQ_IRQ_DMA_CHAN7, };

const struct sigevent *irq_handler_abort(void *area, int id)
{
	void *dma_prog_buf;
	dma_ctx_t *p = &dma_ctx;
	dma_channel_t *chan_data;
	dma_cmd_t *dma_cmd;
	uint32_t fsm; /* Fault status DMA manager register value */
	uint32_t fsc; /* Fault status DMA channel register value */
	uint32_t fault_type; /* Fault type DMA manager register value */
	uint32_t pc; /* DMA pc or channel pc */
	unsigned channel;

	fsm = in32(p->base_address + XZYNQ_DMA_FSM_OFFSET) & 0x01;
	fsc = in32(p->base_address + XZYNQ_DMA_FSC_OFFSET) & 0xFF;

	if (fsm) {
		/* DMA manager is fault */
		fault_type = in32(p->base_address + XZYNQ_DMA_FTM_OFFSET);
		pc = in32(p->base_address + XZYNQ_DMA_DPC_OFFSET);
		/* Kill the DMA manager thread */
		/* Should we disable interrupt?*/
		xzynq_exec_DMAKILL(p->base_address, 0, 0);
	}

	/* Check which channel faults and kill the channel thread */
	for (channel = 0; channel < DMA_N_CH; channel++) {
		if (fsc & (0x01 << channel)) {
			fault_type = in32(p->base_address + XZYNQ_DMA_FTCn_OFFSET(channel));
			pc = in32(p->base_address + XZYNQ_DMA_CPCn_OFFSET(channel));

			/* Kill the channel thread */
			xzynq_exec_DMAKILL(p->base_address, channel, 1);

			/*
			 * Get the fault type and fault pc and invoke the
			 * fault callback.
			 */
			chan_data = p->chans + channel;

			dma_cmd = chan_data->dma_cmd;

			/* Should we check dma_cmd is not null */
			dma_cmd->dma_status = -1;
			dma_cmd->chan_fault_type = fault_type;
			dma_cmd->chan_fault_PC = pc;
			//chan_data->dma_cmd_from_hw = dma_cmd;
			//chan_data->dma_cmd_to_hw = NULL;

			if (!chan_data->hold_dma_prog) {
				dma_prog_buf = (void *) dma_cmd->gen_dma_prog;
				if (dma_prog_buf)
					xzynq_buf_pool_free(chan_data->prog_buf_pool, dma_prog_buf);
				dma_cmd->gen_dma_prog = NULL;
			}
		}
	}

	return NULL;
}

const struct sigevent *irq_handler_chan(void *area, int id)
{
	dma_ctx_t *p = &dma_ctx;
	uint32_t irq_status;
	uint32_t i;

	irq_status = in32(p->base_address + XZYNQ_DMA_INTSTATUS_OFFSET) & channel_mask;

	/* Find first active interrupt that belongs to this process */
	for (i = 0; i < DMA_N_CH; i++) {
		if (irq_status & (1 << i)) {
			/* Clear irq status bit i */
			out32(p->base_address + XZYNQ_DMA_INTCLR_OFFSET, (1 << i));

			/* Call the callback if present */
			if (callback_array[i]) {
				callback_array[i](i);
			}
			return event_array[i];
		}
	}
	return NULL;
}

int dmairq_init(uint32_t irq)
{
	int i;

	/* Attach abort IRQ */
	id_abort = InterruptAttach(irq, irq_handler_abort, NULL, 0,
			_NTO_INTR_FLAGS_TRK_MSK);
	if (id_abort < 0) {
		fprintf(stderr, "Unable to add interrupt #%d\n", id_abort);
		return -1;
	}

	/* Attach every channel IRQ */
	channel_mask = 0;
	for (i = 0; i < DMA_N_CH; i++) {
		event_array[i] = NULL;
		callback_array[i] = NULL;
		id_chan[i] = InterruptAttach(irq_chan[i], irq_handler_chan, NULL, 0,
				_NTO_INTR_FLAGS_TRK_MSK);
		if (id_chan[i] < 0) {
			fprintf(stderr, "Unable to add interrupt #%d\n", irq_chan[i]);
			return -1;
		}
	}

	return 0;
}

void dmairq_fini()
{
	int i;

	InterruptDetach(id_abort);

	for (i = 0; i < DMA_N_CH; i++)
		InterruptDetach(id_chan[i]);
}

void dmairq_event_add(uint32_t channel, const struct sigevent *event)
{
	atomic_set(&channel_mask, 1 << channel);
	event_array[channel] = event;
}

void dmairq_event_remove(uint32_t channel)
{
	atomic_clr(&channel_mask, 1 << channel);
	event_array[channel] = NULL;
}

void dmairq_callback_add(uint32_t channel, dmairq_callback_t func_ptr)
{
	callback_array[channel] = func_ptr;
}

void dmairq_callback_remove(uint32_t channel)
{
	callback_array[channel] = NULL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
