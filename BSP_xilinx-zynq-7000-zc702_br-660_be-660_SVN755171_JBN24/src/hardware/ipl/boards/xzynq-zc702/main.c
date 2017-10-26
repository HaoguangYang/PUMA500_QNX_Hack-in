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

#include <hw/inout.h>
#include "ipl.h"
#include "sdmmc.h"
#include "emac.h"
#include "arp.h"
#include "tftp.h"
#include "fat-fs.h"
#include "sdhc_xzynq.h"
#include "ipl_xzynq.h"
#include "ps7_init.h"
#include "qspi.h"
#include "timer.h"
#include "i2c.h"

extern fs_info_t fs_info;

extern void init_serial_xzynq();

/* Default address IP of the TFTP server */
uint8_t TFTP_IP_SERVER[4] = { 10, 1, 1, 1 };

void delay(unsigned dly)
{
	volatile int j;

	while (dly--) {
		for (j = 0; j < 32; j++) ;
	}
}

/*
 * WARNING: no check on the input and the format of the IP address ...
 * You have been warned
 */
static int get_address_ip(uint8_t *res)
{
	char tmp_ip[16] = { 0 };
	char *ptr;
	unsigned char value[4] = { 0 };
	size_t index = 0;
	int i;

	/* Get the IP address */
	for (i = 0; i < 15; i++) {
		tmp_ip[i] = ser_getchar();
		if (tmp_ip[i] == '\r')
			break;
		ser_putchar(tmp_ip[i]);
	}

	if (tmp_ip[0] == '\r')
		return 1;

	tmp_ip[i] = '\0';

	/* Check the validity */
	ptr = tmp_ip;
	while (*ptr) {
		/* Is it a digit ? */
		if (*ptr >= '0' && *ptr <= '9') {
			value[index] *= 10;
			value[index] += *ptr - '0';
		} else if (*ptr == '.') {
			index++;
		} else {
			ser_putstr("\nWrong IP address !\n");
			return -1;
		}
		ptr++;
	}

	if (index != 0)
		for (i = 0; i < 4; i++)
			res[i] = value[i];

	return 0;
}

static int char2hex(char value)
{
	switch (value) {
	case '0' ... '9':
		return value - 48;
	case 'a' ... 'f':
		return value - 87;
	case 'A' ... 'F':
		return value - 55;
	default:
		return -1;
	}
}

static int check_mac_address(uint8_t *buffer)
{
	int sum = 0;
	int i;

	/* Check the validity */
	for (i = 0; i < 6; i++) {
		sum += buffer[i];
	}

	/* 00:00:00:00:00:00 or FF:FF:FF:FF:FF:FF */
	if (sum == 0 || sum == 6*0xFF)
		return -1;

	return 0;
}

static int check_ip_address(uint8_t *buffer)
{
	int sum = 0;
	int i;

	/* Check the validity */
	for (i = 0; i < 4; i++) {
		sum += buffer[i];
	}

	/* 00:00:00:00:00:00 or FF:FF:FF:FF:FF:FF */
	if (sum == 0 || sum == 4*0xFF)
		return -1;

	return 0;
}

/*
 * WARNING: no check on the input and the format of the MAC address ...
 * You have been warned
 */
static int get_mac_address(uint8_t *res)
{
	int i;
	uint8_t mac_temp[6];
	char c;

	/* Get the IP address */
	for (i = 0; i < 6; i++) {
		/* Set the last 4bits */
		c = char2hex(ser_getchar());

		if (c == -1) {
			/* default value selected */
			if (i == 0)
				/* return special value */
				return 1;
			return -1;
		}

		mac_temp[i] = 0;
		mac_temp[i] |= (c << 4);

		/* Set the first 4bits */
		c = char2hex(ser_getchar());

		if (c == -1) {
			return -1;
		}

		mac_temp[i] |= c;
		ser_puthex8(EMAC_MAC_ADDR[i]);

		/* Last */
		if (i == 5)
			break;

		/* Check the separator */
		if(ser_getchar() != ':')
			return -1;
		ser_putchar(':');
	}

	if (check_mac_address(mac_temp) != 0) {
		return -1;
	}

	for (i=0; i<6; i++) {
		res[i] = mac_temp[i];
	}

	return 0;
}

static inline void eep_read_at_address(uint8_t addr, uint8_t *buffer, uint8_t size)
{
	uint8_t addr_buffer = addr;

	i2c_send(&addr_buffer, 1, I2C_EEPROM_ADDR);
	while (i2c_bus_is_busy());
	msleep(300);

	/* Read the MAC Address */
	i2c_recv(buffer, size, I2C_EEPROM_ADDR);
	while (i2c_bus_is_busy());
}

static inline void ethernet_read_mac_address(uint8_t* mac_buffer)
{
	eep_read_at_address(0x0, mac_buffer, 6);
}

static inline void ethernet_read_ip_target(uint8_t* ip_target_buffer)
{
	eep_read_at_address(0x8, ip_target_buffer, 4);
}

static inline void ethernet_read_ip_server(uint8_t* ip_server_buffer)
{
	eep_read_at_address(0xC, ip_server_buffer, 4);
}

static inline void eep_write_at_address(uint8_t addr, uint8_t *buffer, uint8_t size)
{
	uint8_t eeprom_buffer[7]; /* EEPROM Addr + max size (6) */

	eeprom_buffer[0] = addr;
	mem_cpy(&eeprom_buffer[1], buffer, size);

	/* Store the MAC Address */
	i2c_send(eeprom_buffer, size+1, I2C_EEPROM_ADDR);
	while (i2c_bus_is_busy());
	msleep(1000);
}

static inline void ethernet_write_mac_address()
{
	eep_write_at_address(0x0, &EMAC_MAC_ADDR[0], 6);
}

static inline void ethernet_write_ip_target()
{
	eep_write_at_address(0x8, &EMAC_IP_ADDR[0], 4);
}

static inline void ethernet_write_ip_server()
{
	eep_write_at_address(0xC, &TFTP_IP_SERVER[0], 4);
}

static void ser_putdec8(unsigned x)
{
	int					i;
	unsigned			size = 2;
	char				buf[9];
	static const char	dec[] = "0123456789";

	if (x > 99)
		size++;

	for (i = 0; i < size; i++) {
		buf[(size - 1) - i] = dec[x % 10];
		x /= 10;
	}
	buf[i] = '\0';
	ser_putstr(buf);
}

static inline int ethernet_load_file(unsigned address, const char *fn)
{
	int i;
	int res = 0;
	uint8_t mux_buffer;
	uint8_t mac_buffer[6];
	uint8_t ip_server_buffer[4];
	uint8_t ip_target_buffer[4];

	/* Initialize the i2c clock */
	i2c_set_clk(I2C_CLK);

	/* Set the i2c mux to select the EEPROM channel */
	mux_buffer = I2C_EEPROM_CHANNEL;
	i2c_send(&mux_buffer, 1, I2C_MUX_ADDR);
	while (i2c_bus_is_busy());

	/* Read the current MAC address */
	ethernet_read_mac_address(mac_buffer);
	/* If value read is not correct: Update EEPROM with default value */
	if (check_mac_address(mac_buffer) != 0) {
		mem_cpy(mac_buffer, EMAC_MAC_ADDR, 6);
		ethernet_write_mac_address();
	}
	ser_putstr("MAC Address (");
	for (i = 0; i < 5; i++) {
		ser_puthex8(mac_buffer[i]);
		ser_putstr(":");
	}
	ser_puthex8(mac_buffer[i]);
	ser_putstr(" by default): ");
	res = get_mac_address(mac_buffer);
	if (res == -1) {
		ser_putstr("\nMAC Address invalid !\n");
		return -1;
	}
	ser_putstr("\n");

	/* Save value into global variable */
	mem_cpy(EMAC_MAC_ADDR, mac_buffer, 6);

	/* Write the MAC Address if changed */
	if (res == 0) {
		ethernet_write_mac_address();
	}

	/* Read the current target IP address */
	ethernet_read_ip_target(ip_target_buffer);
	/* If value read is not correct: Update EEPROM with default value */
	if (check_ip_address(ip_target_buffer) != 0) {
		mem_cpy(ip_target_buffer, EMAC_IP_ADDR, 4);
		ethernet_write_ip_target();
	}
	ser_putstr("Bootloader IP Address (");
	for (i = 0; i < 3; i++) {
		ser_putdec8(ip_target_buffer[i]);
		ser_putstr(".");
	}
	ser_putdec8(ip_target_buffer[i]);
	ser_putstr(" by default): ");
	res = get_address_ip(ip_target_buffer);
	if (res == -1) {
		ser_putstr("\nIP Address invalid !\n");
		return -1;
	}
	ser_putstr("\n");

	/* Save value into global variable */
	mem_cpy(EMAC_IP_ADDR, ip_target_buffer, 4);

	/* Write the address if changed */
	if (res == 0)
		ethernet_write_ip_target();

	/* Read the current server IP address */
	ethernet_read_ip_server(ip_server_buffer);
	/* If value read is not correct: Update EEPROM with default value */
	if (check_ip_address(ip_server_buffer) != 0) {
		mem_cpy(ip_server_buffer, TFTP_IP_SERVER, 4);
		ethernet_write_ip_target();
	}
	ser_putstr("Server IP Address (");
	for (i = 0; i < 3; i++) {
		ser_putdec8(ip_server_buffer[i]);
		ser_putstr(".");
	}
	ser_putdec8(ip_server_buffer[i]);
	ser_putstr(" by default): ");
	res = get_address_ip(ip_server_buffer);
	if (res == -1) {
		ser_putstr("\nIP Address invalid !\n");
		return -1;
	}
	ser_putstr("\n");

	/* Save value into global variable */
	mem_cpy(TFTP_IP_SERVER, ip_server_buffer, 4);

	/* Write the address if changed */
	if (res == 0)
		ethernet_write_ip_server();

	/* Initialize the EMAC controller and the PHY */
	if (emac_init() != 0)
		return -1;

	/* Allocate RX buffers */
	for (i = 0; i < RXBD_CNT; i++) {
		emac_bd *bd_rx_ptr;
		emac_bdring_alloc(&(emac_get_rx_ring(&eth0)), 1, &bd_rx_ptr);
		emac_bd_set_address_rx(bd_rx_ptr, (uint8_t *) (emac_rx_buffers +
							       (i *
								XZYNQ_EMACPS_MAX_VLAN_FRAME_SIZE)));
		emac_bdring_to_hw(&(emac_get_rx_ring(&eth0)), 1, bd_rx_ptr);
	}

	/* Start the controller to receive/transmit */
	emac_start(&eth0);

	/* Get the MAC Address of the TFTP server */
	emac_arp_done = 0;
	i = ARP_RETRY;
	do {
		emac_arp_request(TFTP_IP_SERVER);
	} while (emac_wait_for(&emac_arp_done, ARP_DELAY) != 0 && i-- > 0);

	if (!emac_arp_done)
		return -1;

	/* Transfert the file by TFTP */
	emac_tftp_done = 0;
	ser_putstr("TFTP: Starting the transfer\n");
	emac_tftp_request_image(fn);

	/* Wait no more than 10s */
	res = emac_wait_for(&emac_tftp_done, TFTP_DELAY);

	if (res != 0 || emac_tftp_done == 0) {
		return -1;
	}

	emac_fini();

	return 0;
}

static inline int sdmmc_load_file(unsigned address, const char *fn)
{
	xzynq_sdmmc_t sdmmc;
	int status;

	/*
	 * Initialize the SDMMC interface
	 */
	sdmmc.sdmmc_pbase = XZYNQ_SDIO0_BASEADDR;	// SDMMC base address

	/* initialize the sdmmc interface and card */
	if (SDMMC_OK != sdmmc_init_ctrl(&sdmmc)) {
		return SDMMC_ERROR;
	}

	if (sdmmc_init_card(&sdmmc)) {
		return SDMMC_ERROR;
	}

	ser_putstr("Load QNX image from SDMMC...\n");
	if (fat_read_mbr(&sdmmc, 0) != 0) {
		return SDMMC_ERROR;
	}

	if (FAT32 == fs_info.fat_type) {
		status =
		    fat_copy_named_file((unsigned char *)address, (char *)fn);
	} else {
		ser_putstr
		    ("SDMMC card has the unsupport file system, please use FAT32 file system\n");
		return SDMMC_ERROR;
	}

	sdmmc_fini(&sdmmc);

	return status;
}

static inline int qspi_load_file(unsigned address)
{
	int status = 0;

	/* Initialize the QSPI NOR flash */
	status = qspi_init();
	if (status < 0) {
		ser_putstr("QSPI initialization error...\n");
		return status;
	}

	ser_putstr("Load QNX image from QSPI NOR flash...\n");
	status = qspi_get_ifs(address);

	return status;
}

int main()
{
	unsigned image = QNX_LOAD_ADDR;
	
	//No need to do this since already done in FSBL.
	//ps7_init();

	/* Init serial interface */
	init_serial_xzynq();

	ser_putstr
	    ("\nWelcome to QNX Neutrino Initial Program Loader for Xilinx Zynq-7000 ZC702 (ARM Cortex-A9 MPCore)\n");

	while (1) {
		ser_putstr("Command:\n");
		ser_putstr
		    ("Press 'D' for serial download, using the 'sendnto' utility\n");
		ser_putstr
		    ("Press 'F' for QSPI flash download, IFS file MUST be at offset 0x100000.\n");
		ser_putstr
		    ("Press 'M' for SDMMC download, IFS filename MUST be 'QNX-IFS'.\n");
		ser_putstr
		    ("Press 'T' for TFTP download, IFS filename MUST be 'QNX-IFS'.\n");
		switch (ser_getchar()) {
		case 'D':
		case 'd':
			ser_putstr("send image now...\n");
			if (image_download_ser(image)) {
				ser_putstr("download failed...\n");
				continue;
			} else
				ser_putstr("download OK...\n");
			break;
		case 'F':
		case 'f':
			if (qspi_load_file(image) == 0)
				break;

			ser_putstr("load image failed\n");
			continue;
		case 'M':
		case 'm':
			if (sdmmc_load_file(image, "QNX-IFS") == 0)
				break;

			ser_putstr("load image failed\n");
			continue;
		case 'T':
		case 't':
			if (ethernet_load_file(image, "QNX-IFS") == 0)
				break;

			ser_putstr("load image failed\n");
			continue;
		default:
			continue;
		}

		/* Clear pending rsp1 for the OS to detect inserted SD card */
		out32(XZYNQ_SDIO0_BASEADDR + XZYNQ_SD_RSP_1, 0);

		image = image_scan(image, image + 0x200);

		if (image != 0xffffffff) {
			ser_putstr("Found image               @ 0x");
			ser_puthex(image);
			ser_putstr("\n");
			image_setup(image);

			ser_putstr("Jumping to startup        @ 0x");
			ser_puthex(startup_hdr.startup_vaddr);
			ser_putstr("\n\n");
			image_start(image);

			/* Never reach here */
			return 0;
		}

		ser_putstr("Image_scan failed...\n");
	}

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
