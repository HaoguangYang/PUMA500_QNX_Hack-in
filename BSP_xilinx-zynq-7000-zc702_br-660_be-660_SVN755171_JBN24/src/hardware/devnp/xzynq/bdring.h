/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
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

#ifndef NIC_BDRING_H_INCLUDED
#define NIC_BDRING_H_INCLUDED

/** This is an internal structure used to maintain the DMA list */
typedef struct {
	uint32_t phys_base_addr;        /**< Physical address of 1st BD in list */
	uint32_t base_bd_addr;          /**< Virtual address of 1st BD in list */
	uint32_t high_bd_addr;          /**< Virtual address of last BD in the list */
	uint32_t length;                /**< Total size of ring in bytes */
	uint32_t separation;            /**< Number of bytes between the starting address
	                                                                   of adjacent BDs */
	xzynq_bd_t *free_head;
	/**< First BD in the free group */
	xzynq_bd_t *pre_head;
	/**< First BD in the pre-work group */
	xzynq_bd_t *hw_head;
	/**< First BD in the work group */
	xzynq_bd_t *hw_tail;
	/**< Last BD in the work group */
	xzynq_bd_t *post_head;
	/**< First BD in the post-work group */
	xzynq_bd_t *bda_restart;
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
} xzynq_bdring_t;

#define xzynq_bdring_next(ring_ptr, bd_ptr)						\
	(((uint32_t)(bd_ptr) >= (ring_ptr)->high_bd_addr) ?			\
	 (xzynq_bd_t *)(ring_ptr)->base_bd_addr :					\
	 (xzynq_bd_t *)((uint32_t)(bd_ptr) + (ring_ptr)->separation))

#define xzynq_bdring_prev(ring_ptr, bd_ptr)						\
	(((uint32_t)(bd_ptr) <= (ring_ptr)->base_bd_addr) ?			\
	 (xzynq_bd_t *)(ring_ptr)->high_bd_addr :					\
	 (xzynq_bd_t *)((uint32_t)(bd_ptr) - (ring_ptr)->separation))

/*
 * Scatter gather DMA related functions in xemacps_bdring.c
 */
int xzynq_bdring_create(xzynq_bdring_t *ring_ptr, uint32_t phys_addr,
		       uint32_t virt_addr, uint32_t alignment,
		       unsigned bd_count);
int xzynq_bdring_clone(xzynq_bdring_t *ring_ptr, xzynq_bd_t *src_bd_ptr,
		      uint8_t direction);
int xzynq_bdring_alloc(xzynq_bdring_t *ring_ptr, unsigned num_bd,
		      xzynq_bd_t **bdset_ptr);
int xzynq_bdring_unalloc(xzynq_bdring_t *ring_ptr, unsigned num_bd,
			xzynq_bd_t *bdset_ptr);
int xzynq_bdring_to_hw(xzynq_bdring_t *ring_ptr, unsigned num_bd,
		      xzynq_bd_t *bdset_ptr, int is_rx);
int xzynq_bdring_free(xzynq_bdring_t *ring_ptr, unsigned num_bd,
		     xzynq_bd_t *bdset_ptr);
unsigned xzynq_bdring_from_hw_tx(xzynq_bdring_t *ring_ptr,
				unsigned bd_limit,
				xzynq_bd_t **bdset_ptr);
unsigned xzynq_bdring_from_hw_rx(xzynq_bdring_t *ring_ptr,
				unsigned bd_limit,
				xzynq_bd_t **bdset_ptr);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/bdring.h $ $Rev: 752035 $")
#endif
