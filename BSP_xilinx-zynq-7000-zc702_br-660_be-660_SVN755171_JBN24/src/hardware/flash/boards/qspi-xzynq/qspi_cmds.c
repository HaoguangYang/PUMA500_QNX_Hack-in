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

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "qspi_cmds.h"

#define DEVICE_SR_WIP	(1 << 0) /* Write in Progress (WIP) bit */
#define DEVICE_SR_WEL	(1 << 1) /* Write Enable Latch (WEL) bit */
#define DEVICE_SR_BP0	(1 << 2) /* Block Protect bit BP0 */
#define DEVICE_SR_BP1	(1 << 3) /* Block Protect bit BP1 */
#define DEVICE_SR_BP2	(1 << 4) /* Block Protect bit BP2 */
#define DEVICE_SR_E_ERR (1 << 5) /* Erase Error Occurred */
#define DEVICE_SR_P_ERR (1 << 5) /* Programming Error Occurred */
#define DEVICE_SR_RSVD  (3 << 5) /* bits 5 and 6 are reserved and always zero */
#define DEVICE_SR_SRWD	(1 << 7) /* Status Register Write Disable (SRWD) bit */

/* Set 24-bit address in big endian */
static void set_address(uint8_t* buf, const int addr)
{
	buf[0] = (addr & 0xff0000) >> 16;
	buf[1] = (addr & 0x00ff00) >>  8;
	buf[2] = (addr & 0x0000ff) >>  0;
}

/* Must do a write enable immediately before a sector erase or program command */
static int write_enable(const int qspi_fd)
{
	int rc = xzynq_qspi_write_1byte(qspi_fd, FLASH_OPCODE_WREN);
	if (-1 == rc) {
		return rc;
	}
	return EOK;
}

/*
 * return 1 if and only if WIP is set
 * return -1 for error
 * return 0 if not writing
 */
int iswriting(const int qspi_fd)
{
	uint8_t cmd[4] = { FLASH_OPCODE_RDSR1, 0, 0, 0 };
	int rc = 0;

	rc = xzynq_qspi_word_exchange(qspi_fd, (uint32_t*)cmd);
	if (rc) {
		errno = EIO;
		return -1;
	}
	return (cmd[3] & DEVICE_SR_WIP);
}

/* Wait until program is done */
int wait_for_completion(const int qspi_fd)
{
	int count = 2500;
	int rc = 0;
	do {
		rc = iswriting(qspi_fd);
		if (-1 == rc)
			return rc;
		if (rc & DEVICE_SR_WIP)
			usleep(400);
	} while (rc && --count);
	if (count == 0) {
		errno = EIO;
		return -1;
	}
	return EOK;
}


/*
 * read identification
 * prereq: no write in progress
 */
int read_ident(const int qspi_fd, int* manufact_id, int* device_id,
		uint32_t* size)
{
	uint8_t ident[4] = { FLASH_OPCODE_RDID, 0, 0, 0};
	int rc = 0;

	rc = xzynq_qspi_word_exchange(qspi_fd, (uint32_t*)ident);
	if (-1 == rc) {
		return rc;
	}
	*manufact_id = ident[1];
	*device_id = ident[2];
	*size = 1 << ident[3];

	return (EOK);
}


/*
 * release from power down
 * prereq: no write in progress
 */
int pd_release(const int qspi_fd)
{
	return (EOK);
}


/*
 * erase sector at given offset
 * prereq: no write in progress
 */
int sector_erase(const int qspi_fd, const int offset)
{
	uint8_t buf[4];
	int rc = 0;

	buf[0] = FLASH_OPCODE_SE;

	set_address(&buf[1], offset);

	rc = write_enable(qspi_fd);
	if (rc < 0)
		return rc;

	rc = xzynq_qspi_cmd_write(qspi_fd, buf, NULL, 0);
	if (rc < 0)
		return rc;

	return EOK;
}


/*
 * program up to MAX_BURST-4 bytes at any given offset
 * prereq: no write in progress
 * returns number of bytes written
 */
int page_program(const int qspi_fd, int offset, int len, uint8_t const* data)
{
	const int header_size = CMD_LEN + ADDR_LEN;
	int rc;

	int nbytes = min(len, MAX_BURST - header_size);
	// if writing all nbytes crosses a page boundary, then we reduce nbytes so that we write to the
	// end of the current page, but not beyond.
	nbytes = min(nbytes, (offset & ~(PAGE_SIZE-1)) + PAGE_SIZE - offset);

#ifdef _EXTRA_VERIFY
	uint8_t pre_prog[MAX_BURST - header_size];
	rc = read_from(qspi_fd, offset, nbytes, pre_prog);
	if (rc != nbytes) {
		fprintf(stderr, "t%d:%s:%d expected %d bytes, but read_from returned %d\n",
				pthread_self(), __func__, __LINE__, nbytes, rc);
		return -1;
	}
#endif

	rc = write_enable(qspi_fd);
	if (rc < 0) {
		return -1;
	}

	uint8_t header[header_size];

	// copy part of data to program into buf
	header[0] = FLASH_OPCODE_PP;
	set_address(&header[1], offset);

	rc = xzynq_qspi_cmd_write(qspi_fd, header, data, nbytes);

	// attempt wait for write to complete even if previous qspi_cmd_write failed
	if (wait_for_completion(qspi_fd)) {
		return -1;
	}

	// return with error if previous qspi_cmd_write failed
	if (-1 == rc) {
		return -1;
	}

#ifdef _EXTRA_VERIFY
	// do a verify here
	{
		uint8_t readback[MAX_BURST - header_size];
		rc = read_from(qspi_fd, offset, nbytes, readback);
		if (rc != nbytes) {
			fprintf(stderr, "t%d:%s:%d expected %d bytes, but read_from returned %d\n",
					pthread_self(), __func__, __LINE__, nbytes, rc);
		}
		int i = 0;
		int hitbad = 0;

		for (i=0 ; i< nbytes; ++i) {
			if (readback[i] != data[i]) {
				fprintf(stderr, "t%d: offset=%06x, pre_prog[%02x]=%02x, data[%02x]=%02x, readback[%02x]=%02x\n",
						pthread_self(),
						offset,
						i, pre_prog[i],
						i, data[i],
						i, readback[i]);
				hitbad = 1;
			}
		}
	}
#endif
	return nbytes;
}


/*
 * read from open SPI flash from offset, up to MAX_BURST - header_size bytes
 * prereq: no write in progress
 * return len bytes read
 * return -1 if error
 */
int read_from(const int qspi_fd, int offset, int len, uint8_t* buffer)
{
	const int header_size = CMD_LEN + ADDR_LEN;
	const int max_read = MAX_BURST - header_size;
	uint8_t header[header_size];
	int rc;

	if (len == 0) {
		return 0;
	}

	header[0] = FLASH_OPCODE_QUAD_READ;
	set_address(&header[CMD_LEN], offset);

	len = min(len, max_read);
	rc = xzynq_qspi_cmd_read(qspi_fd, header, buffer, len);
	if (-1 == rc) {
		return rc;
	}
	else {
		return len;
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
