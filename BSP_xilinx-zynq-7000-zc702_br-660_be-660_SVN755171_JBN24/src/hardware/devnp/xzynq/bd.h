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

#ifndef NIC_BD_H_INCLUDED
#define NIC_BD_H_INCLUDED

/* Minimum BD alignment */
#define XZYNQ_EMACPS_BD_ALIGNMENT 4

/**
 * The xzynq_bd is the type for buffer descriptors (BDs).
 */
#define XZYNQ_EMACPS_BD_NUM_WORDS 2
typedef uint32_t xzynq_bd_t[XZYNQ_EMACPS_BD_NUM_WORDS];

#define xzynq_bd_get_length(bd_ptr)										\
    (xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) &				\
    XZYNQ_EMACPS_RXBUF_LEN_MASK)

#define xzynq_bd_clear(bd_ptr)    memset((bd_ptr), 0, sizeof(xzynq_bd_t))

#define xzynq_bd_read(regbase, offset)									\
	(*(uint32_t *)((uint32_t)(regbase) + (uint32_t)(offset)))

#define xzynq_bd_write(regbase, offset, data)							\
	(*(uint32_t *)((uint32_t)(regbase) + (uint32_t)(offset)) = (data))

#define xzynq_bd_set_address_tx(bd_ptr, addr)							\
	(xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET, (uint32_t)(addr)))

#define xzynq_bd_set_address_rx(bd_ptr, addr)							\
	xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET,				\
			  ((xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET) &	\
			~XZYNQ_EMACPS_RXBUF_ADD_MASK) | (uint32_t)(addr)))

#define xzynq_bd_set_status(bd_ptr, data)								\
	xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,				\
			  xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) | data)

#define xzynq_bd_get_status(bd_ptr)										\
    xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET)

#define xzynq_bd_set_rx_wrap(bd_ptr)									\
	(xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET,				\
			   xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET) |	\
			   XZYNQ_EMACPS_RXBUF_WRAP_MASK))

#define xzynq_bd_set_tx_wrap(bd_ptr)									\
	(xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,				\
			   xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) |	\
			   XZYNQ_EMACPS_TXBUF_WRAP_MASK))

#define xzynq_bd_get_buf_addr(bd_ptr)									\
	(xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET))

#define xzynq_bd_set_length(bd_ptr, len_bytes)							\
	xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,				\
			  ((xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) &	\
			~XZYNQ_EMACPS_TXBUF_LEN_MASK) | (len_bytes)))

#define xzynq_bd_set_last(bd_ptr)										\
	(xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,				\
			   xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) |	\
			   XZYNQ_EMACPS_TXBUF_LAST_MASK))

#define xzynq_bd_clear_rx_new(bd_ptr)									\
	(xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET,				\
			   xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET) &	\
			   ~XZYNQ_EMACPS_RXBUF_NEW_MASK))

#define xzynq_bd_is_rx_new(bd_ptr)										\
	((xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_ADDR_OFFSET) &			\
	  XZYNQ_EMACPS_RXBUF_NEW_MASK) ? 1 : 0)

#define xzynq_bd_set_tx_used(bd_ptr)									\
	(xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,				\
			   xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) |	\
			   XZYNQ_EMACPS_TXBUF_USED_MASK))

#define xzynq_bd_clear_tx_used(bd_ptr)									\
	(xzynq_bd_write((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET,				\
			   xzynq_bd_read((bd_ptr), XZYNQ_EMACPS_BD_STAT_OFFSET) &	\
			   ~XZYNQ_EMACPS_TXBUF_USED_MASK))

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/bd.h $ $Rev: 752035 $")
#endif
