/*
 * $QNXLicenseC: 
 * Copyright 2007,2009 QNX Software Systems.  
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


#ifndef DMA_H
#define DMA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/siginfo.h>
#include <sys/mman.h>
#include <string.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <errno.h>
#include <atomic.h>
#include <fcntl.h>
#include <sys/rsrcdbmgr.h>
#include <sys/rsrcdbmsg.h>
#include <hw/dma.h>
#include <sys/rsrcdbmgr.h>
#include <sys/hwinfo.h>
#include <sys/cache.h>
#include <drvr/hwinfo.h>
#include <arm/xzynq.h>
#include "xzynq.h"

/* Controller Info */
#ifndef DMA_BASE
    #define DMA_BASE		XZYNQ_DMA_S_BASE_ADDR
#endif

#ifndef DMA_SIZE
    #define DMA_SIZE		XZYNQ_DMA_REG_SIZE
#endif

#ifndef DMA_IRQ
    #define DMA_IRQ			XZYNQ_IRQ_DMA_ABORT
#endif

#ifndef SDRAM_BASE
	#define SDRAM_BASE		0x00000000
#endif

#ifndef DMA_DESCRIPTION_STR    
    #define DMA_DESCRIPTION_STR    "PL330 DMA Controller"
#endif

#define EFAIL	-1

/* #define DEBUG_DMA */

#ifdef DEBUG_DMA
#define LOG(fmt, arg...) fprintf(stderr, fmt, ## arg)
#else
#define LOG(fmt, arg...)
#endif

/* Channel Configurations */
#define DMA_N_CH                   (XZYNQ_DMA_CHANNELS_PER_DEV)
#define DMA_CH_LO                  0
#define DMA_CH_HI                  (XZYNQ_DMA_CHANNELS_PER_DEV-1)
#define DMA_CH_PRIO_LO             0
#define DMA_CH_PRIO_HI             (XZYNQ_DMA_CHANNELS_PER_DEV-1)
#define DMA_CH_DEFAULT_PRIO        DMA_CH_PRIO_LO

/*
 * Data Structures
 */

/* Channels interrupts */
extern const uint32_t irq_chan[DMA_N_CH];

/** DMA channle control structure. It's for AXI bus transaction.
 * This struct will be translated into a 32-bit channel control register value.
 */
typedef struct {
	unsigned int endian_swap_size; /**< Endian swap size. */
	unsigned int dst_cache_ctrl; /**< Destination cache control */
	unsigned int dst_prot_ctrl; /**< Destination protection control */
	unsigned int dst_burst_len; /**< Destination burst length */
	unsigned int dst_burst_size; /**< Destination burst size */
	unsigned int dst_inc; /**< Destination incrementing or fixed address */
	unsigned int src_cache_ctrl; /**< Source cache control */
	unsigned int src_prot_ctrl; /**< Source protection control */
	unsigned int src_burst_len; /**< Source burst length */
	unsigned int src_burst_size; /**< Source burst size */
	unsigned int src_inc; /**< Source incrementing or fixed address */
} dma_chan_ctrl_t;

/** DMA block descriptor stucture.
 */
typedef struct {
	dma_addr_t	src_addr; /**< Source starting address */
	dma_addr_t	dst_addr; /**< Destination starting address */
	unsigned int len; /**< Number of bytes for the block */
} dma_bd_t;

#define XZYNQ_DMA_MAX_CHAN_BUFS	2
#define XZYNQ_DMA_CHAN_BUF_LEN	128

/**
 * The dma_prog_buf_t is the struct for a DMA program buffer.
 */
typedef struct {
	char *buf; /**< Buffer the holds the content */
	unsigned len; /**< The actual length of the DMA program in bytes. */
	int allocated; /**< A tag indicating whether the buffer is allocated or not */
} dma_prog_buf_t;

/**
 * A DMA command consisits of a channel control struct, a block descriptor,
 * a user defined program, a pointer pointing to generated DMA program, and
 * execution result.
 *
 */
typedef struct {
	dma_chan_ctrl_t chan_ctrl; /**< Channel Control Struct */
	dma_bd_t bd; /**< Buffer descriptor */
	void *user_dma_prog; /**< If user wants the driver to
					  *  execute their own DMA program,
					  *  this field points to the DMA
					  *  program.
					  */
	int user_dma_prog_len; /**< The length of user defined DMA program. */
	void *gen_dma_prog; /**< The DMA program genreated
					 * by the driver. This field will be
					 * set if a user invokes the DMA
					 * program generation function. Or
					 * the DMA command is finished and
					 * a user informs the driver not to
					 * release the program buffer.
					 * This field has two purposes, one
					 * is to ask the driver to generate
					 * a DMA program while the DMAC is
					 * performaning DMA transactions. The
					 * other purpose is to debug the
					 * driver.
					 */
	int gen_dma_prog_len; /**< The length of the DMA program generated */
	int dma_status; /**< 0 on success, otherwise error code */
	uint32_t chan_fault_type; /**< Channel fault type in case of fault */
	uint32_t chan_fault_PC; /**< Channel fault PC address */
} dma_cmd_t;

/**
 * It's the done handler a user can set for a channel
 */
typedef void (*dma_done_handler) (unsigned int channel,
				    dma_cmd_t *dma_cmd,
				    void *callbackf);

/**
 * The dma_channel_t is a struct to book keep individual channel of
 * the DMAC.
 */
typedef struct {
	unsigned channel_id; 		/**< Channel number of the DMAC */
	dma_prog_buf_t prog_buf_pool[XZYNQ_DMA_MAX_CHAN_BUFS]; /**< A pool of
							      program buffers*/
	dma_done_handler done_handler; /**< Done interrupt handler */
	void *done_ref;	/**< Done interrupt callback data */
	dma_cmd_t *dma_cmd; /**< DMA command being executed */
	int hold_dma_prog; /**< A tag indicating whether to hold the DMA after completion. */
} dma_channel_t;

/**
 * The dma_ctx_t driver instance data structure. A pointer to an instance data
 * structure is passed around by functions to refer to a specific driver
 * instance.
 */
typedef struct {
	uint32_t 	base_address;	/**< Configuration data structure */
    struct cache_ctrl cachectl;	/**< Cache control */
    int 		cache_length;	/**< icache length */
	dma_channel_t chans[DMA_N_CH]; /**< Channel data */
} dma_ctx_t;

/* DMA shared memory */
typedef struct {
    uint32_t process_cnt;
    pthread_mutex_t libinit_mutex;
    pthread_mutex_t command_mutex;
    pthread_mutex_t register_mutex;
} dma_shmem_t;

typedef void (*dmairq_callback_t)(unsigned); 

/* DMA context */
extern dma_ctx_t dma_ctx;

/*
 * Functions prototypes
 */
/* From sync.c */
int dmasync_init(void);
void dmasync_fini(void);
pthread_mutex_t * dmasync_cmdmutex_get();
pthread_mutex_t * dmasync_libinit_mutex_get();
pthread_mutex_t * dmasync_regmutex_get();
int dmasync_is_first_process();
int dmasync_is_last_process();
void dmasync_process_cnt_incr();
void dmasync_process_cnt_decr();
/* From irq.c */
int dmairq_init(uint32_t irq);
void dmairq_fini();
void dmairq_event_add(uint32_t channel, const struct sigevent *event);
void dmairq_event_remove(uint32_t channel);
void dmairq_callback_add(uint32_t channel,dmairq_callback_t func_ptr);
void dmairq_callback_remove(uint32_t channel);
/* From xzynq.c */
int xzynq_exec_DMAKILL(uint32_t base_addr, unsigned int channel,
		unsigned int thread);
int xzynq_exec_DMAGO(uint32_t base_addr, unsigned int channel,
		uint32_t dma_prog);
int xzynq_free_dma_prog(dma_ctx_t *p, unsigned int channel, dma_cmd_t *cmd);
void *xzynq_buf_pool_alloc(dma_prog_buf_t *pool);
void xzynq_buf_pool_free(dma_prog_buf_t *pool, void *Buf);
int xzynq_gen_dma_prog(dma_ctx_t *p, unsigned int channel, dma_cmd_t *cmd);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
