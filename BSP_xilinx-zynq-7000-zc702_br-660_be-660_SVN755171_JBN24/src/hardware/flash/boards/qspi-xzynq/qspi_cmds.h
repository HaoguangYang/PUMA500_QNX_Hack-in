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

#ifndef QSPI_CMDS_H_
#define QSPI_CMDS_H_

#include <stdint.h>
#include "variant.h"

/* Test defines */
#define KILO(__n) ((__n) << 10)

/* Flash commands */
#define	FLASH_OPCODE_WRSR		0x01 /* Write status register */
#define	FLASH_OPCODE_PP			0x02 /* Page program */
#define	FLASH_OPCODE_NORM_READ	0x03 /* Normal read data bytes */
#define	FLASH_OPCODE_WRDS		0x04 /* Write disable */
#define	FLASH_OPCODE_RDSR1		0x05 /* Read status register 1 */
#define	FLASH_OPCODE_WREN		0x06 /* Write enable */
#define	FLASH_OPCODE_FAST_READ	0x0B /* Fast read data bytes */
#define	FLASH_OPCODE_BE_4K		0x20 /* Erase 4KiB block */
#define	FLASH_OPCODE_RDSR2		0x35 /* Read status register 2 */
#define	FLASH_OPCODE_DUAL_READ	0x3B /* Dual read data bytes */
#define	FLASH_OPCODE_BE_32K		0x52 /* Erase 32KiB block */
#define	FLASH_OPCODE_QUAD_READ	0x6B /* Quad read data bytes */
#define	FLASH_OPCODE_ERASE_SUS	0x75 /* Erase suspend */
#define	FLASH_OPCODE_ERASE_RES	0x7A /* Erase resume */
#define	FLASH_OPCODE_RDID		0x9F /* Read JEDEC ID */
#define	FLASH_OPCODE_BE			0xC7 /* Erase whole flash block */
#define	FLASH_OPCODE_SE			0xD8 /* Sector erase (usually 64KB)*/

int iswriting(const int qspi_fd);
int wait_for_completion(const int qspi_fd);

int read_ident(
	const int qspi_fd,
	int* manufact_id, 
	int* device_id, 
	uint32_t* size);

int pd_release(const int qspi_fd);

int sector_erase(
	const int qspi_fd,
	const int offset);

int page_program(
	const int qspi_fd,
	int offset, 
	int len, 
	uint8_t const* data);

int read_from(
	const int qspi_fd,
	int offset, 
	int len, 
	uint8_t* buffer);

#endif /* QSPI_CMDS_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
