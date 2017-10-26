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

#ifndef XZYNQ_EMACPS_BDRING_H   /* prevent curcular inclusions */
#define XZYNQ_EMACPS_BDRING_H   /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/** This is an internal structure used to maintain the DMA list */
typedef struct {
	uint32_t phys_base_addr;        /**< Physical address of 1st BD in list */
	uint32_t base_bd_addr;          /**< Virtual address of 1st BD in list */
	uint32_t high_bd_addr;          /**< Virtual address of last BD in the list */
	uint32_t length;                /**< Total size of ring in bytes */
	uint32_t separation;            /**< Number of bytes between the starting address
	                                                                   of adjacent BDs */
	emac_bd *free_head;
	/**< First BD in the free group */
	emac_bd *pre_head;
	/**< First BD in the pre-work group */
	emac_bd *hw_head;
	/**< First BD in the work group */
	emac_bd *hw_tail;
	/**< Last BD in the work group */
	emac_bd *post_head;
	/**< First BD in the post-work group */
	emac_bd *bda_restart;
	/**< BDA to load when channel is started */
	unsigned hw_cnt;        /**< Number of BDs in work group */
	unsigned pre_cnt;
	/**< Number of BDs in pre-work group */
	unsigned free_cnt;
	/**< Number of allocatable BDs in the free group */
	unsigned post_cnt;
	/**< Number of BDs in post-work group */
	unsigned all_cnt;
	/**< Total Number of BDs for channel */
} emac_bdring;

#define emac_bdring_next(ring_ptr, bd_ptr)				\
	(((uint32_t)(bd_ptr) >= (ring_ptr)->high_bd_addr) ?			   \
	 (emac_bd *)(ring_ptr)->base_bd_addr :				   \
	 (emac_bd *)((uint32_t)(bd_ptr) + (ring_ptr)->separation))

#define emac_bdring_prev(ring_ptr, bd_ptr)				\
	(((uint32_t)(bd_ptr) <= (ring_ptr)->base_bd_addr) ?			   \
	 (emac_bd *)(ring_ptr)->high_bd_addr :				   \
	 (emac_bd *)((uint32_t)(bd_ptr) - (ring_ptr)->separation))

/*
 * Scatter gather DMA related functions in xemacps_bdring.c
 */
int emac_bdring_create(emac_bdring *ring_ptr, uint32_t phys_addr,
		       uint32_t virt_addr, uint32_t alignment,
		       unsigned bd_count);
int emac_bdring_clone(emac_bdring *ring_ptr, emac_bd *src_bd_ptr,
		      uint8_t direction);
int emac_bdring_alloc(emac_bdring *ring_ptr, unsigned num_bd,
		      emac_bd **bdset_ptr);
int emac_bdring_unalloc(emac_bdring *ring_ptr, unsigned num_bd,
			emac_bd *bdset_ptr);
int emac_bdring_to_hw(emac_bdring *ring_ptr, unsigned num_bd,
		      emac_bd *bdset_ptr);
int emac_bdring_free(emac_bdring *ring_ptr, unsigned num_bd,
		     emac_bd *bdset_ptr);
unsigned emac_bdring_from_hw_tx(emac_bdring *ring_ptr,
				unsigned bd_limit,
				emac_bd **bdset_ptr);
unsigned emac_bdring_from_hw_rx(emac_bdring *ring_ptr,
				unsigned bd_limit,
				emac_bd **bdset_ptr);

#ifdef __cplusplus
}
#endif
#endif                          /* end of protection macros */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
