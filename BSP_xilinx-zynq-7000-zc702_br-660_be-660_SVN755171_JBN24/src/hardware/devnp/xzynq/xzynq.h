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

#ifndef NIC_XZYNQ_H_INCLUDED
#define NIC_XZYNQ_H_INCLUDED

#include <io-pkt/iopkt_driver.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/cache.h>
#include <sys/param.h>
#include <sys/syspage.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/io-pkt.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/slogcodes.h>

#include <netdrvr/mdi.h>
#include <netdrvr/eth.h>
#include <netdrvr/nicsupport.h>
#include <netdrvr/common.h>

#include <hw/nicinfo.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_types.h>

#include <quiesce.h>
#include <siglock.h>

#include <sys/device.h>
#include <net/if_media.h>
#include <dev/mii/miivar.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>

#include <atomic.h>
#include <semaphore.h>

// #include <arm/xzynq.h>

/* -------------------------------------------------------------------------
 * System level control registers (SLCR)
 * -------------------------------------------------------------------------
 */

#define XZYNQ_SLCR_BASE					0xF8000000
#define XZYNQ_SLCR_LEN					0x00001000
#define XZYNQ_SLCR_LOCK_OFFSET			0x00000004
#define XZYNQ_SLCR_LOCK_KEY				0x0000767B
#define XZYNQ_SLCR_UNLOCK_OFFSET		0x00000008
#define XZYNQ_SLCR_UNLOCK_KEY			0x0000DF0D
#define XZYNQ_SLCR_ARM_PLL_CTRL_REG		0x00000100
#define XZYNQ_SLCR_DDR_PLL_CTRL_REG		0x00000104
#define XZYNQ_SLCR_IO_PLL_CTRL_REG		0x00000108
#define XZYNQ_SLCR_ARM_CLK_CTRL_REG		0x00000120
#define XZYNQ_SLCR_APER_CLK_CTRL_REG	0x0000012C
#define XZYNQ_SLCR_GEM0_RCLK_CTRL_REG	0x00000138
#define XZYNQ_SLCR_GEM0_CLK_CTRL_REG	0x00000140
#define XZYNQ_SLCR_SPI_CLK_CTRL_REG		0x00000158
#define XZYNQ_SLCR_CAN_CLK_CTRL_REG		0x0000015C
#define XZYNQ_SLCR_CAN_MIOCLK_CTRL_REG	0x00000160
#define XZYNQ_SLCR_CLK_621_TRUE_REG		0x000001C4
#define XZYNQ_SLCR_CAN_RST_CTRL_REG		0x00000220
#define XZYNQ_SLCR_FPGA_RST_CTRL_REG		0x00000240
#define XZYNQ_SLCR_LVL_SHFTR_EN_REG		0x00000900
#define XZYNQ_SLCR_OCM_CFG_REG				0x00000910

#define XZYNQ_IRQ_GEM0				54

/* -------------------------------------
 * GPIO
 * -------------------------------------
 */

#define XZYNQ_GPIO_BASE		0xE000A000
#define XZYNQ_GPIO_SIZE		0x300

/** @name Register offsets for the GPIO. Each register is 32 bits.
 *  @{
 */
#define XZYNQ_GPIOPS_DATA_LSW_OFFSET  0x000  /* Mask and Data Register LSW, WO */
#define XZYNQ_GPIOPS_DATA_MSW_OFFSET  0x004  /* Mask and Data Register MSW, WO */
#define XZYNQ_GPIOPS_DATA_OFFSET	 0x040  /* Data Register, RW */
#define XZYNQ_GPIOPS_DIRM_OFFSET	 0x204  /* Direction Mode Register, RW */
#define XZYNQ_GPIOPS_OUTEN_OFFSET	 0x208  /* Output Enable Register, RW */
#define XZYNQ_GPIOPS_INTMASK_OFFSET	 0x20C  /* Interrupt Mask Register, RO */
#define XZYNQ_GPIOPS_INTEN_OFFSET	 0x210  /* Interrupt Enable Register, WO */
#define XZYNQ_GPIOPS_INTDIS_OFFSET	 0x214  /* Interrupt Disable Register, WO*/
#define XZYNQ_GPIOPS_INTSTS_OFFSET	 0x218  /* Interrupt Status Register, RO */
#define XZYNQ_GPIOPS_INTTYPE_OFFSET	 0x21C  /* Interrupt Type Register, RW */
#define XZYNQ_GPIOPS_INTPOL_OFFSET	 0x220  /* Interrupt Polarity Register, RW */
#define XZYNQ_GPIOPS_INTANY_OFFSET	 0x224  /* Interrupt On Any Register, RW */
/* @} */

/* -------------------------------------
 * EMAC
 * -------------------------------------
 */

#define XZYNQ_EMAC0_BASE		0xE000B000
#define XZYNQ_EMAC1_BASE		0xE000C000
#define XZYNQ_EMAC_SIZE			0x1000

#include "bd.h"
#include "bdring.h"

/* Erros constants */
#define SUCCESS 0
#define INVALID_PARAM 1
#define DMA_SG_LIST_ERROR 2
#define DMA_SG_NO_LIST 3
#define FAILURE 4
#define EMAC_MII_BUSY 5

/** @name Direction identifiers
 *
 *  These are used by several functions and callbacks that need
 *  to specify whether an operation specifies a send or receive channel.
 * @{
 */
#define XZYNQ_EMACPS_SEND        1	      /**< send direction */
#define XZYNQ_EMACPS_RECV        2	      /**< receive direction */
/*@}*/

#define XZYNQ_EMACPS_RX_BUF_SIZE 1536 /**< Specify the receive buffer size in
                                       bytes, 64, 128, ... 10240 */
#define XZYNQ_EMACPS_NUM_MBUF_RX 128   /**< Number of receive buffer bytes as a
                                       unit, this is HW setup */
#define XZYNQ_EMACPS_NUM_MBUF_TX 128   /**< Number of send buffer bytes as a
                                       unit, this is HW setup */
#define XZYNQ_EMAPS_DEFAULT_PRIO 21
#define XZYNQ_EMAPS_MAX_FRAG     1

/* Register offset definitions. Unless otherwise noted, register access is
 * 32 bit. Names are self explained here.
 */

#define XZYNQ_EMACPS_NWCTRL_OFFSET        0x00000000 /**< Network Control reg */
#define XZYNQ_EMACPS_NWCFG_OFFSET         0x00000004 /**< Network Config reg */
#define XZYNQ_EMACPS_NWSR_OFFSET          0x00000008 /**< Network Status reg */

#define XZYNQ_EMACPS_DMACR_OFFSET         0x00000010 /**< DMA Control reg */
#define XZYNQ_EMACPS_TXSR_OFFSET          0x00000014 /**< TX Status reg */
#define XZYNQ_EMACPS_RXQBASE_OFFSET       0x00000018 /**< RX Q Base address reg */
#define XZYNQ_EMACPS_TXQBASE_OFFSET       0x0000001C /**< TX Q Base address reg */
#define XZYNQ_EMACPS_RXSR_OFFSET          0x00000020 /**< RX Status reg */

#define XZYNQ_EMACPS_ISR_OFFSET           0x00000024 /**< Interrupt Status reg */
#define XZYNQ_EMACPS_IER_OFFSET           0x00000028 /**< Interrupt Enable reg */
#define XZYNQ_EMACPS_IDR_OFFSET           0x0000002C /**< Interrupt Disable reg */
#define XZYNQ_EMACPS_IMR_OFFSET           0x00000030 /**< Interrupt Mask reg */

#define XZYNQ_EMACPS_PHYMNTNC_OFFSET      0x00000034 /**< Phy Maintaince reg */
#define XZYNQ_EMACPS_RXPAUSE_OFFSET       0x00000038 /**< RX Pause Time reg */
#define XZYNQ_EMACPS_TXPAUSE_OFFSET       0x0000003C /**< TX Pause Time reg */

#define XZYNQ_EMACPS_HASHL_OFFSET         0x00000080 /**< Hash Low address reg */
#define XZYNQ_EMACPS_HASHH_OFFSET         0x00000084 /**< Hash High address reg */

#define XZYNQ_EMACPS_LADDR1L_OFFSET       0x00000088 /**< Specific1 addr low reg */
#define XZYNQ_EMACPS_LADDR1H_OFFSET       0x0000008C /**< Specific1 addr high reg */
#define XZYNQ_EMACPS_LADDR2L_OFFSET       0x00000090 /**< Specific2 addr low reg */
#define XZYNQ_EMACPS_LADDR2H_OFFSET       0x00000094 /**< Specific2 addr high reg */
#define XZYNQ_EMACPS_LADDR3L_OFFSET       0x00000098 /**< Specific3 addr low reg */
#define XZYNQ_EMACPS_LADDR3H_OFFSET       0x0000009C /**< Specific3 addr high reg */
#define XZYNQ_EMACPS_LADDR4L_OFFSET       0x000000A0 /**< Specific4 addr low reg */
#define XZYNQ_EMACPS_LADDR4H_OFFSET       0x000000A4 /**< Specific4 addr high reg */

#define XZYNQ_EMACPS_MATCH1_OFFSET        0x000000A8 /**< Type ID1 Match reg */
#define XZYNQ_EMACPS_MATCH2_OFFSET        0x000000AC /**< Type ID2 Match reg */
#define XZYNQ_EMACPS_MATCH3_OFFSET        0x000000B0 /**< Type ID3 Match reg */
#define XZYNQ_EMACPS_MATCH4_OFFSET        0x000000B4 /**< Type ID4 Match reg */

#define XZYNQ_EMACPS_STRETCH_OFFSET       0x000000BC /**< IPG Stretch reg */

#define XZYNQ_EMACPS_OCTTXL_OFFSET        0x00000100 /**< Octects transmitted Low
                                                      reg */
#define XZYNQ_EMACPS_OCTTXH_OFFSET        0x00000104 /**< Octects transmitted High
                                                      reg */

#define XZYNQ_EMACPS_TXCNT_OFFSET         0x00000108 /**< Error-free Frmaes
                                                      transmitted counter */
#define XZYNQ_EMACPS_TXBCCNT_OFFSET       0x0000010C /**< Error-free Broadcast
                                                      Frames counter*/
#define XZYNQ_EMACPS_TXMCCNT_OFFSET       0x00000110 /**< Error-free Multicast
                                                      Frame counter */
#define XZYNQ_EMACPS_TXPAUSECNT_OFFSET    0x00000114 /**< Pause Frames Transmitted
                                                      Counter */
#define XZYNQ_EMACPS_TX64CNT_OFFSET       0x00000118 /**< Error-free 64 byte Frames
                                                      Transmitted counter */
#define XZYNQ_EMACPS_TX65CNT_OFFSET       0x0000011C /**< Error-free 65-127 byte
                                                      Frames Transmitted
                                                      counter */
#define XZYNQ_EMACPS_TX128CNT_OFFSET      0x00000120 /**< Error-free 128-255 byte
                                                      Frames Transmitted
                                                      counter*/
#define XZYNQ_EMACPS_TX256CNT_OFFSET      0x00000124 /**< Error-free 256-511 byte
                                                      Frames transmitted
                                                      counter */
#define XZYNQ_EMACPS_TX512CNT_OFFSET      0x00000128 /**< Error-free 512-1023 byte
                                                      Frames transmitted
                                                      counter */
#define XZYNQ_EMACPS_TX1024CNT_OFFSET     0x0000012C /**< Error-free 1024-1518 byte
                                                      Frames transmitted
                                                      counter */
#define XZYNQ_EMACPS_TX1519CNT_OFFSET     0x00000130 /**< Error-free larger than
                                                      1519 byte Frames
                                                      transmitted counter */
#define XZYNQ_EMACPS_TXURUNCNT_OFFSET     0x00000134 /**< TX under run error
                                                      counter */

#define XZYNQ_EMACPS_SNGLCOLLCNT_OFFSET   0x00000138 /**< Single Collision Frame
                                                      Counter */
#define XZYNQ_EMACPS_MULTICOLLCNT_OFFSET  0x0000013C /**< Multiple Collision Frame
                                                      Counter */
#define XZYNQ_EMACPS_EXCESSCOLLCNT_OFFSET 0x00000140 /**< Excessive Collision Frame
                                                      Counter */
#define XZYNQ_EMACPS_LATECOLLCNT_OFFSET   0x00000144 /**< Late Collision Frame
                                                      Counter */
#define XZYNQ_EMACPS_TXDEFERCNT_OFFSET    0x00000148 /**< Deferred Transmission
                                                      Frame Counter */
#define XZYNQ_EMACPS_TXCSENSECNT_OFFSET   0x0000014C /**< Transmit Carrier Sense
                                                      Error Counter */

#define XZYNQ_EMACPS_OCTRXL_OFFSET        0x00000150 /**< Octects Received register
                                                      Low */
#define XZYNQ_EMACPS_OCTRXH_OFFSET        0x00000154 /**< Octects Received register
                                                      High */

#define XZYNQ_EMACPS_RXCNT_OFFSET         0x00000158 /**< Error-free Frames
                                                      Received Counter */
#define XZYNQ_EMACPS_RXBROADCNT_OFFSET    0x0000015C /**< Error-free Broadcast
                                                      Frames Received Counter */
#define XZYNQ_EMACPS_RXMULTICNT_OFFSET    0x00000160 /**< Error-free Multicast
                                                      Frames Received Counter */
#define XZYNQ_EMACPS_RXPAUSECNT_OFFSET    0x00000164 /**< Pause Frames
                                                      Received Counter */
#define XZYNQ_EMACPS_RX64CNT_OFFSET       0x00000168 /**< Error-free 64 byte Frames
                                                      Received Counter */
#define XZYNQ_EMACPS_RX65CNT_OFFSET       0x0000016C /**< Error-free 65-127 byte
                                                      Frames Received Counter */
#define XZYNQ_EMACPS_RX128CNT_OFFSET      0x00000170 /**< Error-free 128-255 byte
                                                      Frames Received Counter */
#define XZYNQ_EMACPS_RX256CNT_OFFSET      0x00000174 /**< Error-free 256-512 byte
                                                      Frames Received Counter */
#define XZYNQ_EMACPS_RX512CNT_OFFSET      0x00000178 /**< Error-free 512-1023 byte
                                                      Frames Received Counter */
#define XZYNQ_EMACPS_RX1024CNT_OFFSET     0x0000017C /**< Error-free 1024-1518 byte
                                                      Frames Received Counter */
#define XZYNQ_EMACPS_RX1519CNT_OFFSET     0x00000180 /**< Error-free 1519-max byte
                                                      Frames Received Counter */
#define XZYNQ_EMACPS_RXUNDRCNT_OFFSET     0x00000184 /**< Undersize Frames Received
                                                      Counter */
#define XZYNQ_EMACPS_RXOVRCNT_OFFSET      0x00000188 /**< Oversize Frames Received
                                                      Counter */
#define XZYNQ_EMACPS_RXJABCNT_OFFSET      0x0000018C /**< Jabbers Received
                                                      Counter */
#define XZYNQ_EMACPS_RXFCSCNT_OFFSET      0x00000190 /**< Frame Check Sequence
                                                      Error Counter */
#define XZYNQ_EMACPS_RXLENGTHCNT_OFFSET   0x00000194 /**< Length Field Error
                                                      Counter */
#define XZYNQ_EMACPS_RXSYMBCNT_OFFSET     0x00000198 /**< Symbol Error Counter */
#define XZYNQ_EMACPS_RXALIGNCNT_OFFSET    0x0000019C /**< Alignment Error Counter */
#define XZYNQ_EMACPS_RXRESERRCNT_OFFSET   0x000001A0 /**< Receive Resource Error
                                                      Counter */
#define XZYNQ_EMACPS_RXORCNT_OFFSET       0x000001A4 /**< Receive Overrun Counter */
#define XZYNQ_EMACPS_RXIPCCNT_OFFSET      0x000001A8 /**< IP header Checksum Error
                                                      Counter */
#define XZYNQ_EMACPS_RXTCPCCNT_OFFSET     0x000001AC /**< TCP Checksum Error
                                                      Counter */
#define XZYNQ_EMACPS_RXUDPCCNT_OFFSET     0x000001B0 /**< UDP Checksum Error
                                                      Counter */
#define XZYNQ_EMACPS_LAST_OFFSET          0x000001B4 /**< Last statistic counter
						      offset, for clearing */

#define XZYNQ_EMACPS_1588_SEC_OFFSET      0x000001D0 /**< 1588 second counter */
#define XZYNQ_EMACPS_1588_NANOSEC_OFFSET  0x000001D4 /**< 1588 nanosecond counter */
#define XZYNQ_EMACPS_1588_ADJ_OFFSET      0x000001D8 /**< 1588 nanosecond
						      adjustment counter */
#define XZYNQ_EMACPS_1588_INC_OFFSET      0x000001DC /**< 1588 nanosecond
						      increment counter */
#define XZYNQ_EMACPS_PTP_TXSEC_OFFSET     0x000001E0 /**< 1588 PTP transmit second
						      counter */
#define XZYNQ_EMACPS_PTP_TXNANOSEC_OFFSET 0x000001E4 /**< 1588 PTP transmit
						      nanosecond counter */
#define XZYNQ_EMACPS_PTP_RXSEC_OFFSET     0x000001E8 /**< 1588 PTP receive second
						      counter */
#define XZYNQ_EMACPS_PTP_RXNANOSEC_OFFSET 0x000001EC /**< 1588 PTP receive
						      nanosecond counter */
#define XZYNQ_EMACPS_PTPP_TXSEC_OFFSET    0x000001F0 /**< 1588 PTP peer transmit
						      second counter */
#define XZYNQ_EMACPS_PTPP_TXNANOSEC_OFFSET 0x000001F4 /**< 1588 PTP peer transmit
						      nanosecond counter */
#define XZYNQ_EMACPS_PTPP_RXSEC_OFFSET    0x000001F8 /**< 1588 PTP peer receive
						      second counter */
#define XZYNQ_EMACPS_PTPP_RXNANOSEC_OFFSET 0x000001FC /**< 1588 PTP peer receive
						      nanosecond counter */

/* Define some bit positions for registers. */

/** @name network control register bit definitions
 * @{
 */
#define XZYNQ_EMACPS_NWCTRL_ZEROPAUSETX_MASK 0x00000800 /**< Transmit zero quantum
                                                         pause frame */
#define XZYNQ_EMACPS_NWCTRL_PAUSETX_MASK     0x00000800 /**< Transmit pause frame */
#define XZYNQ_EMACPS_NWCTRL_HALTTX_MASK      0x00000400 /**< Halt transmission
                                                         after current frame */
#define XZYNQ_EMACPS_NWCTRL_STARTTX_MASK     0x00000200 /**< Start tx (tx_go) */

#define XZYNQ_EMACPS_NWCTRL_STATWEN_MASK     0x00000080 /**< Enable writing to
                                                         stat counters */
#define XZYNQ_EMACPS_NWCTRL_STATINC_MASK     0x00000040 /**< Increment statistic
                                                         registers */
#define XZYNQ_EMACPS_NWCTRL_STATCLR_MASK     0x00000020 /**< Clear statistic
                                                         registers */
#define XZYNQ_EMACPS_NWCTRL_MDEN_MASK        0x00000010 /**< Enable MDIO port */
#define XZYNQ_EMACPS_NWCTRL_TXEN_MASK        0x00000008 /**< Enable transmit */
#define XZYNQ_EMACPS_NWCTRL_RXEN_MASK        0x00000004 /**< Enable receive */
#define XZYNQ_EMACPS_NWCTRL_LOOPEN_MASK      0x00000002 /**< local loopback */
/*@}*/

/** @name network configuration register bit definitions
 * @{
 */
#define XZYNQ_EMACPS_NWCFG_BADPREAMBEN_MASK 0x20000000 /**< disable rejection of
                                                        non-standard preamble */
#define XZYNQ_EMACPS_NWCFG_IPDSTRETCH_MASK  0x10000000 /**< enable transmit IPG */
#define XZYNQ_EMACPS_NWCFG_FCSIGNORE_MASK   0x04000000 /**< disable rejection of
                                                        FCS error */
#define XZYNQ_EMACPS_NWCFG_HDRXEN_MASK      0x02000000 /**< RX half duplex */
#define XZYNQ_EMACPS_NWCFG_RXCHKSUMEN_MASK  0x01000000 /**< enable RX checksum
                                                        offload */
#define XZYNQ_EMACPS_NWCFG_PAUSECOPYDI_MASK 0x00800000 /**< Do not copy pause
                                                        Frames to memory */
#define XZYNQ_EMACPS_NWCFG_MDC_SHIFT_MASK   18	   /**< shift bits for MDC */
#define XZYNQ_EMACPS_NWCFG_MDCCLKDIV_MASK   0x001C0000 /**< MDC Mask PCLK divisor */
#define XZYNQ_EMACPS_NWCFG_FCSREM_MASK      0x00020000 /**< Discard FCS from
                                                        received frames */
#define XZYNQ_EMACPS_NWCFG_LENGTHERRDSCRD_MASK 0x00010000
/**< RX length error discard */
#define XZYNQ_EMACPS_NWCFG_RXOFFS_MASK      0x0000C000 /**< RX buffer offset */
#define XZYNQ_EMACPS_NWCFG_PAUSEEN_MASK     0x00002000 /**< Enable pause RX */
#define XZYNQ_EMACPS_NWCFG_RETRYTESTEN_MASK 0x00001000 /**< Retry test */
#define XZYNQ_EMACPS_NWCFG_EXTADDRMATCHEN_MASK 0x00000200
/**< External address match enable */
#define XZYNQ_EMACPS_NWCFG_1000_MASK        0x00000400 /**< 1000 Mbps */
#define XZYNQ_EMACPS_NWCFG_1536RXEN_MASK    0x00000100 /**< Enable 1536 byte
                                                        frames reception */
#define XZYNQ_EMACPS_NWCFG_UCASTHASHEN_MASK 0x00000080 /**< Receive unicast hash
                                                        frames */
#define XZYNQ_EMACPS_NWCFG_MCASTHASHEN_MASK 0x00000040 /**< Receive multicast hash
                                                        frames */
#define XZYNQ_EMACPS_NWCFG_BCASTDI_MASK     0x00000020 /**< Do not receive
                                                        broadcast frames */
#define XZYNQ_EMACPS_NWCFG_COPYALLEN_MASK   0x00000010 /**< Copy all frames */
#define XZYNQ_EMACPS_NWCFG_JUMBO_MASK       0x00000008 /**< Jumbo frames */
#define XZYNQ_EMACPS_NWCFG_NVLANDISC_MASK   0x00000004 /**< Receive only VLAN
                                                        frames */
#define XZYNQ_EMACPS_NWCFG_FDEN_MASK        0x00000002 /**< full duplex */
#define XZYNQ_EMACPS_NWCFG_100_MASK         0x00000001 /**< 100 Mbps */
/*@}*/

/** @name network status register bit definitaions
 * @{
 */
#define XZYNQ_EMACPS_NWSR_MDIOIDLE_MASK     0x00000004 /**< PHY management idle */
#define XZYNQ_EMACPS_NWSR_MDIO_MASK         0x00000002 /**< Status of mdio_in */
/*@}*/


/** @name MAC address register word 1 mask
 * @{
 */
#define XZYNQ_EMACPS_LADDR_MACH_MASK        0x0000FFFF /**< Address bits[47:32]
                                                      bit[31:0] are in BOTTOM */
/*@}*/


/** @name DMA control register bit definitions
 * @{
 */
#define XZYNQ_EMACPS_DMACR_RXBUF_MASK      0x00FF0000 /**< Mask bit for RX buffer
                                                       size */
#define XZYNQ_EMACPS_DMACR_RXBUF_SHIFT     16	  /**< Shift bit for RX buffer
                                                       size */
#define XZYNQ_EMACPS_DMACR_TCPCKSUM_MASK   0x00000800 /**< enable/disable TX
                                                       checksum offload */
#define XZYNQ_EMACPS_DMACR_TXSIZE_MASK     0x00000400 /**< TX buffer memory size */
#define XZYNQ_EMACPS_DMACR_RXSIZE_MASK     0x00000300 /**< RX buffer memory size */
#define XZYNQ_EMACPS_DMACR_ENDIAN_MASK     0x00000080 /**< endian configuration */
#define XZYNQ_EMACPS_DMACR_BLENGTH_MASK    0x0000001F /**< buffer burst length */
/*@}*/

/** @name transmit status register bit definitions
 * @{
 */
#define XZYNQ_EMACPS_TXSR_HRESPNOK_MASK    0x00000100 /**< Transmit hresp not OK */
#define XZYNQ_EMACPS_TXSR_URUN_MASK        0x00000040 /**< Transmit underrun */
#define XZYNQ_EMACPS_TXSR_TXCOMPL_MASK     0x00000020 /**< Transmit completed OK */
#define XZYNQ_EMACPS_TXSR_BUFEXH_MASK      0x00000010 /**< Transmit buffs exhausted
                                                       mid frame */
#define XZYNQ_EMACPS_TXSR_TXGO_MASK        0x00000008 /**< Status of go flag */
#define XZYNQ_EMACPS_TXSR_RXOVR_MASK       0x00000004 /**< Retry limit exceeded */
#define XZYNQ_EMACPS_TXSR_FRAMERX_MASK     0x00000002 /**< Collision tx frame */
#define XZYNQ_EMACPS_TXSR_USEDREAD_MASK    0x00000001 /**< TX buffer used bit set */

#define XZYNQ_EMACPS_TXSR_ERROR_MASK      (XZYNQ_EMACPS_TXSR_HRESPNOK_MASK | \
                                       XZYNQ_EMACPS_TXSR_URUN_MASK | \
                                       XZYNQ_EMACPS_TXSR_BUFEXH_MASK | \
                                       XZYNQ_EMACPS_TXSR_RXOVR_MASK | \
                                       XZYNQ_EMACPS_TXSR_FRAMERX_MASK | \
                                       XZYNQ_EMACPS_TXSR_USEDREAD_MASK)
/*@}*/

/**
 * @name receive status register bit definitions
 * @{
 */
#define XZYNQ_EMACPS_RXSR_HRESPNOK_MASK    0x00000008 /**< Receive hresp not OK */
#define XZYNQ_EMACPS_RXSR_RXOVR_MASK       0x00000004 /**< Receive overrun */
#define XZYNQ_EMACPS_RXSR_FRAMERX_MASK     0x00000002 /**< Frame received OK */
#define XZYNQ_EMACPS_RXSR_BUFFNA_MASK      0x00000001 /**< RX buffer used bit set */

#define XZYNQ_EMACPS_RXSR_ERROR_MASK      (XZYNQ_EMACPS_RXSR_HRESPNOK_MASK | \
                                       XZYNQ_EMACPS_RXSR_RXOVR_MASK | \
                                       XZYNQ_EMACPS_RXSR_BUFFNA_MASK)
/*@}*/

/**
 * @name interrupts bit definitions
 * Bits definitions are same in XZYNQ_EMACPS_ISR_OFFSET,
 * XZYNQ_EMACPS_IER_OFFSET, XZYNQ_EMACPS_IDR_OFFSET, and XZYNQ_EMACPS_IMR_OFFSET
 * @{
 */
#define XZYNQ_EMACPS_IXR_PTPPSTX_MASK    0x02000000 /**< PTP Psync transmitted */
#define XZYNQ_EMACPS_IXR_PTPPDRTX_MASK   0x01000000 /**< PTP Pdelay_req
						     transmitted */
#define XZYNQ_EMACPS_IXR_PTPSTX_MASK     0x00800000 /**< PTP Sync transmitted */
#define XZYNQ_EMACPS_IXR_PTPDRTX_MASK    0x00400000 /**< PTP Delay_req transmitted
						*/
#define XZYNQ_EMACPS_IXR_PTPPSRX_MASK    0x00200000 /**< PTP Psync received */
#define XZYNQ_EMACPS_IXR_PTPPDRRX_MASK   0x00100000 /**< PTP Pdelay_req received */
#define XZYNQ_EMACPS_IXR_PTPSRX_MASK     0x00080000 /**< PTP Sync received */
#define XZYNQ_EMACPS_IXR_PTPDRRX_MASK    0x00040000 /**< PTP Delay_req received */
#define XZYNQ_EMACPS_IXR_PAUSETX_MASK    0x00004000	/**< Pause frame transmitted */
#define XZYNQ_EMACPS_IXR_PAUSEZERO_MASK  0x00002000	/**< Pause time has reached
                                                     zero */
#define XZYNQ_EMACPS_IXR_PAUSENZERO_MASK 0x00001000	/**< Pause frame received */
#define XZYNQ_EMACPS_IXR_HRESPNOK_MASK   0x00000800	/**< hresp not ok */
#define XZYNQ_EMACPS_IXR_RXOVR_MASK      0x00000400	/**< Receive overrun occurred */
#define XZYNQ_EMACPS_IXR_TXCOMPL_MASK    0x00000080	/**< Frame transmitted ok */
#define XZYNQ_EMACPS_IXR_TXEXH_MASK      0x00000040	/**< Transmit err occurred or
                                                     no buffers*/
#define XZYNQ_EMACPS_IXR_RETRY_MASK      0x00000020	/**< Retry limit exceeded */
#define XZYNQ_EMACPS_IXR_URUN_MASK       0x00000010	/**< Transmit underrun */
#define XZYNQ_EMACPS_IXR_TXUSED_MASK     0x00000008	/**< Tx buffer used bit read */
#define XZYNQ_EMACPS_IXR_RXUSED_MASK     0x00000004	/**< Rx buffer used bit read */
#define XZYNQ_EMACPS_IXR_FRAMERX_MASK    0x00000002	/**< Frame received ok */
#define XZYNQ_EMACPS_IXR_MGMNT_MASK      0x00000001	/**< PHY management complete */
#define XZYNQ_EMACPS_IXR_ALL_MASK        0x00007FFF	/**< Everything! */

#define XZYNQ_EMACPS_IXR_TX_ERR_MASK    (XZYNQ_EMACPS_IXR_TXEXH_MASK |         \
                                     XZYNQ_EMACPS_IXR_RETRY_MASK |         \
                                     XZYNQ_EMACPS_IXR_URUN_MASK  |         \
                                     XZYNQ_EMACPS_IXR_TXUSED_MASK)


#define XZYNQ_EMACPS_IXR_RX_ERR_MASK    (XZYNQ_EMACPS_IXR_HRESPNOK_MASK |      \
                                     XZYNQ_EMACPS_IXR_RXUSED_MASK |        \
                                     XZYNQ_EMACPS_IXR_RXOVR_MASK)

/*@}*/

/** @name PHY Maintenance bit definitions
 * @{
 */
#define XZYNQ_EMACPS_PHYMNTNC_OP_MASK    0x40020000	/**< operation mask bits */
#define XZYNQ_EMACPS_PHYMNTNC_OP_R_MASK  0x20000000	/**< read operation */
#define XZYNQ_EMACPS_PHYMNTNC_OP_W_MASK  0x10000000	/**< write operation */
#define XZYNQ_EMACPS_PHYMNTNC_ADDR_MASK  0x0F800000	/**< Address bits */
#define XZYNQ_EMACPS_PHYMNTNC_REG_MASK   0x007C0000	/**< register bits */
#define XZYNQ_EMACPS_PHYMNTNC_DATA_MASK  0x00000FFF	/**< data bits */
#define XZYNQ_EMACPS_PHYMNTNC_PHYAD_SHIFT_MASK   23	/**< Shift bits for PHYAD */
#define XZYNQ_EMACPS_PHYMNTNC_PHREG_SHIFT_MASK   18	/**< Shift bits for PHREG */
/*@}*/

/* Transmit buffer descriptor status words offset
 * @{
 */
#define XZYNQ_EMACPS_BD_ADDR_OFFSET  0x00000000 /**< word 0/addr of BDs */
#define XZYNQ_EMACPS_BD_STAT_OFFSET  0x00000004 /**< word 1/status of BDs */
/*
 * @}
 */

/* Transmit buffer descriptor status words bit positions.
 * Transmit buffer descriptor consists of two 32-bit registers,
 * the first - word0 contains a 32-bit address pointing to the location of
 * the transmit data.
 * The following register - word1, consists of various information to control
 * the XEmacPs transmit process.  After transmit, this is updated with status
 * information, whether the frame was transmitted OK or why it had failed.
 * @{
 */
#define XZYNQ_EMACPS_TXBUF_USED_MASK  0x80000000 /**< Used bit. */
#define XZYNQ_EMACPS_TXBUF_WRAP_MASK  0x40000000 /**< Wrap bit, last descriptor */
#define XZYNQ_EMACPS_TXBUF_RETRY_MASK 0x20000000 /**< Retry limit exceeded */
#define XZYNQ_EMACPS_TXBUF_URUN_MASK  0x10000000 /**< Transmit underrun occurred */
#define XZYNQ_EMACPS_TXBUF_EXH_MASK   0x08000000 /**< Buffers exhausted */
#define XZYNQ_EMACPS_TXBUF_TCP_MASK   0x04000000 /**< Late collision. */
#define XZYNQ_EMACPS_TXBUF_NOCRC_MASK 0x00010000 /**< No CRC */
#define XZYNQ_EMACPS_TXBUF_LAST_MASK  0x00008000 /**< Last buffer */
#define XZYNQ_EMACPS_TXBUF_LEN_MASK   0x00003FFF /**< Mask for length field */
/*
 * @}
 */

/* Receive buffer descriptor status words bit positions.
 * Receive buffer descriptor consists of two 32-bit registers,
 * the first - word0 contains a 32-bit word aligned address pointing to the
 * address of the buffer. The lower two bits make up the wrap bit indicating
 * the last descriptor and the ownership bit to indicate it has been used by
 * the XEmacPs.
 * The following register - word1, contains status information regarding why
 * the frame was received (the filter match condition) as well as other
 * useful info.
 * @{
 */
#define XZYNQ_EMACPS_RXBUF_BCAST_MASK     0x80000000 /**< Broadcast frame */
#define XZYNQ_EMACPS_RXBUF_MULTIHASH_MASK 0x40000000 /**< Multicast hashed frame */
#define XZYNQ_EMACPS_RXBUF_UNIHASH_MASK   0x20000000 /**< Unicast hashed frame */
#define XZYNQ_EMACPS_RXBUF_EXH_MASK       0x08000000 /**< buffer exhausted */
#define XZYNQ_EMACPS_RXBUF_AMATCH_MASK    0x06000000 /**< Specific address
                                                      matched */
#define XZYNQ_EMACPS_RXBUF_IDFOUND_MASK   0x01000000 /**< Type ID matched */
#define XZYNQ_EMACPS_RXBUF_IDMATCH_MASK   0x00C00000 /**< ID matched mask */
#define XZYNQ_EMACPS_RXBUF_VLAN_MASK      0x00200000 /**< VLAN tagged */
#define XZYNQ_EMACPS_RXBUF_PRI_MASK       0x00100000 /**< Priority tagged */
#define XZYNQ_EMACPS_RXBUF_VPRI_MASK      0x000E0000 /**< Vlan priority */
#define XZYNQ_EMACPS_RXBUF_CFI_MASK       0x00010000 /**< CFI frame */
#define XZYNQ_EMACPS_RXBUF_EOF_MASK       0x00008000 /**< End of frame. */
#define XZYNQ_EMACPS_RXBUF_SOF_MASK       0x00004000 /**< Start of frame. */
#define XZYNQ_EMACPS_RXBUF_LEN_MASK       0x00003FFF /**< Mask for length field */

#define XZYNQ_EMACPS_RXBUF_WRAP_MASK      0x00000002 /**< Wrap bit, last BD */
#define XZYNQ_EMACPS_RXBUF_NEW_MASK       0x00000001 /**< Used bit.. */
#define XZYNQ_EMACPS_RXBUF_ADD_MASK       0xFFFFFFFC /**< Mask for address */
/*
 * @}
 */


/* Utility macros
 * @{
 */
#define xzynq_get_tx_ring(emac) ((emac)->meminfo.tx_bdring)
#define xzynq_get_rx_ring(emac) ((emac)->meminfo.rx_bdring)

#define xzynq_int_enable(emac, mask)						\
	xzynq_write_reg((emac)->regbase,						\
		       XZYNQ_EMACPS_IER_OFFSET,					\
		       (mask & XZYNQ_EMACPS_IXR_ALL_MASK));

#define xzynq_int_disable(emac, mask)					\
	xzynq_write_reg((emac)->regbase,						\
		       XZYNQ_EMACPS_IDR_OFFSET,					\
		       (mask & XZYNQ_EMACPS_IXR_ALL_MASK));

#define xzynq_transmit(emac)								\
	xzynq_write_reg(emac->regbase,						\
		       XZYNQ_EMACPS_NWCTRL_OFFSET,				\
		       (xzynq_read_reg(emac->regbase,			\
				      XZYNQ_EMACPS_NWCTRL_OFFSET) | XZYNQ_EMACPS_NWCTRL_STARTTX_MASK))

#define xzynq_read_reg(regbase, reg_offset)		  \
	in32((regbase) + (reg_offset))

#define xzynq_write_reg(regbase, reg_offset, data) \
	out32((regbase) + (reg_offset), (data))

#define xzynq_bd_to_index(ringptr, bdptr)						\
	(((uint32_t)bdptr - (uint32_t)(ringptr)->base_bd_addr) / (ringptr)->separation)
/*
 * @}
 */

typedef struct {
	/* To ensure cache coherence */
    struct cache_ctrl    cachectl;

    /*
     * Buffers that contains packets to send
     * The DMA will copy them
     */
    struct mbuf          **rx_mbuf;
    struct mbuf          **tx_mbuf;

	xzynq_bdring_t		 tx_bdring;          /* Transmit BD ring */
	xzynq_bdring_t		 rx_bdring;          /* Receive BD ring */

    xzynq_bd_t           *rx_bds;
    xzynq_bd_t           *tx_bds;
} xzynq_mem_t;

typedef struct {
    struct device              dev;            /* Common device */
    struct ethercom            ecom;           /* Common Ethernet */
    struct ethercom            *common_ecom[2];

    struct sigevent            intr_tx_event;
    int32_t                    tx_chid;
    int32_t                    tx_coid;
    sem_t                      send_tx_sem;

    int32_t                    iid;
    int	                       dying;
    int						   linkup;
    int						   force_link;
	int						   probe_phy;

    uint32_t                   num_if;
    uintptr_t				   regbase;
    uintptr_t                  gpio_regbase;

    void                       *sd_hook;
    nic_config_t               cfg;
    nic_stats_t                stats;
    struct callout             hk_callout;

    struct _iopkt_self         *iopkt;
    struct _iopkt_inter        inter;

    struct callout             mii_callout;
    struct mii_data            bsd_mii;

    mdi_t                      *mdi;

    const struct sigevent      *(*isrp)(void *, int);

    xzynq_mem_t				   meminfo;
} xzynq_dev_t;

/* bsd media */
void bsd_mii_initmedia(xzynq_dev_t *xzynq);

/* options */
int xzynq_parse_options(xzynq_dev_t *xzynq, char *optstring,
		nic_config_t *cfg);

/* control */
void xzynq_set_mac_address(xzynq_dev_t *xzynq, int index);
void xzynq_read_eeproom_mac_address(xzynq_dev_t *xzynq);

/* intr */
const struct sigevent *xzynq_isr(void *arg, int iid);
int xzynq_process_interrupt(void *arg, struct nw_work_thread *wtp);
int xzynq_enable_interrupt(void *arg);
const struct sigevent* xzynq_isr_kermask(void *arg, int iid);
int xzynq_enable_interrupt_kermask(void *arg);

/* ioctl */
int xzynq_ioctl(struct ifnet * ifp, unsigned long cmd, caddr_t data);

/* transmit */
void* xzynq_thread_tx(void* arg);
void xzynq_start(struct ifnet *ifp);

/* receive */
int xzynq_process_rx(xzynq_dev_t *xzynq, struct nw_work_thread *wtp);

/* MII */
void xzynq_mdi_callback(void *handle, uint8_t phyaddr, uint8_t linkstate);
int xzynq_mdi_find_phy(xzynq_dev_t *xzynq);
void xzynq_mdi_configure_phy(xzynq_dev_t *xzynq, int phyid);

/* xzynq */
paddr_t xzynq_vtophys(void *addr);
void xzynq_hw_reset(xzynq_dev_t *xzynq);
void xzynq_stop(struct ifnet *ifp, int disable);
void xzynq_shutdown(void *arg);
int xzynq_init(struct ifnet *ifp);
void xzynq_stop(struct ifnet *ifp, int disable);
int xzynq_detach(struct device *dev, int flags);
int xzynq_attach(struct device *parent, struct device *self, void *aux);
void xzynq_setup_rx_buffers(xzynq_dev_t *xzynq, int inval);
void xzynq_cleanup_descriptors(xzynq_dev_t *xzynq);
void xzynq_setup_descriptors(xzynq_dev_t *xzynq);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/xzynq.h $ $Rev: 752035 $")
#endif
