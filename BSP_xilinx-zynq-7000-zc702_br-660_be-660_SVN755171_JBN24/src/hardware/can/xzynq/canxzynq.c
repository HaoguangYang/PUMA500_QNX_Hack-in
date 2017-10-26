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
#include <stdint.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <devctl.h>
#include <unistd.h>
#include <atomic.h>
#include <gulliver.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <hw/clk.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "canxzynq.h"
#include "externs.h"

#ifdef DEBUG_XZYNQ
#define LOG(fmt, arg...) fprintf(stderr, fmt, ## arg)
#else
#define LOG(fmt, arg...)
#endif

/* Set default CAN message id's in a range that is valid for both standard and extended id's */
#define CANMID_DEFAULT              0x1

static intrspin_t can_lock;

/* Function prototypes */
void can_tx(CANDEV_XZYNQ *cdev, canmsg_t *txmsg);
void can_print_mbox(CANDEV_XZYNQ_INFO *devinfo);
void can_print_reg(CANDEV_XZYNQ_INFO *devinfo);

#ifdef DEBUG_XZYNQ
int yints = 0;
unsigned int sa[64] = { 0 };
unsigned int zz = 0;
#endif

uint8_t can_get_mode(CANDEV_XZYNQ_INFO *devinfo)
{
	uint32_t reg = in32(devinfo->base + XZYNQ_CAN_SR_OFFSET);

	if (reg & XZYNQ_CAN_SR_CONFIG_MASK) {
		return XZYNQ_CAN_MODE_CONFIG;
	} else if (reg & XZYNQ_CAN_SR_SLEEP_MASK) {
		return XZYNQ_CAN_MODE_SLEEP;
	} else if (reg & XZYNQ_CAN_SR_NORMAL_MASK) {
		if (reg & XZYNQ_CAN_SR_SNOOP_MASK)
			return XZYNQ_CAN_MODE_SNOOP;
		else
			return XZYNQ_CAN_MODE_NORMAL;
	} else {
		return XZYNQ_CAN_MODE_LOOPBACK;
	}
}

void can_set_mode(CANDEV_XZYNQ_INFO *devinfo, uint8_t mode)
{
	uint32_t current_mode = can_get_mode(devinfo);

	LOG("Setting mode to %d\n", mode);

	/*
	 * If current mode is Normal Mode and the mode to enter is Sleep Mode,
	 * or if current mode is Sleep Mode and the mode to enter is Normal
	 * Mode, no transition through Configuration Mode is needed.
	 */
	if ((current_mode == XZYNQ_CAN_MODE_NORMAL) && (mode
			== XZYNQ_CAN_MODE_SLEEP)) {
		/* Normal Mode ---> Sleep Mode */
		out32(devinfo->base + XZYNQ_CAN_MSR_OFFSET, XZYNQ_CAN_MSR_SLEEP_MASK);
		return;

	} else if ((current_mode == XZYNQ_CAN_MODE_SLEEP) && (mode
			== XZYNQ_CAN_MODE_NORMAL)) {
		/* Sleep Mode ---> Normal Mode */
		out32(devinfo->base + XZYNQ_CAN_MSR_OFFSET, 0);
		return;
	}

	/*
	 * If the mode transition is not any of the two cases above, CAN must
	 * enter Configuration Mode before switching into the target operation
	 * mode.
	 */
	out32(devinfo->base + XZYNQ_CAN_SRR_OFFSET, 0);

	/*
	 * Check if the device has entered Configuration Mode, if not, return to
	 * the caller.
	 */
	if (can_get_mode(devinfo) != XZYNQ_CAN_MODE_CONFIG)
		return;

	switch (mode) {
	case XZYNQ_CAN_MODE_CONFIG:
		/*
		 * As CAN is in Configuration Mode already.
		 * Nothing is needed to be done here.
		 */
		break;
	case XZYNQ_CAN_MODE_SLEEP:
		out32(devinfo->base + XZYNQ_CAN_MSR_OFFSET, XZYNQ_CAN_MSR_SLEEP_MASK);
		out32(devinfo->base + XZYNQ_CAN_SRR_OFFSET, XZYNQ_CAN_SRR_CEN_MASK);
		break;
	case XZYNQ_CAN_MODE_NORMAL:
		out32(devinfo->base + XZYNQ_CAN_MSR_OFFSET, 0);
		out32(devinfo->base + XZYNQ_CAN_SRR_OFFSET, XZYNQ_CAN_SRR_CEN_MASK);
		break;
	case XZYNQ_CAN_MODE_LOOPBACK:
		out32(devinfo->base + XZYNQ_CAN_MSR_OFFSET, XZYNQ_CAN_MSR_LBACK_MASK);
		out32(devinfo->base + XZYNQ_CAN_SRR_OFFSET, XZYNQ_CAN_SRR_CEN_MASK);
		break;
	case XZYNQ_CAN_MODE_SNOOP:
		out32(devinfo->base + XZYNQ_CAN_MSR_OFFSET, XZYNQ_CAN_MSR_SNOOP_MASK);
		out32(devinfo->base + XZYNQ_CAN_SRR_OFFSET, XZYNQ_CAN_SRR_CEN_MASK);
		break;
	}
}

static void can_set_timestamp(CANDEV_XZYNQ_INFO *devinfo, uint32_t timestamp)
{
	if (timestamp == 0) {
		out32(devinfo->base + XZYNQ_CAN_TCR_OFFSET, XZYNQ_CAN_TCR_CTS_MASK);
		fprintf(stderr, "Timestamp reset\n");
	} else {
		fprintf(stderr, "Controller can only reset timestamp\n");
	}
}

static void can_setfilter(CANDEV_XZYNQ_INFO *devinfo, int mbxid, int mask)
{
	uint32_t reg;
	uint32_t id_reg = 0;
	uint32_t mask_reg = 0;
	uint32_t id = devinfo->canmsg[mbxid].id;
	uint32_t retry = 3;
	CAN_MSG_OBJ *msg = &devinfo->canmsg[mbxid];

	/* Checking the filters are not busy */
	while ((in32(devinfo->base + XZYNQ_CAN_SR_OFFSET) & XZYNQ_CAN_SR_ACFBSY_MASK) && retry--) {
		/* Wait for 10ms */
		usleep(10000);
	}

	if (retry < 0) {
		LOG("Acceptance Filter busy\n");
		return;
	}

	if (!msg->ide) {
		/* Setting up the 11-bit ID field */
		id_reg = (id & XZYNQ_CAN_IDR_STD_MASK) << XZYNQ_CAN_IDR_ID1_SHIFT;
	} else {
		/* Setting up the 29-bit ID field */
		id_reg = ((id & XZYNQ_CAN_IDR_EXT11_MASK) >> XZYNQ_CAN_IDR_EXT11_SHIFT)
			<< XZYNQ_CAN_IDR_ID1_SHIFT;
		id_reg |= (id & XZYNQ_CAN_IDR_EXT18_MASK) << XZYNQ_CAN_IDR_ID2_SHIFT;
	}
	id_reg |= msg->ide << XZYNQ_CAN_IDR_IDE_SHIFT;
	id_reg |= msg->srr << XZYNQ_CAN_IDR_SRR_SHIFT;
	id_reg |= msg->rtr << XZYNQ_CAN_IDR_IDE_SHIFT;

	if (!msg->ide) {
		/* Setting up the 11-bit mask field */
		mask_reg = (mask & XZYNQ_CAN_IDR_STD_MASK) << XZYNQ_CAN_IDR_ID1_SHIFT;
	} else {
		/* Setting up the 29-bit mask field */
		mask_reg = ((mask & XZYNQ_CAN_IDR_EXT11_MASK) >> XZYNQ_CAN_IDR_EXT11_SHIFT)
			<< XZYNQ_CAN_IDR_ID1_SHIFT;
		mask_reg |= (mask & XZYNQ_CAN_IDR_EXT18_MASK) << XZYNQ_CAN_IDR_ID2_SHIFT;
	}

	/* Get current filtering configuration */
	reg = in32(devinfo->base + XZYNQ_CAN_AFR_OFFSET);

	switch (mbxid) {
	case 0:
		/* Deactivate filter */
		out32(devinfo->base + XZYNQ_CAN_AFR_OFFSET, reg
				& ~XZYNQ_CAN_AFR_UAF1_MASK);
		if (!mask) {
			LOG("Deleted filter 1\n");
		} else {
			LOG("Using filter 1 with ID %#8.8x mask %#8.8x\n", id, mask);
			/* Set Mask and ID ((AFMR & Message_ID) == (AFMR & AFIR)) */
			out32(devinfo->base + XZYNQ_CAN_AFMR1_OFFSET, mask_reg);
			out32(devinfo->base + XZYNQ_CAN_AFIR1_OFFSET, id_reg);
			/* Activate filter */
			out32(devinfo->base + XZYNQ_CAN_AFR_OFFSET, reg
					| XZYNQ_CAN_AFR_UAF1_MASK);
		}
		break;
	case 1:
		/* Deactivate filter */
		out32(devinfo->base + XZYNQ_CAN_AFR_OFFSET, reg
				& ~XZYNQ_CAN_AFR_UAF2_MASK);
		if (!mask) {
			LOG("Deleted filter 2\n");
		} else {
			LOG("Using filter 2 with ID %#8.8x mask %#8.8x\n", id, mask);
			/* Set Mask and ID ((AFMR & Message_ID) == (AFMR & AFIR)) */
			out32(devinfo->base + XZYNQ_CAN_AFMR2_OFFSET, mask_reg);
			out32(devinfo->base + XZYNQ_CAN_AFIR2_OFFSET, id_reg);
			/* Activate filter */
			out32(devinfo->base + XZYNQ_CAN_AFR_OFFSET, reg
					| XZYNQ_CAN_AFR_UAF2_MASK);
		}
		break;
	case 2:
		/* Deactivate filter */
		out32(devinfo->base + XZYNQ_CAN_AFR_OFFSET, reg
				& ~XZYNQ_CAN_AFR_UAF3_MASK);
		if (!mask) {
			LOG("Deleted filter 3\n");
		} else {
			LOG("Using filter 3 with ID %#8.8x mask %#8.8x\n", id, mask);
			/* Set Mask and ID ((AFMR & Message_ID) == (AFMR & AFIR)) */
			out32(devinfo->base + XZYNQ_CAN_AFMR3_OFFSET, mask_reg);
			out32(devinfo->base + XZYNQ_CAN_AFIR3_OFFSET, id_reg);
			/* Activate filter */
			out32(devinfo->base + XZYNQ_CAN_AFR_OFFSET, reg
					| XZYNQ_CAN_AFR_UAF3_MASK);
		}
		break;
	case 3:
		/* Deactivate filter */
		out32(devinfo->base + XZYNQ_CAN_AFR_OFFSET, reg
				& ~XZYNQ_CAN_AFR_UAF4_MASK);
		if (!mask) {
			LOG("Deleted filter 4\n");
		} else {
			LOG("Using filter 4 with ID %#8.8x mask %#8.8x\n", id, mask);
			/* Set Mask and ID ((AFMR & Message_ID) == (AFMR & AFIR)) */
			out32(devinfo->base + XZYNQ_CAN_AFMR4_OFFSET, mask_reg);
			out32(devinfo->base + XZYNQ_CAN_AFIR4_OFFSET, id_reg);
			/* Activate filter */
			out32(devinfo->base + XZYNQ_CAN_AFR_OFFSET, reg
					| XZYNQ_CAN_AFR_UAF4_MASK);
		}
		break;
	default:
		fprintf(stderr, "Cannot set filter for index %d\n", mbxid);
		return;
	}
	/* Save the mask */
	msg->mask = mask;
}

/* CAN Bus error handler */
static void can_error_handler(CANDEV_XZYNQ_INFO *devinfo)
{
	uint32_t error;

	error = in32(devinfo->base + XZYNQ_CAN_ESR_OFFSET);
	if (error & XZYNQ_CAN_ESR_ACKER_MASK) {
		/* ACK Error */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_ESR_ACKER_MASK);
	}
	if (error & XZYNQ_CAN_ESR_BERR_MASK) {
		/* Bit Error */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_ESR_BERR_MASK);
	}
	if (error & XZYNQ_CAN_ESR_STER_MASK) {
		/* Stuff Error */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_ESR_STER_MASK);
	}
	if (error & XZYNQ_CAN_ESR_FMER_MASK) {
		/* Form Error */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_ESR_FMER_MASK);
	}
	if (error & XZYNQ_CAN_ESR_CRCER_MASK) {
		/* CRC Error */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_ESR_CRCER_MASK);
	}
	/* Clear Error Status Register */
	out32(devinfo->base + XZYNQ_CAN_ESR_OFFSET, error);
}

/* CAN Bus event handler */
static void can_event_handler(CANDEV_XZYNQ_INFO *devinfo, uint32_t event_int)
{
	/* Record interrupts in the errstatus for devctl */
	if (event_int & XZYNQ_CAN_IXR_BSOFF_MASK) {
		/* Bus Off: the CAN device should be reset and reconfigured */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_IXR_BSOFF_MASK << 16);
	}
	if (event_int & XZYNQ_CAN_IXR_RXOFLW_MASK) {
		/* RX FIFO Overflow */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_IXR_RXOFLW_MASK << 16);
	}
	if (event_int & XZYNQ_CAN_IXR_RXUFLW_MASK) {
		/* RX FIFO Underflow */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_IXR_RXUFLW_MASK << 16);
	}
	if (event_int & XZYNQ_CAN_IXR_TXBFLL_MASK) {
		/* TX High Priority Buffer Full */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_IXR_TXBFLL_MASK << 16);
	}
	if (event_int & XZYNQ_CAN_IXR_TXFLL_MASK) {
		/* TX FIFO Full */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_IXR_TXFLL_MASK << 16);
		out32(devinfo->base + XZYNQ_CAN_ISR_OFFSET, XZYNQ_CAN_IXR_TXFLL_MASK);
	}
	if (event_int & XZYNQ_CAN_IXR_WKUP_MASK) {
		/* Wake up from sleep mode */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_IXR_WKUP_MASK << 16);
	}
	if (event_int & XZYNQ_CAN_IXR_SLP_MASK) {
		/* Enter sleep mode */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_IXR_SLP_MASK << 16);
	}
	if (event_int & XZYNQ_CAN_IXR_ARBLST_MASK) {
		/* Lost bus arbitration */
		atomic_set(&devinfo->errstatus, XZYNQ_CAN_IXR_ARBLST_MASK << 16);
	}
}

/* Formatting the ID from a read ID register */
static uint32_t can_get_id(uint32_t id_reg)
{
	uint32_t id = 0;

	if (id_reg & XZYNQ_CAN_IDR_IDE_MASK) {
		id = ((id_reg & XZYNQ_CAN_IDR_ID1_MASK) >> XZYNQ_CAN_IDR_ID1_SHIFT) <<
			XZYNQ_CAN_IDR_EXT11_SHIFT;
		id |= (id_reg & XZYNQ_CAN_IDR_ID2_MASK) >> XZYNQ_CAN_IDR_ID2_SHIFT;
	} else {
		id = (id_reg & XZYNQ_CAN_IDR_ID1_MASK) >> XZYNQ_CAN_IDR_ID1_SHIFT;
	}

	return id;
}

/* CAN Interrupt Handler */
const struct sigevent *can_intr(void *area, int id)
{
	struct sigevent *event = NULL;
	CANDEV_XZYNQ_INFO *devinfo = area;
	CANDEV_XZYNQ *devlist = devinfo->devlist;
	uint32_t pending_int;
	uint32_t event_int;
	uint32_t *val32;
	uint32_t int_enable;
	uint32_t id_read = 0;
	uint32_t dlc = 0;
	uint32_t datal;
	uint32_t datah;
	canmsg_list_t *msglist = NULL;
	canmsg_t *txmsg = NULL;
	canmsg_t *rxmsg = NULL;
	int mbxid = 0;
	int i;

	/* Getting source(s) of interrupt */
	pending_int = in32(devinfo->base + XZYNQ_CAN_ISR_OFFSET);
	pending_int &= in32(devinfo->base + XZYNQ_CAN_IER_OFFSET);

	/* Disable all interrupts */
	out32(devinfo->base + XZYNQ_CAN_IER_OFFSET, 0);

	/* Clear all interrupts flags */
	out32(devinfo->base + XZYNQ_CAN_ICR_OFFSET, pending_int);

#ifdef DEBUG_XZYNQ
	yints++;
	if (zz < (sizeof(sa)/sizeof(unsigned int)))
		sa[zz++] = pending_int;
#endif

	/* An error interrupt is occurring */
	if (pending_int & XZYNQ_CAN_IXR_ERROR_MASK)
		can_error_handler(devinfo);

	/*
	 * Check if any following event interrupt is pending:
	 *	  - RX FIFO Overflow
	 *	  - RX FIFO Underflow
	 *	  - TX High Priority Buffer full
	 *	  - TX FIFO Full
	 *	  - Wake up from sleep mode
	 *	  - Enter sleep mode
	 *	  - Enter Bus off status
	 *	  - Arbitration is lost
	 */
	event_int = pending_int & XZYNQ_CAN_IXR_EVT_ALL;
	if (event_int)
		can_event_handler(devinfo, event_int);

	/* RX interrupt */
	if (pending_int & XZYNQ_CAN_IXR_RX_ALL) {
		/* Read all the RX registers remove one message from it */
		id_read = can_get_id(in32(devinfo->base + XZYNQ_CAN_RXFIFO_ID_OFFSET));
		dlc = in32(devinfo->base + XZYNQ_CAN_RXFIFO_DLC_OFFSET);
		datal = in32(devinfo->base + XZYNQ_CAN_RXFIFO_DW1_OFFSET);
		datah = in32(devinfo->base + XZYNQ_CAN_RXFIFO_DW2_OFFSET);

		/* Check if filters are used */
		if (in32(devinfo->base + XZYNQ_CAN_AFR_OFFSET)
				& XZYNQ_CAN_AFR_UAF_ALL_MASK) {
			/* Check to whom it was intended */
			for (i = 0; i < devinfo->numrx; i++) {
				if ((id_read & devinfo->canmsg[i].mask) ==
					(devinfo->canmsg[i].id & devinfo->canmsg[i].mask))
					break;
			}
		} else {
			/* Otherwise the message goes to rx0 by default */
			i = 0;
		}

		if (i < devinfo->numrx) {
			/* Get the mailbox's data message list */
			mbxid = i;
			msglist = devlist[mbxid].cdev.msg_list;
			/* Get the next free receive message */
			if ((rxmsg = msg_enqueue_next(msglist))) {
				/* Access the data as a uint32_t array */
				val32 = (uint32_t *) rxmsg->cmsg.dat;

				val32[0] = datal;
				val32[1] = datah;

				/* Save read values for debug purposes */
				devinfo->canmsg[mbxid].canmdl = datal;
				devinfo->canmsg[mbxid].canmdh = datah;

				if (devinfo->iflags & INFO_FLAGS_ENDIAN_SWAP) {
					/* Convert from Big Endian to Little Endian since data is received MSB */
					ENDIAN_SWAP32(&val32[0]);
					ENDIAN_SWAP32(&val32[1]);
				}

				/* Determine if we should store away extra receive message info */
				if (devinfo->iflags & INFO_FLAGS_RX_FULL_MSG) {
					/* Save the message id */
					rxmsg->cmsg.mid = id_read;
					/* Save message timestamp */
					rxmsg->cmsg.ext.timestamp = dlc
							& XZYNQ_CAN_DLCR_TIMESTAMP_MASK;
				}

				/* Indicate receive message is ready with data */
				msg_enqueue(msglist);
			}
		}
	}

	/* TX interrupt */
	if (pending_int & XZYNQ_CAN_IXR_TX_ALL) {
		/* Last node is TX in our case */
		mbxid = (devinfo->numrx + devinfo->numtx) - 1;
		msglist = devlist[mbxid].cdev.msg_list;
		/* Get next transmit message */
		if ((txmsg = msg_dequeue_next(msglist))) {
			/* Acknowledge message was transmitted */
			msg_dequeue(msglist);

			/* Determine if there is another message to transmit */
			if ((txmsg = msg_dequeue_next(msglist))) {
				/* Transmit next message */
				can_tx(&devlist[mbxid], txmsg);
			} else {
				/* No more transmit messages, end transmission */
				atomic_clr(&devlist[mbxid].dflags,
						CANDEV_XZYNQ_TX_ENABLED);
			}
		}
	}

	/* Making sure the default interrupts are enabled */
	int_enable = in32(devinfo->base + XZYNQ_CAN_IER_OFFSET)
			| XZYNQ_CAN_IXR_DEFAULT;
	out32(devinfo->base + XZYNQ_CAN_IER_OFFSET, int_enable);

	/* Check if we have to notify any blocked clients */
	event = can_client_check(&devlist[mbxid].cdev);
	InterruptUnlock(&can_lock);
	return event;
}

/* LIBCAN driver transmit function */
void can_drvr_transmit(CANDEV *cdev)
{
	CANDEV_XZYNQ *dev = (CANDEV_XZYNQ *) cdev;
	canmsg_t * txmsg;

	/* Make sure transmit isn't already in progress and there is valid data */
	if (!(dev->dflags & CANDEV_XZYNQ_TX_ENABLED) && (txmsg = msg_dequeue_next(
			cdev->msg_list))) {
		/* Indicate transmit is in progress */
		atomic_set(&dev->dflags, CANDEV_XZYNQ_TX_ENABLED);
		/* Start transmission */
		can_tx(dev, txmsg);
	}
}

/* LIBCAN driver devctl function */
int can_drvr_devctl(CANDEV *cdev, int dcmd, DCMD_DATA *data)
{
	CANDEV_XZYNQ *dev = (CANDEV_XZYNQ *) cdev;
	CANDEV_XZYNQ_INFO *devinfo = dev->devinfo;
	int mbxid = dev->mbxid;

	switch (dcmd) {
	case CAN_DEVCTL_SET_MID:
		/* Set new message id */
		if (!devinfo->canmsg[mbxid].ide)
			devinfo->canmsg[mbxid].id = data->mid & XZYNQ_CAN_IDR_STD_MASK;
		else
			devinfo->canmsg[mbxid].id = data->mid & (XZYNQ_CAN_IDR_EXT11_MASK |
								 XZYNQ_CAN_IDR_EXT18_MASK);
		/* Reset RX filter with same mask but different ID */
		if (mbxid < devinfo->numrx) {
			can_setfilter(devinfo, mbxid, devinfo->canmsg[mbxid].mask);
		}
		break;
	case CAN_DEVCTL_GET_MID:
		/* Read device's current message id (removing non-message id bits) */
		data->mid = devinfo->canmsg[mbxid].id;
		break;
	case CAN_DEVCTL_SET_MFILTER:
		/* Make sure this is a receive mailbox */
		if (!(mbxid < devinfo->numrx)) {
			fprintf(stderr, "Filtering only available for RX nodes\n");
			return (EINVAL);
		}
		can_setfilter(devinfo, mbxid, data->mfilter);
		break;
	case CAN_DEVCTL_GET_MFILTER:
		/* Make sure this is a receive mailbox */
		if (!(mbxid < devinfo->numrx)) {
			fprintf(stderr, "Filtering only available for RX nodes\n");
			return (EINVAL);
		}
		data->mfilter = devinfo->canmsg[mbxid].mask;
		break;
	case CAN_DEVCTL_SET_PRIO:
		/* Can only set the priority for TX frames */
		if (mbxid < devinfo->numrx) {
			fprintf(stderr, "RX priority is ID dependent only\n");
		} else {
			fprintf(stderr, "TX priority set to %s\n", (data->prio)?"high":"low");
			devinfo->canmsg[mbxid].prio = data->prio;
		}
		break;
	case CAN_DEVCTL_GET_PRIO:
		data->prio = devinfo->canmsg[mbxid].prio;
		break;
	case CAN_DEVCTL_SET_TIMESTAMP:
		/* Set the current time stamp */
		can_set_timestamp(devinfo, data->timestamp);
		break;
	case CAN_DEVCTL_GET_TIMESTAMP:
		fprintf(stderr, "Not supported by controller\n");
		break;
	case CAN_DEVCTL_READ_CANMSG_EXT:
		/* This is handled by the interrupt handler */
		break;
	case CAN_DEVCTL_ERROR:
		/* Read current state of CAN Error and Status register */
		data->error.drvr1 = in32(devinfo->base + XZYNQ_CAN_ESR_OFFSET);
		/* Read and clear previous devctl info */
		data->error.drvr2 = atomic_clr_value(&devinfo->errstatus, 0xffffffff);
		break;
	case CAN_DEVCTL_DEBUG_INFO:
		/* Print debug information */
		can_debug(devinfo);
		break;
	default:
		/* Driver does not support this DEVCTL */
		return (ENOTSUP);
	}

	return (EOK);
}

/* Initialize CAN device registers */
void can_init_intr(CANDEV_XZYNQ_INFO *devinfo, CANDEV_XZYNQ_INIT *devinit)
{
	/* Attach interrupt handler for system interrupts */
	devinfo->iidsys = InterruptAttach(devinit->irqsys, can_intr, devinfo, 0, 0);
	if (devinfo->iidsys == -1) {
		perror("InterruptAttach irqsys");
		exit(EXIT_FAILURE);
	}

	LOG("Registered interrupt %d successfully\n", devinit->irqsys);

	/* Setup Watermarks */
	out32(devinfo->base + XZYNQ_CAN_SRR_OFFSET, 0);
	out32(devinfo->base + XZYNQ_CAN_WIR_OFFSET, (1 << XZYNQ_CAN_WIR_EW_SHIFT) | 1);
	out32(devinfo->base + XZYNQ_CAN_SRR_OFFSET, XZYNQ_CAN_SRR_CEN_MASK);

	/* Enable all system interrupts to be generated on CAN interrupt lines */
	out32(devinfo->base + XZYNQ_CAN_IER_OFFSET, XZYNQ_CAN_IXR_DEFAULT);

	ThreadCtl(_NTO_TCTL_IO, 0); // required for InterruptLock/Unlock abilities
	memset(&can_lock, 0, sizeof(can_lock));
}

/* Initialize CAN clocks */
void can_init_clocks(CANDEV_XZYNQ_INIT *devinit)
{
	int fd, err;
	clk_freq_t clk_freq;

	fd = open("/dev/clock", O_RDWR);
	
	if (fd == -1) {
		perror("CAN Clocks");
		return;
	}

	/* Enable the clock */
	switch (devinit->port) {
	default:
		fprintf(stderr, "Unsupported port %08x, using CAN0\n", devinit->port);
		devinit->port = XZYNQ_CAN0_BASEADDR;
	case XZYNQ_CAN0_BASEADDR:
		clk_freq.id = XZYNQ_CAN0_CTRL;
		err = devctl(fd, DCMD_CLOCK_ENABLE, &clk_freq, sizeof(clk_freq_t), NULL);
		if (err) {
			perror("devctl");
			return;
		}
		break;
	case XZYNQ_CAN1_BASEADDR:
		clk_freq.id = XZYNQ_CAN1_CTRL;
		err = devctl(fd, DCMD_CLOCK_ENABLE, &clk_freq, sizeof(clk_freq_t), NULL);
		if (err) {
			perror("devctl");
			return;
		}
		break;
	}

	/* Set the frequency */
	clk_freq.id = XZYNQ_CAN_CLK;
	clk_freq.freq = devinit->clk;
	err = devctl(fd, DCMD_CLOCK_SET_FREQ, &clk_freq, sizeof(clk_freq_t), NULL);
	if (err) {
		perror("devctl");
		return;
	}

	err = devctl(fd, DCMD_CLOCK_GET_FREQ, &clk_freq, sizeof(clk_freq_t), NULL);
	if (err) {
		perror("devctl");
		return;
	}

	/* Set real frequency */
	devinit->clk = clk_freq.freq;
	close(fd);

	LOG("CAN CLK: %u\n", devinit->clk);
}

/* Initialize CAN device registers */
void can_init_hw(CANDEV_XZYNQ_INFO *devinfo, CANDEV_XZYNQ_INIT *devinit)
{
	uint32_t reg;
	uint32_t i = 0;

	/*
	 * Enter configuration mode by doing a software reset
	 */
	out32(devinfo->base + XZYNQ_CAN_SRR_OFFSET, XZYNQ_CAN_SRR_SRST_MASK);
	while (can_get_mode(devinfo) != XZYNQ_CAN_MODE_CONFIG)
		/* Added timeout as function can be called within interrupt */
		if (i++ > 100)
			return;

	/*
	 * Setup Baud Rate Prescaler Register (BRPR) and Bit Timing Register
	 * (BTR) such that CAN baud rate equals 40Kbps, given the CAN clock
	 * equal to 24MHz. For more information see the CAN 2.0A, CAN 2.0B,
	 * ISO 11898-1 specifications.
	 */
	out32(devinfo->base + XZYNQ_CAN_BRPR_OFFSET, devinit->br_brp);
	out32(devinfo->base + XZYNQ_CAN_BTR_OFFSET, (devinit->br_sjw
			<< XZYNQ_CAN_BTR_SJW_SHIFT) || (devinit->br_tseg1
			<< XZYNQ_CAN_BTR_TS1_SHIFT) || (devinit->br_tseg2
			<< XZYNQ_CAN_BTR_TS2_SHIFT));

	/*
	 * Setting up the CAN mode requested
	 */
	if (devinit->flags & INIT_FLAGS_LOOPBACK) {
		can_set_mode(devinfo, XZYNQ_CAN_MODE_LOOPBACK);
	} else if (devinit->flags & INIT_FLAGS_SNOOP) {
		can_set_mode(devinfo, XZYNQ_CAN_MODE_SNOOP);
	} else {
		can_set_mode(devinfo, XZYNQ_CAN_MODE_NORMAL);
	}

	/* Set initial value for Local Network Time */
	if (devinit->flags & INIT_FLAGS_TIMESTAMP) {
		can_set_timestamp(devinfo, devinit->timestamp);
	}

	/* Clear interrupt registers */
	reg = in32(devinfo->base + XZYNQ_CAN_ISR_OFFSET);
	out32(devinfo->base + XZYNQ_CAN_ICR_OFFSET, reg);
}

static void can_init_msg_tx(CANDEV_XZYNQ_INFO *devinfo,
		CANDEV_XZYNQ_INIT *devinit, int id, int mbxid)
{
	CAN_MSG_OBJ *msg = &devinfo->canmsg[id];

	msg->rtr = 0; /* RTR (remote transmission) field for atomic transactions */
	msg->srr = 0; /* SRR is a dummy bit to let standard format RTR messages win arbitration */
	msg->prio = 0; /* Reset priority value to low for TX nodes */

	msg->id = devinit->midtx + CANMID_DEFAULT * mbxid;

	msg->dlc = devinit->cinit.datalen;

	if (devinit->flags & INIT_FLAGS_EXTENDED_MID) {
		msg->ide = 1; /* The extended identifier bit (IDE) is used for acceptance filtering. */
		/* Shift value by 18 bits */
		msg->id <<= 18;
	} else {
		msg->ide = 0; /* The extended identifier bit (IDE) has no effect on the acceptance filtering. */
	}

	LOG("TX: [%d] %x\n", id, msg->id);
}

static void can_init_msg_rx(CANDEV_XZYNQ_INFO *devinfo,
		CANDEV_XZYNQ_INIT *devinit, int id, int mbxid)
{
	CAN_MSG_OBJ *msg = &devinfo->canmsg[id];

	msg->rtr = 0; /* RTR (remote transmission) field for atomic transactions */
	msg->srr = 0; /* SRR is a dummy bit to let standard format RTR messages win arbitration */
	/* The receive priority for the message objects is attached to the CAN identifier. */
	msg->prio = devinfo->numrx - id;

	msg->id = devinit->midrx + CANMID_DEFAULT * mbxid;

	msg->dlc = devinit->cinit.datalen;

	if (devinit->flags & INIT_FLAGS_EXTENDED_MID) {
		/* 29-bit message ID */
		msg->ide = 1;
		/* Shift value by 18 bits */
		msg->id <<= 18;
		can_setfilter(devinfo, mbxid, XZYNQ_CAN_IDR_EXT11_MASK |
				XZYNQ_CAN_IDR_EXT11_MASK);
	} else {
		/* 11-bit message ID */
		msg->ide = 0;
		can_setfilter(devinfo, mbxid, XZYNQ_CAN_IDR_STD_MASK);
	}

	LOG("RX: [%d] %x\n", id, msg->id);
}

/* Initialize CAN mailboxes in device memory */
void can_init_mailbox(CANDEV_XZYNQ_INFO *devinfo, CANDEV_XZYNQ_INIT *devinit)
{
	int i;
	int counter = 0;

	/* Configure Receive Mailboxes */
	counter = 0;
	for (i = 0; i < devinfo->numrx; i++) {
		can_init_msg_rx(devinfo, devinit, i, counter++);
	}

	/* Configure Transmit Mailboxes */
	counter = 0;
	for (i = devinfo->numrx; i < (devinfo->numrx+devinfo->numtx); i++) {
		can_init_msg_tx(devinfo, devinit, i, counter++);
	}
}

/* Transmit a CAN message from the specified mailbox */
void can_tx(CANDEV_XZYNQ *dev, canmsg_t *txmsg)
{
	CANDEV_XZYNQ_INFO *devinfo = dev->devinfo;
	int mbxid = dev->mbxid;
	uint32_t id_reg, dlc_reg;
	uint32_t *val32 = (uint32_t *) txmsg->cmsg.dat;

	/* Copy message data into transmit mailbox */
	devinfo->canmsg[mbxid].canmdl = val32[0];
	devinfo->canmsg[mbxid].canmdh = val32[1];

	if (devinfo->iflags & INFO_FLAGS_ENDIAN_SWAP) {
		/* Convert from Little Endian to Big Endian since data is transmitted MSB */
		ENDIAN_SWAP32(&devinfo->canmsg[mbxid].canmdl);
		ENDIAN_SWAP32(&devinfo->canmsg[mbxid].canmdh);
	}

	if (!devinfo->canmsg[mbxid].ide) {
		/* Setting up the 11-bit ID field */
		id_reg = (devinfo->canmsg[mbxid].id & XZYNQ_CAN_IDR_STD_MASK) << XZYNQ_CAN_IDR_ID1_SHIFT;
	} else {
		/* Setting up the 29-bit ID field */
		id_reg = ((devinfo->canmsg[mbxid].id & XZYNQ_CAN_IDR_EXT11_MASK)
			  >> XZYNQ_CAN_IDR_EXT11_SHIFT) << XZYNQ_CAN_IDR_ID1_SHIFT;
		id_reg |= (devinfo->canmsg[mbxid].id & XZYNQ_CAN_IDR_EXT18_MASK)
				<< XZYNQ_CAN_IDR_ID2_SHIFT;
	}
	id_reg |= devinfo->canmsg[mbxid].ide << XZYNQ_CAN_IDR_IDE_SHIFT;
	id_reg |= devinfo->canmsg[mbxid].srr << XZYNQ_CAN_IDR_SRR_SHIFT;
	id_reg |= devinfo->canmsg[mbxid].rtr << XZYNQ_CAN_IDR_IDE_SHIFT;

	/* Setting up the data length field */
	dlc_reg = (devinfo->canmsg[mbxid].dlc << XZYNQ_CAN_DLCR_DLC_SHIFT)
			& XZYNQ_CAN_DLCR_DLC_MASK;

	/* Making sure FIFO is not full */
	if (devinfo->canmsg[mbxid].prio) {
		if (in32(devinfo->base + XZYNQ_CAN_ISR_OFFSET)
				& XZYNQ_CAN_IXR_TXBFLL_MASK) {
			/*
			 * Cannot wait for FIFO to be emptied as this
			 * code can be called from the interrupt handler
			 */
			return;
		}
	} else {
		if (in32(devinfo->base + XZYNQ_CAN_ISR_OFFSET)
				& XZYNQ_CAN_IXR_TXFLL_MASK) {
			/*
			 * Cannot wait for FIFO to be emptied as this
			 * code can be called from the interrupt handler
			 */
			return;
		}
	}

	/* Transmit message */
	if (devinfo->canmsg[mbxid].prio) {
		out32(devinfo->base + XZYNQ_CAN_TXHPB_ID_OFFSET, id_reg);
		out32(devinfo->base + XZYNQ_CAN_TXHPB_DLC_OFFSET, dlc_reg);
		out32(devinfo->base + XZYNQ_CAN_TXHPB_DW1_OFFSET,
				devinfo->canmsg[mbxid].canmdl);
		out32(devinfo->base + XZYNQ_CAN_TXHPB_DW2_OFFSET,
				devinfo->canmsg[mbxid].canmdh);
	} else {
		out32(devinfo->base + XZYNQ_CAN_TXFIFO_ID_OFFSET, id_reg);
		out32(devinfo->base + XZYNQ_CAN_TXFIFO_DLC_OFFSET, dlc_reg);
		out32(devinfo->base + XZYNQ_CAN_TXFIFO_DW1_OFFSET,
				devinfo->canmsg[mbxid].canmdl);
		out32(devinfo->base + XZYNQ_CAN_TXFIFO_DW2_OFFSET,
				devinfo->canmsg[mbxid].canmdh);
	}
}

/* Print out debug information */
void can_debug(CANDEV_XZYNQ_INFO *devinfo)
{
	fprintf(stderr, "\nCAN REG\n");
	can_print_reg(devinfo);
	fprintf(stderr, "\nMailboxes\n");
	can_print_mbox(devinfo);
}

/* Print CAN device registers */
#define PRINT_REG(addr)	fprintf(stderr,  "%s = %#8.8x\n", #addr, in32(devinfo->base+addr))
void can_print_reg(CANDEV_XZYNQ_INFO *devinfo)
{
	fprintf(stderr,"\n**********************************************************************************\n");
	PRINT_REG(XZYNQ_CAN_SRR_OFFSET);
	PRINT_REG(XZYNQ_CAN_MSR_OFFSET);
	PRINT_REG(XZYNQ_CAN_BRPR_OFFSET);
	PRINT_REG(XZYNQ_CAN_BTR_OFFSET);
	PRINT_REG(XZYNQ_CAN_ECR_OFFSET);
	PRINT_REG(XZYNQ_CAN_ESR_OFFSET);
	PRINT_REG(XZYNQ_CAN_SR_OFFSET);
	PRINT_REG(XZYNQ_CAN_ISR_OFFSET);
	PRINT_REG(XZYNQ_CAN_IER_OFFSET);
	PRINT_REG(XZYNQ_CAN_TCR_OFFSET);
	PRINT_REG(XZYNQ_CAN_WIR_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFR_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFMR1_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFIR1_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFMR2_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFIR2_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFMR3_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFIR3_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFMR4_OFFSET);
	PRINT_REG(XZYNQ_CAN_AFIR4_OFFSET);
#ifdef DEBUG_XZYNQ
	{
		int i;
		fprintf(stderr, "\n# of interrupts: %d\n", yints);
		fprintf(stderr, "Interrupts pending values:");
		for (i=0; i<sizeof(sa); i++) {
			if (sa[i] == 0)
				break;
			if (!(i%8))
				fprintf(stderr, "\n");
			fprintf(stderr, "%08x ", sa[i]);
		}
		fprintf(stderr, "\n\n");
	}
#endif
	fprintf(stderr,"**********************************************************************************\n");
}

/* Print CAN device mailbox memory */
void can_print_mbox(CANDEV_XZYNQ_INFO *devinfo)
{
	int i = 0;

	fprintf(stderr, "%s\n", devinfo->canmsg[i].ide?"Extended ID mode":"Standard ID mode");
	fprintf(stderr, "RX Mailboxes\n");
	fprintf(stderr, "MB\tID\t\tMASK\t\tDLC\tMDL\t\tMDH\n");
	fprintf(stderr,"==================================================================================\n");
	for (i = 0; i < devinfo->numrx; i++) {
		fprintf(stderr, "RX%d\t0x%8.8x\t0x%8.8x\t%d\t0x%8.8x\t0x%8.8x\n", i,
				devinfo->canmsg[i].id, devinfo->canmsg[i].mask,
				devinfo->canmsg[i].dlc, devinfo->canmsg[i].canmdl,
				devinfo->canmsg[i].canmdh);
	}
	fprintf(stderr, "\nTX Mailboxes\n");
	fprintf(stderr, "MB\tID\t\tMASK\t\tDLC\tMDL\t\tMDH\n");
	fprintf(stderr,"==================================================================================\n");
	for (i = devinfo->numrx; i < (devinfo->numrx+devinfo->numtx); i++) {
		fprintf(stderr, "TX%d\t0x%8.8x\t0x%8.8x\t%d\t0x%8.8x\t0x%8.8x\n", i,
				devinfo->canmsg[i].id, devinfo->canmsg[i].mask,
				devinfo->canmsg[i].dlc, devinfo->canmsg[i].canmdl,
				devinfo->canmsg[i].canmdh);
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/can/xzynq/canxzynq.c $ $Rev: 752035 $")
#endif
