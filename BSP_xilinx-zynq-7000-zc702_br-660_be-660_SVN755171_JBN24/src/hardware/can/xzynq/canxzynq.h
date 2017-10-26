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
#ifndef __CANXZYNQ_H_INCLUDED
#define __CANXZYNQ_H_INCLUDED

#include <arm/xzynq.h>
#include <hw/can.h>

/*
 * Mailbox size if 5 as follow
 * 4 receiver nodes that correspond to the 4 available filters
 *   -> data goes to the first node whose ID matches
 * 1 transmitter than can send with 2 different priorities
 */
#define XZYNQ_CAN_NUM_MAX_RX	(4)
#define XZYNQ_CAN_NUM_MAX_TX	(1)
#define XZYNQ_CAN_NUM_MAILBOX	(XZYNQ_CAN_NUM_MAX_RX + XZYNQ_CAN_NUM_MAX_TX)

/* Definitions for CAN modes */
#define XZYNQ_CAN_MODE_CONFIG	0x00000001 /**< Configuration mode */
#define XZYNQ_CAN_MODE_NORMAL	0x00000002 /**< Normal mode */
#define XZYNQ_CAN_MODE_LOOPBACK	0x00000004 /**< Loop Back mode */
#define XZYNQ_CAN_MODE_SLEEP	0x00000008 /**< Sleep mode */
#define XZYNQ_CAN_MODE_SNOOP	0x00000010 /**< Snoop mode */

/* Register offsets for the CAN. Each register is 32 bits. */
#define XZYNQ_CAN_SRR_OFFSET	  	0x00 /**< Software Reset Register */
#define XZYNQ_CAN_MSR_OFFSET	  	0x04 /**< Mode Select Register */
#define XZYNQ_CAN_BRPR_OFFSET	  	0x08 /**< Baud Rate Prescaler */
#define XZYNQ_CAN_BTR_OFFSET	  	0x0C /**< Bit Timing Register */
#define XZYNQ_CAN_ECR_OFFSET	  	0x10 /**< Error Counter Register */
#define XZYNQ_CAN_ESR_OFFSET	  	0x14 /**< Error Status Register */
#define XZYNQ_CAN_SR_OFFSET	  		0x18 /**< Status Register */
#define XZYNQ_CAN_ISR_OFFSET	  	0x1C /**< Interrupt Status Register */
#define XZYNQ_CAN_IER_OFFSET	  	0x20 /**< Interrupt Enable Register */
#define XZYNQ_CAN_ICR_OFFSET	  	0x24 /**< Interrupt Clear Register */
#define XZYNQ_CAN_TCR_OFFSET	  	0x28 /**< Timestamp Control Register */
#define XZYNQ_CAN_WIR_OFFSET	  	0x2C /**< Watermark Interrupt Reg */
#define XZYNQ_CAN_TXFIFO_ID_OFFSET  0x30 /**< TX FIFO id */
#define XZYNQ_CAN_TXFIFO_DLC_OFFSET 0x34 /**< TX FIFO dlc */
#define XZYNQ_CAN_TXFIFO_DW1_OFFSET	0x38 /**< TX FIFO Data Word 1 */
#define XZYNQ_CAN_TXFIFO_DW2_OFFSET 0x3C /**< TX FIFO Data Word 2 */
#define XZYNQ_CAN_TXHPB_ID_OFFSET   0x40 /**< TX High Priority Buffer id */
#define XZYNQ_CAN_TXHPB_DLC_OFFSET  0x44 /**< TX High Priority Buffer dlc */
#define XZYNQ_CAN_TXHPB_DW1_OFFSET  0x48 /**< TX High Priority Buf Data 1 */
#define XZYNQ_CAN_TXHPB_DW2_OFFSET  0x4C /**< TX High Priority Buf Data Word 2 */
#define XZYNQ_CAN_RXFIFO_ID_OFFSET  0x50 /**< RX FIFO id */
#define XZYNQ_CAN_RXFIFO_DLC_OFFSET 0x54 /**< RX FIFO dlc */
#define XZYNQ_CAN_RXFIFO_DW1_OFFSET 0x58 /**< RX FIFO Data Word 1 */
#define XZYNQ_CAN_RXFIFO_DW2_OFFSET 0x5C /**< RX FIFO Data Word 2 */
#define XZYNQ_CAN_AFR_OFFSET	  	0x60 /**< Acceptance Filter Register */
#define XZYNQ_CAN_AFMR1_OFFSET	  	0x64 /**< Acceptance Filter Mask 1 */
#define XZYNQ_CAN_AFIR1_OFFSET	  	0x68 /**< Acceptance Filter id  1 */
#define XZYNQ_CAN_AFMR2_OFFSET	  	0x6C /**< Acceptance Filter Mask  2 */
#define XZYNQ_CAN_AFIR2_OFFSET	  	0x70 /**< Acceptance Filter id 2 */
#define XZYNQ_CAN_AFMR3_OFFSET	  	0x74 /**< Acceptance Filter Mask 3 */
#define XZYNQ_CAN_AFIR3_OFFSET	  	0x78 /**< Acceptance Filter id 3 */
#define XZYNQ_CAN_AFMR4_OFFSET	  	0x7C /**< Acceptance Filter Mask  4 */
#define XZYNQ_CAN_AFIR4_OFFSET	  	0x80 /**< Acceptance Filter id 4 */

#define XZYNQ_CAN_REG_SIZE	  		0x84

/* Software Reset Register (SRR) Bit Definitions and Masks */
#define XZYNQ_CAN_SRR_CEN_MASK		0x00000002  /**< Can Enable */
#define XZYNQ_CAN_SRR_SRST_MASK		0x00000001  /**< Reset */

/* Mode Select Register (MSR) Bit Definitions and Masks */
#define XZYNQ_CAN_MSR_SNOOP_MASK	0x00000004 /**< Snoop Mode Select */
#define XZYNQ_CAN_MSR_LBACK_MASK	0x00000002 /**< Loop Back Mode Select */
#define XZYNQ_CAN_MSR_SLEEP_MASK	0x00000001 /**< Sleep Mode Select */

/* Baud Rate Prescaler register (BRPR) Bit Definitions and Masks */
#define XZYNQ_CAN_BRPR_BRP_MASK		0x000000FF /**< Baud Rate Prescaler */

#define XZYNQ_CAN_BRPR_10K_DEFAULT		199 /* 200 - 1 as starting from 0 */
#define XZYNQ_CAN_BRPR_20K_DEFAULT		 99 /* 100 - 1 as starting from 0 */
#define XZYNQ_CAN_BRPR_25K_DEFAULT		 79 /* 80 - 1 as starting from 0 */
#define XZYNQ_CAN_BRPR_50K_DEFAULT		 39 /* 40 - 1 as starting from 0 */
#define XZYNQ_CAN_BRPR_125K_DEFAULT		 23 /* 24 - 1 as starting from 0 */
#define XZYNQ_CAN_BRPR_100K_DEFAULT		 19 /* 20 - 1 as starting from 0 */
#define XZYNQ_CAN_BRPR_500K_DEFAULT		  3 /* 4 - 1 as starting from 0 */
#define XZYNQ_CAN_BRPR_1M_DEFAULT		  1 /* 2 - 1 as starting from 0 */
#define XZYNQ_CAN_BRPR_MAXVAL			255

// TODO do not hardcode clock values
#define XZYNQ_CAN_CLK_12MHZ		12000000
#define XZYNQ_CAN_CLK_24MHZ 	24000000
#define XZYNQ_CAN_CLK_48MHZ 	48000000

/* Bit Timing Register (BTR) Bit Definitions and Masks */
#define XZYNQ_CAN_BTR_SJW_MASK	0x00000180 /**< Synchronization Jump Width */
#define XZYNQ_CAN_BTR_SJW_SHIFT	7
#define XZYNQ_CAN_BTR_TS2_MASK	0x00000070 /**< Time Segment 2 */
#define XZYNQ_CAN_BTR_TS2_SHIFT	4
#define XZYNQ_CAN_BTR_TS1_MASK	0x0000000F /**< Time Segment 1 */
#define XZYNQ_CAN_BTR_TS1_SHIFT	0

/* Default timing values */
#define XZYNQ_CAN_BTR_TS1_DEFAULT	3 /* 4 - 1 as starting from 0 */
#define XZYNQ_CAN_BTR_TS1_MAXVAL	15
#define XZYNQ_CAN_BTR_TS2_DEFAULT	2 /* 3 - 1 as starting from 0 */
#define XZYNQ_CAN_BTR_TS2_MAXVAL	7
#define XZYNQ_CAN_BTR_SJW_DEFAULT	3 /* 4 - 1 as starting from 0 */
#define XZYNQ_CAN_BTR_SJW_MAXVAL	3

/* Error Counter Register (ECR) Bit Definitions and Masks */
#define XZYNQ_CAN_ECR_REC_MASK	0x0000FF00 /**< Receive Error Counter */
#define XZYNQ_CAN_ECR_REC_SHIFT	8
#define XZYNQ_CAN_ECR_TEC_MASK	0x000000FF /**< Transmit Error Counter */

/* Error Status Register (ESR) Bit Definitions and Masks */
#define XZYNQ_CAN_ESR_ACKER_MASK	0x00000010 /**< ACK Error */
#define XZYNQ_CAN_ESR_BERR_MASK		0x00000008 /**< Bit Error */
#define XZYNQ_CAN_ESR_STER_MASK		0x00000004 /**< Stuff Error */
#define XZYNQ_CAN_ESR_FMER_MASK		0x00000002 /**< Form Error */
#define XZYNQ_CAN_ESR_CRCER_MASK	0x00000001 /**< CRC Error */

/* Status Register (SR) Bit Definitions and Masks */
#define XZYNQ_CAN_SR_SNOOP_MASK		0x00001000 /**< Snoop Mask */
#define XZYNQ_CAN_SR_ACFBSY_MASK	0x00000800 /**< Acceptance Filter busy */
#define XZYNQ_CAN_SR_TXFLL_MASK		0x00000400 /**< TX FIFO is full */
#define XZYNQ_CAN_SR_TXBFLL_MASK	0x00000200 /**< TX High Priority Buffer full */
#define XZYNQ_CAN_SR_ESTAT_MASK		0x00000180 /**< Error Status */
#define XZYNQ_CAN_SR_ESTAT_SHIFT	7
#define XZYNQ_CAN_SR_ERRWRN_MASK	0x00000040 /**< Error Warning */
#define XZYNQ_CAN_SR_BBSY_MASK		0x00000020 /**< Bus Busy */
#define XZYNQ_CAN_SR_BIDLE_MASK		0x00000010 /**< Bus Idle */
#define XZYNQ_CAN_SR_NORMAL_MASK	0x00000008 /**< Normal Mode */
#define XZYNQ_CAN_SR_SLEEP_MASK		0x00000004 /**< Sleep Mode */
#define XZYNQ_CAN_SR_LBACK_MASK		0x00000002 /**< Loop Back Mode */
#define XZYNQ_CAN_SR_CONFIG_MASK	0x00000001 /**< Configuration Mode */

/* Interrupt Status/Enable/Clear Register Bit Definitions and Masks */
#define XZYNQ_CAN_IXR_TXFEMP_MASK   0x00004000 /**< Tx Fifo Empty Interrupt */
#define XZYNQ_CAN_IXR_TXFWMEMP_MASK 0x00002000 /**< Tx Fifo Watermark Empty */
#define XZYNQ_CAN_IXR_RXFWMFLL_MASK 0x00001000 /**< Rx FIFO Watermark Full */
#define XZYNQ_CAN_IXR_WKUP_MASK		0x00000800 /**< Wake up Interrupt */
#define XZYNQ_CAN_IXR_SLP_MASK		0x00000400 /**< Sleep Interrupt */
#define XZYNQ_CAN_IXR_BSOFF_MASK	0x00000200 /**< Bus Off Interrupt */
#define XZYNQ_CAN_IXR_ERROR_MASK	0x00000100 /**< Error Interrupt */
#define XZYNQ_CAN_IXR_RXNEMP_MASK	0x00000080 /**< RX FIFO Not Empty Interrupt */
#define XZYNQ_CAN_IXR_RXOFLW_MASK	0x00000040 /**< RX FIFO Overflow Interrupt */
#define XZYNQ_CAN_IXR_RXUFLW_MASK	0x00000020 /**< RX FIFO Underflow Interrupt */
#define XZYNQ_CAN_IXR_RXOK_MASK		0x00000010 /**< New Message Received Intr */
#define XZYNQ_CAN_IXR_TXBFLL_MASK	0x00000008 /**< TX High Priority Buf Full */
#define XZYNQ_CAN_IXR_TXFLL_MASK	0x00000004 /**< TX FIFO Full Interrupt */
#define XZYNQ_CAN_IXR_TXOK_MASK		0x00000002 /**< TX Successful Interrupt */
#define XZYNQ_CAN_IXR_ARBLST_MASK	0x00000001 /**< Arbitration Lost Interrupt */
#define XZYNQ_CAN_IXR_RX_ALL (XZYNQ_CAN_IXR_RXFWMFLL_MASK | \
				XZYNQ_CAN_IXR_RXNEMP_MASK | \
	 			XZYNQ_CAN_IXR_RXOK_MASK)
#define XZYNQ_CAN_IXR_TX_ALL (XZYNQ_CAN_IXR_TXFEMP_MASK | \
				XZYNQ_CAN_IXR_TXFWMEMP_MASK | \
				XZYNQ_CAN_IXR_TXOK_MASK)
#define XZYNQ_CAN_IXR_EVT_ALL (XZYNQ_CAN_IXR_RXOFLW_MASK | \
				XZYNQ_CAN_IXR_RXUFLW_MASK | \
				XZYNQ_CAN_IXR_TXBFLL_MASK | \
				XZYNQ_CAN_IXR_TXFLL_MASK  | \
				XZYNQ_CAN_IXR_WKUP_MASK   | \
				XZYNQ_CAN_IXR_SLP_MASK    | \
				XZYNQ_CAN_IXR_BSOFF_MASK  | \
				XZYNQ_CAN_IXR_ARBLST_MASK)
#define XZYNQ_CAN_IXR_DEFAULT (XZYNQ_CAN_IXR_RXOK_MASK | \
				XZYNQ_CAN_IXR_TXOK_MASK | \
				XZYNQ_CAN_IXR_EVT_ALL | \
				XZYNQ_CAN_IXR_ERROR_MASK)
#define XZYNQ_CAN_IXR_ALL (XZYNQ_CAN_IXR_DEFAULT | \
				XZYNQ_CAN_IXR_TX_ALL)

/* CAN Timestamp Control Register (TCR) Bit Definitions and Masks */
#define XZYNQ_CAN_TCR_CTS_MASK	0x00000001 /**< Clear Timestamp counter mask */

/* CAN Watermark Register (WIR) Bit Definitions and Masks */
#define XZYNQ_CAN_WIR_FW_MASK   0x0000003F /**< Rx Full Threshold mask */
#define XZYNQ_CAN_WIR_EW_MASK 	0x00003F00 /**< Tx Empty Threshold mask */
#define XZYNQ_CAN_WIR_EW_SHIFT 	0x00000008 /**< Tx Empty Threshold shift */

/*
 * CAN Frame Identifier (TX High Priority Buffer/TX/RX/Acceptance Filter
 * Mask/Acceptance Filter id)
 */
#define XZYNQ_CAN_IDR_ID1_MASK	0xFFE00000 /**< Standard Messg Identifier */
#define XZYNQ_CAN_IDR_ID1_SHIFT	21
#define XZYNQ_CAN_IDR_SRR_MASK	0x00100000 /**< Substitute Remote TX Req */
#define XZYNQ_CAN_IDR_SRR_SHIFT	20
#define XZYNQ_CAN_IDR_IDE_MASK	0x00080000 /**< Identifier Extension */
#define XZYNQ_CAN_IDR_IDE_SHIFT	19
#define XZYNQ_CAN_IDR_ID2_MASK	0x0007FFFE /**< Extended Message Ident */
#define XZYNQ_CAN_IDR_ID2_SHIFT	1
#define XZYNQ_CAN_IDR_RTR_MASK	0x00000001 /**< Remote TX Request */

#define XZYNQ_CAN_IDR_STD_MASK		0x000007FF
#define XZYNQ_CAN_IDR_EXT11_MASK	0x1FFC0000
#define XZYNQ_CAN_IDR_EXT11_SHIFT	18
#define XZYNQ_CAN_IDR_EXT18_MASK	0x0003FFFF

/* CAN Frame Data Length Code (TX High Priority Buffer/TX/RX) */
#define XZYNQ_CAN_DLCR_DLC_MASK	 		0xF0000000	/**< Data Length Code */
#define XZYNQ_CAN_DLCR_DLC_SHIFT		28
#define XZYNQ_CAN_DLCR_TIMESTAMP_MASK	0x0000FFFF	/**< Timestamp Mask (Rx only) */

/* Definitions for CAN Messages DLC */
#define XZYNQ_CAN_DLC_BYTE0		0x0
#define XZYNQ_CAN_DLC_BYTE1		0x1
#define XZYNQ_CAN_DLC_BYTE2		0x2
#define XZYNQ_CAN_DLC_BYTE3		0x3
#define XZYNQ_CAN_DLC_BYTE4		0x4
#define XZYNQ_CAN_DLC_BYTE5		0x5
#define XZYNQ_CAN_DLC_BYTE6		0x6
#define XZYNQ_CAN_DLC_BYTE7		0x7
#define XZYNQ_CAN_DLC_BYTE8		0x8

/* CAN Frame Data Word 1 (TX High Priority Buffer/TX/RX) */
#define XZYNQ_CAN_DW1R_DB0_MASK		0xFF000000 /**< Data Byte 0 */
#define XZYNQ_CAN_DW1R_DB0_SHIFT	24
#define XZYNQ_CAN_DW1R_DB1_MASK		0x00FF0000 /**< Data Byte 1 */
#define XZYNQ_CAN_DW1R_DB1_SHIFT	16
#define XZYNQ_CAN_DW1R_DB2_MASK		0x0000FF00 /**< Data Byte 2 */
#define XZYNQ_CAN_DW1R_DB2_SHIFT	8
#define XZYNQ_CAN_DW1R_DB3_MASK		0x000000FF /**< Data Byte 3 */

/* CAN Frame Data Word 2 (TX High Priority Buffer/TX/RX) */
#define XZYNQ_CAN_DW2R_DB4_MASK		0xFF000000 /**< Data Byte 4 */
#define XZYNQ_CAN_DW2R_DB4_SHIFT	24
#define XZYNQ_CAN_DW2R_DB5_MASK		0x00FF0000 /**< Data Byte 5 */
#define XZYNQ_CAN_DW2R_DB5_SHIFT	16
#define XZYNQ_CAN_DW2R_DB6_MASK		0x0000FF00 /**< Data Byte 6 */
#define XZYNQ_CAN_DW2R_DB6_SHIFT	8
#define XZYNQ_CAN_DW2R_DB7_MASK		0x000000FF /**< Data Byte 7 */

/* Acceptance Filter Register (AFR) Bit Definitions and Masks */
#define XZYNQ_CAN_AFR_UAF4_MASK		0x00000008 /**< Use Acceptance Filter No.4 */
#define XZYNQ_CAN_AFR_UAF3_MASK		0x00000004 /**< Use Acceptance Filter No.3 */
#define XZYNQ_CAN_AFR_UAF2_MASK		0x00000002 /**< Use Acceptance Filter No.2 */
#define XZYNQ_CAN_AFR_UAF1_MASK		0x00000001 /**< Use Acceptance Filter No.1 */
#define XZYNQ_CAN_AFR_UAF_ALL_MASK	(XZYNQ_CAN_AFR_UAF4_MASK | \
					XZYNQ_CAN_AFR_UAF3_MASK | \
					XZYNQ_CAN_AFR_UAF2_MASK | \
					XZYNQ_CAN_AFR_UAF1_MASK)

/* CAN frame length constants */
#define XZYNQ_CAN_MAX_FRAME_SIZE 	16 /**< Maximum CAN frame length in bytes */

/* For backwards compatibilty */
#define XZYNQ_CAN_TXBUF_ID_OFFSET   XZYNQ_CAN_TXHPB_ID_OFFSET
#define XZYNQ_CAN_TXBUF_DLC_OFFSET  XZYNQ_CAN_TXHPB_DLC_OFFSET
#define XZYNQ_CAN_TXBUF_DW1_OFFSET  XZYNQ_CAN_TXHPB_DW1_OFFSET
#define XZYNQ_CAN_TXBUF_DW2_OFFSET  XZYNQ_CAN_TXHPB_DW2_OFFSET

#define XZYNQ_CAN_RXFWIR_RXFLL_MASK XZYNQ_CAN_WIR_FW_MASK
#define XZYNQ_CAN_RXWIR_OFFSET 	 	XZYNQ_CAN_WIR_OFFSET
#define XZYNQ_CAN_IXR_RXFLL_MASK	XZYNQ_CAN_IXR_RXFWMFLL_MASK

#define CANDEV_XZYNQ_TX_ENABLED		0x00000001

#define INIT_FLAGS_LOOPBACK			0x00000001  /* Enable self-test/loopback mode */
#define INIT_FLAGS_EXTENDED_MID		0x00000002  /* Enable 29 bit extended message ID */
#define INIT_FLAGS_MSGDATA_LSB		0x00000004  /* Transmit message data LSB */
#define INIT_FLAGS_AUTOBUS			0x00000008  /* Enable auto bus on */
#define INIT_FLAGS_LNTC				0x00000010  /* Enable Local Network Time Clear Bit on mailbox 16 */
#define INIT_FLAGS_TIMESTAMP		0x00000020  /* Set initial local network time */
#define INIT_FLAGS_RX_FULL_MSG		0x00000040  /* Receiver should store message ID, timestamp, etc. */
#define INIT_FLAGS_BITRATE_ERM		0x00000080  /* Enable Bitrate Edge Resynchronization on rising and falling edges */
#define INIT_FLAGS_BITRATE_SAM		0x00000100  /* Enable Bitrate Triple Sample Mode */
#define INIT_FLAGS_MDRIVER_INIT		0x00000200  /* Initialize from mini-driver (if it exists and is running) */
#define INIT_FLAGS_MDRIVER_SORT		0x00000400  /* Sort buffered mdriver message based on MID */
#define INIT_FLAGS_SNOOP			0x00000800  /* Enable snooping mode */

#define INFO_FLAGS_RX_FULL_MSG      0x00000001  /* Receiver should store message id, timestamp, etc. */
#define INFO_FLAGS_ENDIAN_SWAP      0x00000002  /* Data is TX/RX'd MSB, need to perform ENDIAN conversions */

struct candev_xzynq_entry;

/* CAN Message Object Data Structure */
typedef struct can_msg_obj {
	uint32_t id;
	uint8_t srr;
	uint8_t ide;
	uint8_t rtr;
	uint8_t dlc;
	uint16_t timestamp;
	uint32_t canmdl;
	uint32_t canmdh;
	uint32_t mask;
	uint32_t prio;
} CAN_MSG_OBJ;

/* Mini-driver Data in memory - this must exactly match the definition used by the mdriver in Startup */
typedef struct minican_data {
	uintptr_t canport; /* CAN base register mapping */
	uintptr_t canport_k; /* CAN base register mapping */
	CAN_MSG_OBJ *canmem; /* CAN base memory mapping */
	CAN_MSG_OBJ *canmem_k; /* CAN base memory mapping */
	uint16_t nstartup; /* Stats for number of calls to MDRIVER_STARTUP */
	uint16_t nstartupp; /* Stats for number of calls to MDRIVER_STARTUP_PREPARE */
	uint16_t nstartupf; /* Stats for number of calls to MDRIVER_STARTUP_FINI */
	uint16_t nkernel; /* Stats for number of calls to MDRIVER_KERNEL */
	uint16_t nprocess; /* Stats for number of calls to MDRIVER_PROCESS */
	uint16_t nrx; /* Number of received messages */
	uint32_t tx_enabled; /* Flags to keep track of which mailboxes have a tx in progress */
/* Buffered messages of type CAN_MSG_OBJ follow this data structure in memory */
} MINICAN_DATA;

/* Initialization and Options Structure */
typedef struct candev_xzynq_init_entry {
	CANDEV_INIT	cinit;		/* Generic CAN Device Init Params */
	_Paddr32t	port;		/* Device Physical Register Address */
	uint32_t	clk;		/* CAN Clock */
	/* Bitrate related parameters */
	uint32_t	bitrate;	/* Bitrate */
	uint8_t		br_brp;		/* Bitrate Prescaler */
	uint8_t		br_sjw;		/* Bitrate Synch Jump Width */
	uint8_t		br_tseg1; 	/* Bitrate Time Segment 1 */
	uint8_t		br_tseg2; 	/* Bitrate Time Segment 2 */
	int			irqsys; 	/* Device Message System Vector */
	uint32_t	flags;		/* Initialization flags */
	uint32_t	numrx;		/* Number of RX Mailboxes (TX = Total Mailboxes - RX) */
	uint32_t	midrx;		/* RX Message id */
	uint32_t	midtx;		/* TX Message id */
	uint32_t	timestamp;	/* Initial value for local network time */
	uint32_t	rst_pin;	/* PHY reset pin (if existing) */
} CANDEV_XZYNQ_INIT;

/* General Device Information Structure */
typedef struct candev_xzynq_info_entry {
	uintptr_t base; /* Device Virtual Register Mapping */
	CAN_MSG_OBJ *canmsg; /* Array of CAN message objects */
	volatile uint32_t errstatus; /* Keep track of ESR/ISR register status for devctl */
	struct candev_xzynq_entry *devlist; /* Array of all device mailboxes */
	int iidsys; /* Return iid from InterruptAttach */
	uint32_t numtx; /* Number of TX Mailboxes */
	uint32_t numrx; /* Number of RX Mailboxes */
	uint32_t iflags; /* Info flags */
	MINICAN_DATA *mdata; /* Mini-driver data */
	CAN_MSG_OBJ *mcanmsg; /* Mini-driver buffered CAN messages */
} CANDEV_XZYNQ_INFO;

/* Device specific extension of CANDEV struct */
typedef struct candev_xzynq_entry {
	CANDEV cdev; /* CAN Device - MUST be first entry */
	int mbxid; /* Index into mailbox memory */
	volatile uint32_t dflags; /* Device specific flags */
	CANDEV_XZYNQ_INFO *devinfo; /* Common device information */
} CANDEV_XZYNQ;

#ifdef CONFIG_PMM
#include <hw/pmm.h>
#include <hw/clk.h>

extern pmm_functions_t funcs;
#endif

/* Driver implemented CAN library function prototypes */
void can_drvr_transmit(CANDEV *cdev);
int can_drvr_devctl(CANDEV *cdev, int dcmd, DCMD_DATA *data);

/* Function prototypes */
void can_init_mailbox(CANDEV_XZYNQ_INFO *devinfo, CANDEV_XZYNQ_INIT *devinit);
void can_init_hw(CANDEV_XZYNQ_INFO *devinfo, CANDEV_XZYNQ_INIT *devinit);
void can_init_intr(CANDEV_XZYNQ_INFO *devinfo, CANDEV_XZYNQ_INIT *devinit);
void can_init_clocks(CANDEV_XZYNQ_INIT *devinit);
void can_print_reg(CANDEV_XZYNQ_INFO *devinfo);
void can_print_mbox(CANDEV_XZYNQ_INFO *devinfo);
void can_debug(CANDEV_XZYNQ_INFO *devinfo);

#ifdef CONFIG_PMM
void xzynq_pm_standby(void* arg);
void xzynq_pm_resume(void* arg);
#endif

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/can/xzynq/canxzynq.h $ $Rev: 752035 $")
#endif
