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
#include "emac.h"

#define XZYNQ_EMACPS_CACHE_FLUSH(bd_ptr)
#define XZYNQ_EMACPS_CACHE_INVALIDATE(bd_ptr)

#define XZYNQ_EMACPS_RING_SEEKAHEAD(ring_ptr, bd_ptr, num_bd)			   \
	{									\
		uint32_t addr = (uint32_t) bd_ptr;				   \
									\
		addr += ((ring_ptr)->separation * num_bd);			  \
		if ((addr > (ring_ptr)->high_bd_addr) || ((uint32_t) bd_ptr > addr)) { \
			addr -= (ring_ptr)->length;					 \
		}								\
									\
		bd_ptr = (emac_bd *)addr;					  \
	}

#define XZYNQ_EMACPS_RING_SEEKBACK(ring_ptr, bd_ptr, num_bd)			   \
	{									\
		uint32_t addr = (uint32_t) bd_ptr;				   \
									\
		addr -= ((ring_ptr)->separation * num_bd);			  \
		if ((addr < (ring_ptr)->base_bd_addr) || ((uint32_t) bd_ptr < addr)) { \
			addr += (ring_ptr)->length;					 \
		}								\
									\
		bd_ptr = (emac_bd *)addr;					  \
	}

static void mem_clr(uint32_t *dest, size_t size)
{
	size /= sizeof(uint32_t);

	while (size-- > 0) *dest++ = 0;
}

int emac_bdring_create(emac_bdring *ring_ptr, uint32_t phys_addr,
			   uint32_t virt_addr, uint32_t alignment,
			   unsigned bd_count)
{
	unsigned i;
	uint32_t bd_virt_addr;
	uint32_t bd_phy_addr;

	/* In case there is a failure prior to creating list, make sure the
	 * following attributes are 0 to prevent calls to other functions
	 * from doing anything.
	 */
	ring_ptr->all_cnt = 0;
	ring_ptr->free_cnt = 0;
	ring_ptr->hw_cnt = 0;
	ring_ptr->pre_cnt = 0;
	ring_ptr->post_cnt = 0;

	/* Figure out how many bytes will be between the start of adjacent BDs */
	ring_ptr->separation =
		(sizeof(emac_bd) + (alignment - 1)) & ~(alignment - 1);

	/* Must make sure the ring doesn't span address 0x00000000. If it does,
	 * then the next/prev BD traversal macros will fail.
	 */
	if (virt_addr > (virt_addr + (ring_ptr->separation * bd_count) - 1))
		return DMA_SG_LIST_ERROR;

	/* Initial ring setup:
	 *  - Clear the entire space
	 *  - Setup each BD's BDA field with the physical address of the next BD
	 */
	mem_clr((uint32_t *)virt_addr, ring_ptr->separation * bd_count);

	bd_virt_addr = virt_addr;
	bd_phy_addr = phys_addr + ring_ptr->separation;
	for (i = 1; i < bd_count; i++) {
		XZYNQ_EMACPS_CACHE_FLUSH(bd_virt_addr);
		bd_virt_addr += ring_ptr->separation;
		bd_phy_addr += ring_ptr->separation;
	}

	/* Setup and initialize pointers and counters */
	ring_ptr->base_bd_addr = virt_addr;
	ring_ptr->phys_base_addr = phys_addr;
	ring_ptr->high_bd_addr = bd_virt_addr;
	ring_ptr->length =
		ring_ptr->high_bd_addr - ring_ptr->base_bd_addr +
		ring_ptr->separation;
	ring_ptr->all_cnt = bd_count;
	ring_ptr->free_cnt = bd_count;
	ring_ptr->free_head = (emac_bd *)virt_addr;
	ring_ptr->pre_head = (emac_bd *)virt_addr;
	ring_ptr->hw_head = (emac_bd *)virt_addr;
	ring_ptr->hw_tail = (emac_bd *)virt_addr;
	ring_ptr->post_head = (emac_bd *)virt_addr;
	ring_ptr->bda_restart = (emac_bd *)phys_addr;

	return SUCCESS;
}

int emac_bdring_clone(emac_bdring *ring_ptr, emac_bd *src_bd_ptr,
			  uint8_t direction)
{
	unsigned i;
	uint32_t cur_bd;

	/* Can't do this function if there isn't a ring */
	if (ring_ptr->all_cnt == 0)
		return DMA_SG_NO_LIST;

	/* Can't do this function with some of the BDs in use */
	if (ring_ptr->free_cnt != ring_ptr->all_cnt)
		return DMA_SG_LIST_ERROR;

	if ((direction != XZYNQ_EMACPS_SEND)
		&& (direction != XZYNQ_EMACPS_RECV))
		return INVALID_PARAM;

	/* Starting from the top of the ring, save BD.Next, overwrite the entire
	 * BD with the template, then restore BD.Next
	 */
	for (i = 0, cur_bd = (uint32_t)ring_ptr->base_bd_addr;
		 i < ring_ptr->all_cnt; i++, cur_bd += ring_ptr->separation) {
		mem_cpy((void *)cur_bd, src_bd_ptr, sizeof(emac_bd));
		XZYNQ_EMACPS_CACHE_FLUSH(cur_bd);
	}

	cur_bd -= ring_ptr->separation;

	if (direction == XZYNQ_EMACPS_RECV)
		emac_bd_set_rx_wrap(cur_bd);
	else
		emac_bd_set_tx_wrap(cur_bd);

	XZYNQ_EMACPS_CACHE_FLUSH(cur_bd);

	return SUCCESS;
}

int emac_bdring_alloc(emac_bdring *ring_ptr, unsigned num_bd,
			  emac_bd **bdset_ptr)
{
	/* Enough free BDs available for the request? */
	if (ring_ptr->free_cnt < num_bd)
		return FAILURE;

	/* Set the return argument and move free_head forward */
	*bdset_ptr = ring_ptr->free_head;
	XZYNQ_EMACPS_RING_SEEKAHEAD(ring_ptr, ring_ptr->free_head, num_bd);
	ring_ptr->free_cnt -= num_bd;
	ring_ptr->pre_cnt += num_bd;
	return SUCCESS;
}

int emac_bdring_unalloc(emac_bdring *ring_ptr, unsigned num_bd,
			emac_bd *bdset_ptr)
{
	/* Enough BDs in the free state for the request? */
	if (ring_ptr->pre_cnt < num_bd)
		return FAILURE;

	/* Set the return argument and move free_head backward */
	XZYNQ_EMACPS_RING_SEEKBACK(ring_ptr, ring_ptr->free_head, num_bd);
	ring_ptr->free_cnt += num_bd;
	ring_ptr->pre_cnt -= num_bd;
	return SUCCESS;
}

int emac_bdring_to_hw(emac_bdring *ring_ptr, unsigned num_bd,
			  emac_bd *bdset_ptr)
{
	emac_bd *cur_bd_ptr;
	unsigned i;

	/* if no bds to process, simply return. */
	if (0 == num_bd)
		return SUCCESS;

	/* Make sure we are in sync with emac_bdring_alloc() */
	if ((ring_ptr->pre_cnt < num_bd) || (ring_ptr->pre_head != bdset_ptr))
		return DMA_SG_LIST_ERROR;

	cur_bd_ptr = bdset_ptr;
	for (i = 0; i < num_bd; i++) {
		XZYNQ_EMACPS_CACHE_FLUSH(cur_bd_ptr);
		cur_bd_ptr = emac_bdring_next(ring_ptr, cur_bd_ptr);
	}

	/* Adjust ring pointers & counters */
	XZYNQ_EMACPS_RING_SEEKAHEAD(ring_ptr, ring_ptr->pre_head, num_bd);
	ring_ptr->pre_cnt -= num_bd;

	ring_ptr->hw_tail = cur_bd_ptr;
	ring_ptr->hw_cnt += num_bd;

	return SUCCESS;
}

unsigned emac_bdring_from_hw_tx(emac_bdring *ring_ptr, unsigned bd_limit,
				emac_bd **bdset_ptr)
{
	emac_bd *cur_bd_ptr;
	uint32_t bd_str = 0;
	unsigned bd_count;
	unsigned bd_partial_count;
	unsigned int sop = 0;

	cur_bd_ptr = ring_ptr->hw_head;
	bd_count = 0;
	bd_partial_count = 0;

	/* If no BDs in work group, then there's nothing to search */
	if (ring_ptr->hw_cnt == 0) {
		*bdset_ptr = NULL;
		return 0;
	}

	if (bd_limit > ring_ptr->hw_cnt)
		bd_limit = ring_ptr->hw_cnt;

	/* Starting at hw_head, keep moving forward in the list until:
	 *  - A BD is encountered with its new/used bit set which means
	 *    hardware has not completed processing of that BD.
	 *  - ring_ptr->hw_tail is reached and ring_ptr->hw_cnt is reached.
	 *  - The number of requested BDs has been processed
	 */
	while (bd_count < bd_limit) {
		/* Read the status */
		XZYNQ_EMACPS_CACHE_INVALIDATE(cur_bd_ptr);
		bd_str = emac_bd_read(cur_bd_ptr, XZYNQ_EMACPS_BD_STAT_OFFSET);

		if ((sop == 0) && (bd_str & XZYNQ_EMACPS_TXBUF_USED_MASK))
			sop = 1;

		if (sop == 1) {
			bd_count++;
			bd_partial_count++;
		}

		/* hardware has processed this BD so check the "last" bit.
		 * If it is clear, then there are more BDs for the current
		 * packet. Keep a count of these partial packet BDs.
		 */
		if ((sop == 1) && (bd_str & XZYNQ_EMACPS_TXBUF_LAST_MASK)) {
			sop = 0;
			bd_partial_count = 0;
		}

		/* Move on to next BD in work group */
		cur_bd_ptr = emac_bdring_next(ring_ptr, cur_bd_ptr);
	}

	/* Subtract off any partial packet BDs found */
	bd_count -= bd_partial_count;

	/* If bd_count is non-zero then BDs were found to return. Set return
	 * parameters, update pointers and counters, return success
	 */
	if (bd_count > 0) {
		*bdset_ptr = ring_ptr->hw_head;
		ring_ptr->hw_cnt -= bd_count;
		ring_ptr->post_cnt += bd_count;
		XZYNQ_EMACPS_RING_SEEKAHEAD(ring_ptr, ring_ptr->hw_head,
						bd_count);
		return bd_count;
	} else {
		*bdset_ptr = NULL;
		return 0;
	}
}

unsigned emac_bdring_from_hw_rx(emac_bdring *ring_ptr, unsigned bd_limit,
				emac_bd **bdset_ptr)
{
	emac_bd *cur_bd_ptr;
	uint32_t bd_str = 0;
	unsigned bd_count;
	unsigned bd_partial_count;

	cur_bd_ptr = ring_ptr->hw_head;
	bd_count = 0;
	bd_partial_count = 0;

	/* If no BDs in work group, then there's nothing to search */
	if (ring_ptr->hw_cnt == 0) {
		*bdset_ptr = NULL;
		return 0;
	}

	/* Starting at hw_head, keep moving forward in the list until:
	 *  - A BD is encountered with its new/used bit set which means
	 *    hardware has completed processing of that BD.
	 *  - ring_ptr->hw_tail is reached and ring_ptr->hw_cnt is reached.
	 *  - The number of requested BDs has been processed
	 */
	while (bd_count < bd_limit) {

		/* Read the status */
		XZYNQ_EMACPS_CACHE_INVALIDATE(cur_bd_ptr);
		bd_str = emac_bd_read(cur_bd_ptr, XZYNQ_EMACPS_BD_STAT_OFFSET);

		if (!(emac_bd_is_rx_new(cur_bd_ptr)))
			break;

		bd_count++;

		/* hardware has processed this BD so check the "last" bit. If
		 * it is clear, then there are more BDs for the current packet.
		 * Keep a count of these partial packet BDs.
		 */
		if (bd_str & XZYNQ_EMACPS_RXBUF_EOF_MASK)
			bd_partial_count = 0;
		else
			bd_partial_count++;

		/* Move on to next BD in work group */
		cur_bd_ptr = emac_bdring_next(ring_ptr, cur_bd_ptr);
	}

	/* Subtract off any partial packet BDs found */
	bd_count -= bd_partial_count;

	/* If bd_count is non-zero then BDs were found to return. Set return
	 * parameters, update pointers and counters, return success
	 */
	if (bd_count > 0) {
		*bdset_ptr = ring_ptr->hw_head;
		ring_ptr->hw_cnt -= bd_count;
		ring_ptr->post_cnt += bd_count;
		XZYNQ_EMACPS_RING_SEEKAHEAD(ring_ptr, ring_ptr->hw_head,
						bd_count);
		return bd_count;
	} else {
		*bdset_ptr = NULL;
		return 0;
	}
}

int emac_bdring_free(emac_bdring *ring_ptr, unsigned num_bd,
			 emac_bd *bdset_ptr)
{
	/* if no bds to process, simply return. */
	if (0 == num_bd)
		return SUCCESS;

	/* Make sure we are in sync with emac_bdringFromHw() */
	if ((ring_ptr->post_cnt < num_bd) || (ring_ptr->post_head != bdset_ptr))
		return DMA_SG_LIST_ERROR;

	/* Update pointers and counters */
	ring_ptr->free_cnt += num_bd;
	ring_ptr->post_cnt -= num_bd;
	XZYNQ_EMACPS_RING_SEEKAHEAD(ring_ptr, ring_ptr->post_head, num_bd);
	return SUCCESS;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
