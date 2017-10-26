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

#ifndef XZYNQ_EMACPS_BD_H       /* prevent circular inclusions */
#define XZYNQ_EMACPS_BD_H       /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "emac.h"

/* Minimum BD alignment */
#define XZYNQ_EMACPS_DMABD_MINIMUM_ALIGNMENT  4

/**
 * The emac_bd is the type for buffer descriptors (BDs).
 */
#define XZYNQ_EMACPS_BD_NUM_WORDS 2
typedef uint32_t emac_bd[XZYNQ_EMACPS_BD_NUM_WORDS];

#define emac_bd_clear(bd_ptr)    memset((bd_ptr), 0, sizeof(emac_bd))

#define emac_bd_read(base_address, offset)		   \
	(*(uint32_t *)((uint32_t)(base_address) + (uint32_t)(offset)))

#define emac_bd_write(base_address, offset, data)		   \
	(*(uint32_t *)((uint32_t)(base_address) + (uint32_t)(offset)) = (data))

#define emac_bd_set_address_tx(bd_ptr, addr)				\
	(emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET, (uint32_t)(addr)))

#define emac_bd_set_address_rx(bd_ptr, addr)				\
	emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET,		  \
			  ((emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET) &		\
			~XZYNQ_EMACPS_RXBUF_ADD_MASK) | (uint32_t)(addr)))

#define emac_bd_set_status(bd_ptr, data)			   \
	emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,		  \
			  emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) | data)

#define emac_bd_set_rx_wrap(bd_ptr)					\
	(emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET,		  \
			   emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET) |		 \
			   XZYNQ_EMACPS_RXBUF_WRAP_MASK))

#define emac_bd_set_tx_wrap(bd_ptr)					\
	(emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,		  \
			   emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) |		 \
			   XZYNQ_EMACPS_TXBUF_WRAP_MASK))

#define emac_bd_get_buf_addr(bd_ptr)				   \
	(emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET))

#define emac_bd_set_length(bd_ptr, len_bytes)				\
	emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,		  \
			  ((emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) &		\
			~XZYNQ_EMACPS_TXBUF_LEN_MASK) | (len_bytes)))

#define emac_bd_set_last(bd_ptr)				   \
	(emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,		  \
			   emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) |		 \
			   XZYNQ_EMACPS_TXBUF_LAST_MASK))

#define emac_bd_clear_rx_new(bd_ptr)					\
	(emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET,		  \
			   emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET) &		 \
			   ~XZYNQ_EMACPS_RXBUF_NEW_MASK))

#define emac_bd_is_rx_new(bd_ptr)					\
	((emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET) &		  \
	  XZYNQ_EMACPS_RXBUF_NEW_MASK) ? 1 : 0)

#define emac_bd_set_tx_used(bd_ptr)					\
	(emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,		  \
			   emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) |		 \
			   XZYNQ_EMACPS_TXBUF_USED_MASK))

#define emac_bd_clear_tx_used(bd_ptr)					\
	(emac_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,		  \
			   emac_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) &		 \
			   ~XZYNQ_EMACPS_TXBUF_USED_MASK))

#ifdef __cplusplus
}
#endif
#endif                          /* end of protection macro */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
