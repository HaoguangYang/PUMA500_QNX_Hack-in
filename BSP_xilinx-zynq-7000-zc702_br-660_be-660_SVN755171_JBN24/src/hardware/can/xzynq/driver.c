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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/syspage.h>
#include <hw/gpio.h>

#include "canxzynq.h"
#include "proto.h"
#include "externs.h"

// Function prototypes
void device_init(int argc, char *argv[]);
void create_device(CANDEV_XZYNQ_INIT *devinit);

#ifdef CONFIG_PMM
#include <hw/pmm.h>
#include <hw/clk.h>

uint8_t can_get_mode(CANDEV_XZYNQ_INFO *devinfo);
void can_set_mode(CANDEV_XZYNQ_INFO *devinfo, uint8_t mode);

pmm_functions_t funcs;

static uint8_t pm_can_mode;

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_pm_standby(void* arg)
{
	CANDEV_XZYNQ_INFO *devinfo = (CANDEV_XZYNQ_INFO *) arg;

	pm_can_mode = can_get_mode(devinfo);
	can_set_mode(devinfo, XZYNQ_CAN_MODE_SLEEP);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_pm_resume(void* arg)
{
	CANDEV_XZYNQ_INFO *devinfo = (CANDEV_XZYNQ_INFO *) arg;

	can_set_mode(devinfo, pm_can_mode);
}

#endif

int main(int argc, char *argv[])
{
	// Driver implemented functions called by CAN library
	can_drvr_funcs_t drvr_funcs = { can_drvr_transmit,
			can_drvr_devctl };

	// Get I/O privity
	ThreadCtl(_NTO_TCTL_IO, 0);

	// Initialize Resource Manager
	can_resmgr_init(&drvr_funcs);

	// Process options and create devices
	device_init(argc, argv);

	// Start Handling Clients
	can_resmgr_start();

	return EXIT_SUCCESS;
}

void device_init(int argc, char *argv[])
{
	int opt;
	int numcan = 0;
	char *cp;
	// Set default options
	CANDEV_XZYNQ_INIT devinit =
	{
		{
			CANDEV_TYPE_RX, /* devtype */
			0, /* can_unit - set this later */
			0, /* dev_unit - set this later*/
			64, /* msgnum */
			XZYNQ_CAN_DLC_BYTE8, /* datalen */
		},
		XZYNQ_CAN0_BASEADDR, /* CAN port registers base address */
		XZYNQ_CAN_CLK_24MHZ, /* clk */
		0, /* bitrate */
		XZYNQ_CAN_BRPR_125K_DEFAULT, /* br_brp */
		XZYNQ_CAN_BTR_SJW_DEFAULT, /* br_sjw */
		XZYNQ_CAN_BTR_TS1_DEFAULT, /* br_tseg1 */
		XZYNQ_CAN_BTR_TS2_DEFAULT, /* br_tseg2 */
		XZYNQ_IRQ_CAN0, /* irqsys */
		(INIT_FLAGS_MDRIVER_INIT), /* flags */
		XZYNQ_CAN_NUM_MAX_RX, /* numrx */
		0x403, /* midrx */
		0x403, /* midtx */
		0x0, /* timestamp */
		-1, /* PHY reset pin, default = none */
	};

	// Process command line options and create associated devices
	while (optind < argc) {
		// Process dash options
		while ((opt = getopt(argc, argv, "ab:B:c:Di:l:m:Mn:op:r:StTu:x")) != -1) {
			switch (opt) {
			case 'a':
				devinit.flags |= INIT_FLAGS_AUTOBUS;
				break;
			case 'b':
				// Determine BRP value for desired bitrate
				if (strncmp(optarg, "10K", 3) == 0)
					devinit.br_brp = XZYNQ_CAN_BRPR_10K_DEFAULT;
				else if (strncmp(optarg, "20K", 3) == 0)
					devinit.br_brp = XZYNQ_CAN_BRPR_20K_DEFAULT;
				else if (strncmp(optarg, "25K", 3) == 0)
					devinit.br_brp = XZYNQ_CAN_BRPR_25K_DEFAULT;
				else if (strncmp(optarg, "50K", 3) == 0)
					devinit.br_brp = XZYNQ_CAN_BRPR_50K_DEFAULT;
				else if (strncmp(optarg, "100K", 4) == 0)
					devinit.br_brp = XZYNQ_CAN_BRPR_100K_DEFAULT;
				else if (strncmp(optarg, "125K", 4) == 0)
					devinit.br_brp = XZYNQ_CAN_BRPR_125K_DEFAULT;
				else if (strncmp(optarg, "500K", 4) == 0)
					devinit.br_brp = XZYNQ_CAN_BRPR_500K_DEFAULT;
				else if (strncmp(optarg, "1M", 2) == 0)
					devinit.br_brp = XZYNQ_CAN_BRPR_1M_DEFAULT;
				else
					// Set default to 10K
					devinit.br_brp = XZYNQ_CAN_BRPR_125K_DEFAULT;
				break;
			case 'B':
				// Values to program bitrate manually
				devinit.br_brp = strtoul(optarg, &optarg, 0) - 1;

				if ((cp = strchr(optarg, ','))) {
					cp += 1; // Skip over the ','
					devinit.br_tseg1 = strtoul(cp, &cp, 0) - 1;
				}
				if ((cp = strchr(cp, ','))) {
					cp += 1; // Skip over the ','
					devinit.br_tseg2 = strtoul(cp, &cp, 0) - 1;
				}
				if ((cp = strchr(cp, ','))) {
					cp += 1; // Skip over the ','
					devinit.br_sjw = strtoul(cp, &cp, 0) - 1;
				}

				// Check for valid bitrate settings
				if (devinit.br_brp > XZYNQ_CAN_BRPR_MAXVAL ||
					devinit.br_sjw > XZYNQ_CAN_BTR_SJW_MAXVAL ||
					devinit.br_tseg1 > XZYNQ_CAN_BTR_TS1_MAXVAL ||
					devinit.br_tseg2 > XZYNQ_CAN_BTR_TS2_MAXVAL ||
					devinit.br_tseg1 == 0 || devinit.br_tseg2 == 0) {
					fprintf(stderr, "Invalid manual bitrate settings\n");
					exit(EXIT_FAILURE);
				}

				break;
			case 'c':
				if (strncmp(optarg, "12M", 3) == 0)
					devinit.clk = XZYNQ_CAN_CLK_12MHZ;
				else if (strncmp(optarg, "24M", 3) == 0)
					devinit.clk = XZYNQ_CAN_CLK_24MHZ;
				else if (strncmp(optarg, "48M", 3) == 0)
					devinit.clk = XZYNQ_CAN_CLK_48MHZ;
				else
					devinit.clk = strtoul(optarg, NULL, 0);
				break;
			case 'D':
				devinit.flags &= ~INIT_FLAGS_MDRIVER_INIT;
				break;
			case 'i':
				devinit.midrx = strtoul(optarg, &optarg, 16);
				if ((cp = strchr(optarg, ','))) {
					devinit.midtx = strtoul(cp + 1, NULL, 0);
				}
				break;
			case 'l':
				devinit.cinit.datalen = strtoul(optarg, NULL, 0);
				if (devinit.cinit.datalen > XZYNQ_CAN_DLC_BYTE8) {
					fprintf(stderr,
							"Invalid CAN message data length, setting to %d\n",
							XZYNQ_CAN_DLC_BYTE8);
					devinit.cinit.datalen = XZYNQ_CAN_DLC_BYTE8;
				}
				break;
			case 'm':
				devinit.flags |= INIT_FLAGS_TIMESTAMP;
				devinit.timestamp = strtoul(optarg, NULL, 16);
				break;
			case 'M':
				devinit.flags |= INIT_FLAGS_RX_FULL_MSG;
				break;
			case 'n':
				devinit.cinit.msgnum = strtoul(optarg, NULL, 0);
				break;
			case 'o':
				devinit.flags |= INIT_FLAGS_MSGDATA_LSB;
				break;
			case 'p':
				devinit.rst_pin	= strtoul(optarg, &cp, 10);
				/* Check MIO pin consistency */
				if ((devinit.rst_pin > 53) || (optarg == cp)) {
					fprintf(stderr, "Invalid reset pin number %d\n", devinit.rst_pin);
					devinit.rst_pin = -1;
				}
				break;
			case 'r':
				devinit.numrx = strtoul(optarg, NULL, 0);
				if (devinit.numrx > XZYNQ_CAN_NUM_MAX_RX) {
					fprintf(
							stderr,
							"Invalid number or RX CAN mailboxes, setting to %d\n",
							XZYNQ_CAN_NUM_MAX_RX);
					devinit.numrx = XZYNQ_CAN_NUM_MAX_RX;
				}
				break;
			case 'S':
				devinit.flags |= INIT_FLAGS_MDRIVER_SORT;
				break;
			case 't':
				devinit.flags |= INIT_FLAGS_LOOPBACK;
				break;
			case 'T':
				devinit.flags |= INIT_FLAGS_SNOOP;
				break;
			case 'u':
				devinit.cinit.can_unit = strtoul(optarg, NULL, 0);
				break;
			case 'x':
				devinit.flags |= INIT_FLAGS_EXTENDED_MID;
				break;
			default:
				break;
			}
		}

		// Process ports and interrupt
		while (optind < argc && *(optarg = argv[optind]) != '-') {
			/* Set defaults for Zynq7000 Board */
			if (strncmp(optarg, "xzynqcan1", 9) == 0) {
				devinit.port = XZYNQ_CAN0_BASEADDR;
				devinit.irqsys = XZYNQ_IRQ_CAN0;
				// Set default can unit number
				if (!devinit.cinit.can_unit)
					devinit.cinit.can_unit = 1;
			} else if (strncmp(optarg, "xzynqcan2", 9) == 0) {
				devinit.port = XZYNQ_CAN1_BASEADDR;
				devinit.irqsys = XZYNQ_IRQ_CAN1;
				// Set default can unit number
				if (!devinit.cinit.can_unit)
					devinit.cinit.can_unit = 2;
			}
			/* Set command line option for HECC */
			else {
				// Set default port for CAN 0
				if (strncmp(optarg, "can1", 4) == 0) {
					// Set defaults even though user may override them
					devinit.port = XZYNQ_CAN0_BASEADDR;
					devinit.irqsys = XZYNQ_IRQ_CAN0;
					// Set default can unit number
					if (!devinit.cinit.can_unit)
						devinit.cinit.can_unit = 1;
					// Increment optarg
					optarg += 4;
				} else
				// Set default port for CAN 1
				if (strncmp(optarg, "can2", 4) == 0) {
					// Set defaults even though user may override them
					devinit.port = XZYNQ_CAN1_BASEADDR;
					devinit.irqsys = XZYNQ_IRQ_CAN1;
					// Set default can unit number
					if (!devinit.cinit.can_unit)
						devinit.cinit.can_unit = 2;
					// Increment optarg
					optarg += 4;
				} else {
					fprintf(stderr, "Invalid options\n");
					exit(EXIT_FAILURE);
				}
				// Set DCAN port registers address
				if (*optarg == ',')
					devinit.port = strtoul(optarg + 1, &optarg, 0);
				// Set system interrupt vector
				if (*optarg == ',')
					devinit.irqsys = strtoul(optarg + 1, NULL, 0);
			}
			++optind;

			// Create the CAN device
			create_device(&devinit);
			// Reset unit number for next device
			devinit.cinit.can_unit = 0;
			numcan++;
		}
	}
	// If no devices have been created yet, create the default device
	if (numcan == 0) {
		// Create the default CAN device
		devinit.cinit.can_unit = 1;
		create_device(&devinit);
	}
}

void create_device(CANDEV_XZYNQ_INIT *devinit)
{
	CANDEV_XZYNQ_INFO *devinfo;
	CANDEV_XZYNQ *devlist;
	int mdriver_intr = -1;
	int i;

#ifdef DEBUG_DRVR
	fprintf(stderr, "port = 0x%X\n", devinit->port);
	fprintf(stderr, "clk = %d\n", devinit->clk);
	fprintf(stderr, "bitrate = %d\n", devinit->bitrate);
	fprintf(stderr, "brp = %d\n", devinit->br_brp);
	fprintf(stderr, "sjw = %d\n", devinit->br_sjw);
	fprintf(stderr, "tseg1 = %d\n", devinit->br_tseg1);
	fprintf(stderr, "tseg2 = %d\n", devinit->br_tseg2);
	fprintf(stderr, "irqsys = %d\n", devinit->irqsys);
	fprintf(stderr, "msgnum = %u\n", devinit->cinit.msgnum);
	fprintf(stderr, "datalen = %u\n", devinit->cinit.datalen);
	fprintf(stderr, "unit = %u\n", devinit->cinit.can_unit);
	fprintf(stderr, "flags = %u\n", devinit->flags);
	fprintf(stderr, "numrx = %u\n", devinit->numrx);
	fprintf(stderr, "midrx = 0x%X\n", devinit->midrx);
	fprintf(stderr, "midtx = 0x%X\n", devinit->midtx);
	fprintf(stderr, "rst_pin = %d\n", devinit->rst_pin);
#endif

	// Allocate device info
	devinfo = (void *) _smalloc(sizeof(*devinfo));
	if (!devinfo) {
		fprintf(stderr, "_smalloc failed\n");
		exit(EXIT_FAILURE);
	}
	memset(devinfo, 0, sizeof(*devinfo));

	// Allocate an array of devices - one for each mailbox
	devlist = (void *) _smalloc(sizeof(*devlist) * XZYNQ_CAN_NUM_MAILBOX);
	if (!devlist) {
		fprintf(stderr, "_smalloc failed\n");
		exit(EXIT_FAILURE);
	}
	memset(devlist, 0, sizeof(*devlist) * XZYNQ_CAN_NUM_MAILBOX);

	// Map device registers
	devinfo->base = mmap_device_io(XZYNQ_CAN_REG_SIZE, devinit->port);
	if (devinfo->base == MAP_DEVICE_FAILED) {
		perror("Can't map device I/O");
		exit(EXIT_FAILURE);
	}

	// Determine if there is an active mini-driver and initialize driver to support it
	if (devinit->flags & INIT_FLAGS_MDRIVER_INIT) {
		mdriver_intr = mdriver_init(devinfo, devinit);
	}

#ifdef DEBUG_DRVR
	printf("INIT_FLAGS_MDRIVER_INIT %x mdriver_intr %x\n", (devinit->flags & INIT_FLAGS_MDRIVER_INIT), mdriver_intr);
#endif

	// Map device message memory
	devinfo->canmsg = (void *) _smalloc(XZYNQ_CAN_NUM_MAILBOX
			* sizeof(CAN_MSG_OBJ));
	if (devinfo->canmsg == NULL) {
		perror("Can't map device memory");
		exit(EXIT_FAILURE);
	}
	memset(devinfo->canmsg, 0, XZYNQ_CAN_NUM_MAILBOX * sizeof(CAN_MSG_OBJ));

	// Setup device info
	devinfo->devlist = devlist;
	// Setup the RX and TX mailbox sizes
	devinfo->numrx = devinit->numrx;
	devinfo->numtx = 1;

	// Initialize flags
	if (devinit->flags & INIT_FLAGS_RX_FULL_MSG)
		devinfo->iflags |= INFO_FLAGS_RX_FULL_MSG;
	if (!(devinit->flags & INIT_FLAGS_MSGDATA_LSB))
		devinfo->iflags |= INFO_FLAGS_ENDIAN_SWAP;

	// Initialize all device mailboxes
	for (i = 0; i < (devinfo->numrx + devinfo->numtx); i++) {
		// Set index into device mailbox memory
		devlist[i].mbxid = i;
		// Store a pointer to the device info
		devlist[i].devinfo = devinfo;

		// Set device mailbox unit number
		devinit->cinit.dev_unit = i;
		// Set device mailbox as transmit or receive
		if (i < devinfo->numrx)
			devinit->cinit.devtype = CANDEV_TYPE_RX;
		else
			devinit->cinit.devtype = CANDEV_TYPE_TX;

		// Initialize the CAN device
		can_resmgr_init_device(&devlist[i].cdev, &devinit->cinit);

		// Create the resmgr device
		can_resmgr_create_device(&devlist[i].cdev);
	}

	// Initialize clocks
	can_init_clocks(devinit);

	// Initialize device hardware if there is no mini-driver
	if (!(devinit->flags & INIT_FLAGS_MDRIVER_INIT) || mdriver_intr == -1)
		can_init_hw(devinfo, devinit);

#ifdef DEBUG_DRVR
	can_print_reg(devinfo);
#endif

	// Initialize device mailboxes if there is no mini-driver
	if (!(devinit->flags & INIT_FLAGS_MDRIVER_INIT) || mdriver_intr == -1)
		can_init_mailbox(devinfo, devinit);
#ifdef DEBUG_DRVR
	can_print_mbox(devinfo);
#endif

	// Initialize interrupts and attach interrupt handler
	can_init_intr(devinfo, devinit);

	// Add mini-driver's bufferred CAN messages if mini-driver is active.
	// Note1: This must be done BEFORE we start handling client requests and AFTER calling
	// InterruptAttach() to ensure we don't miss any data - mini-driver has not ended until
	// InterruptAttach is called.
	// Note2: We may receive new CAN messages (via the interrupt handler) while we add the
	// mini-driver's buffered messages to the driver's message queue.  The interrupt handler
	// will had new messages to the "head" of the message queue while the mini-driver messages
	// are added to the "tail" of the message queue - this allows us to add both old and new
	// messages to the queue in parrallel and in the correct order.
	// Note3: It is important that enough CAN messages are pre-allocated on the message
	// queue to ensure that there is room to add both mini-driver buffered messages and
	// new message.  The number required will be be dependent on the CAN data rate and the
	// time it takes for the system to boot and the full driver to take over.
	if (devinit->flags & INIT_FLAGS_MDRIVER_INIT && mdriver_intr != -1) {
		mdriver_init_data(devinfo, devinit);
	}

	// if a PHY needs to be toggled, do it here
	if (devinit->rst_pin != -1) {
		gpio_functions_t gpiofuncs;
		int mio = devinit->rst_pin;

		get_gpiofuncs(&gpiofuncs, sizeof(gpio_functions_t));
		if (gpiofuncs.init() != 0)
			fprintf(stderr, "Failed to initialize GPIO library\n");

		/* Setting pin parameters */
		gpiofuncs.mio_set_mux_l3_sel(mio, 0); // Select GPIO mode
		gpiofuncs.mio_set_mux_l2_sel(mio, 0);
		gpiofuncs.mio_set_mux_l1_sel(mio, 0);
		gpiofuncs.mio_set_mux_l0_sel(mio, 0);
		gpiofuncs.mio_set_mux_pullup(mio, 1); // Enabled pullup
		gpiofuncs.mio_set_mux_io_type(mio, 1); // LVCMOS1.8
		gpiofuncs.mio_set_mux_tri_enable(mio, 0); // 3-state
		gpiofuncs.mio_set_mux_speed(mio, 0); // Slow CMOS
		gpiofuncs.mio_set_mux_dis_rcvr(mio, 0); // Disable receiver

		/* Setting pin low */
		gpiofuncs.gpio_set_direction(mio, 1);
		gpiofuncs.gpio_set_output_enable(mio, 1);
		gpiofuncs.gpio_set_output(mio, 0);

		gpiofuncs.fini();
	}

#ifdef CONFIG_PMM
	funcs.arg = devinfo;
	funcs.standby = xzynq_pm_standby;
	funcs.resume = xzynq_pm_resume;

	if (devinit->port == XZYNQ_CAN0_BASEADDR) {
		pmm_init(&funcs, "can0");
	} else if (devinit->port == XZYNQ_CAN1_BASEADDR) {
		pmm_init(&funcs, "can1");
	}
#endif

#ifdef DEBUG_DRVR
	can_debug(devinfo);
#endif
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/can/xzynq/driver.c $ $Rev: 752035 $")
#endif
