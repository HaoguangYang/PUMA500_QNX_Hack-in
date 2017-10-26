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

#include "xzynq.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef CONFIG_PMM
#include <hw/pmm.h>
#include <hw/clk.h>

pmm_functions_t funcs;

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_pm_standby(void* arg)
{
	int fd, err;
	ctrl_id_t id = XZYNQ_GEM0_CTRL;

	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("open");
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
			"devnp: %s can't open /dev/clock", __func__);
		return;
	}

	err = devctl(fd, DCMD_CLOCK_DISABLE, &id, sizeof(clk_id_t), NULL);
	if (err) {
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
			"devnp: %s can't disable the clock", __func__);
	}

	id = XZYNQ_GEM0_RCLK;
	err = devctl(fd, DCMD_CLOCK_DISABLE, &id, sizeof(clk_id_t), NULL);
	if (err) {
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
			"devnp: %s can't disable the clock", __func__);
	}
	close(fd);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void xzynq_pm_resume(void* arg)
{
	int fd, err;
	ctrl_id_t id = XZYNQ_GEM0_CTRL;

	fd = open("/dev/clock", O_RDWR);
	if (fd == -1) {
		perror("open");
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
			"devnp: %s can't open /dev/clock", __func__);
		return;
	}

	err = devctl(fd, DCMD_CLOCK_ENABLE, &id, sizeof(clk_id_t), NULL);
	if (err) {
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
			"devnp: %s can't enable the clock", __func__);
	}

	id = XZYNQ_GEM0_RCLK;
	err = devctl(fd, DCMD_CLOCK_ENABLE, &id, sizeof(clk_id_t), NULL);
	if (err) {
		slogf(_SLOGC_NETWORK, _SLOG_INFO,
			"devnp: %s can't enable the clock", __func__);
	}
	close(fd);
}

#endif

static int xzynq_entry(void *dll_hdl, struct _iopkt_self *iopkt, char *options);

struct _iopkt_drvr_entry IOPKT_DRVR_ENTRY_SYM( xzynq) =
		IOPKT_DRVR_ENTRY_SYM_INIT(xzynq_entry);
#ifdef VARIANT_a
#include <nw_dl.h>
/* This is what gets specified in the stack's dl.c */
struct nw_dll_syms xzynq_syms[] = { {"iopkt_drvr_entry",
		&IOPKT_DRVR_ENTRY_SYM(xzynq)}, {NULL, NULL}};
#endif
CFATTACH_DECL(xzynq,
		sizeof(xzynq_dev_t),
		NULL,
		xzynq_attach,
		xzynq_detach,
		NULL);

struct attach_args {
	char *options;
	struct _iopkt_self *iopkt;
	void *dll_hdl;
};

paddr_t xzynq_vtophys(void *addr)
{
	off64_t offset;

	if (mem_offset64(addr, NOFD, 1, &offset, 0) == -1) {
		return (-1);
	}
	return (offset);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
void xzynq_setup_rx_buffers(xzynq_dev_t *xzynq, int inval)
{
	struct nw_work_thread *wtp;
	struct mbuf *m;
	off64_t phys;
	int free_bds, i, j, rc;
	xzynq_bd_t *bd_ptr;
	wtp = WTP;

	j = 0;

	/* Get the numbers of free BD */
	free_bds = xzynq->meminfo.rx_bdring.free_cnt;

	/* Re-allocate a BD for each slot */
	for (i = 0; i < free_bds; i++) {
		rc = xzynq_bdring_alloc(&(xzynq_get_rx_ring(xzynq)), 1, &bd_ptr);
		if (rc) {
			slogf(_SLOGC_NETWORK, _SLOG_INFO,
				"devnp: %s bdring_alloc error", __func__);
			break;
		}

		j = xzynq_bd_to_index(&(xzynq_get_rx_ring(xzynq)), bd_ptr);
		m = m_getcl_wtp(M_DONTWAIT, MT_DATA, M_PKTHDR, wtp);
		xzynq->meminfo.rx_mbuf[j] = m;
		phys = mbuf_phys(m);

		/* this should be done in xzynq_bdring_to_hw */
		if (inval)
			CACHE_INVAL(&xzynq->meminfo.cachectl, m->m_data, phys, m->m_ext.ext_size);

		xzynq_bd_set_address_rx(bd_ptr, phys);

		rc = xzynq_bdring_to_hw(&(xzynq_get_rx_ring(xzynq)), 1, bd_ptr, XZYNQ_EMACPS_RECV);
		if (rc) {
			slogf(_SLOGC_NETWORK, _SLOG_INFO,
				"devnp: %s bdring_to_hw error", __func__);
			break;
		}
	}
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
void xzynq_cleanup_descriptors(xzynq_dev_t *xzynq)
{
	int i;

	/* Reset Buffers Descriptors */
	if (xzynq->meminfo.rx_bds)
		munmap(xzynq->meminfo.rx_bds, XZYNQ_EMACPS_NUM_MBUF_RX
				* sizeof(xzynq->meminfo.rx_bds));

	if (xzynq->meminfo.tx_bds)
		munmap(xzynq->meminfo.tx_bds, XZYNQ_EMACPS_NUM_MBUF_TX
				* sizeof(xzynq->meminfo.tx_bds));

	if (xzynq->meminfo.rx_mbuf) {
		for (i = 0; i < XZYNQ_EMACPS_NUM_MBUF_RX; i++) {
			if (xzynq->meminfo.rx_mbuf[i]) {
				m_free(xzynq->meminfo.rx_mbuf[i]);
				xzynq->meminfo.rx_mbuf[i] = NULL;
			}
		}

		free(xzynq->meminfo.rx_mbuf, M_DEVBUF);
		xzynq->meminfo.rx_mbuf = NULL;
	}

	if (xzynq->meminfo.tx_mbuf) {
		for (i = 0; i < XZYNQ_EMACPS_NUM_MBUF_TX; i++) {
			if (xzynq->meminfo.tx_mbuf[i]) {
				m_free(xzynq->meminfo.tx_mbuf[i]);
				xzynq->meminfo.tx_mbuf[i] = NULL;
			}
		}

		free(xzynq->meminfo.tx_mbuf, M_DEVBUF);
		xzynq->meminfo.tx_mbuf = NULL;
	}
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
void xzynq_setup_descriptors(xzynq_dev_t *xzynq)
{
	int size;
	xzynq_bd_t bd_template;

	xzynq->meminfo.cachectl.fd = NOFD;
	cache_init(0, &xzynq->meminfo.cachectl, NULL);

	/* Allocate the RX ring buffer */
	size = XZYNQ_EMACPS_NUM_MBUF_RX * sizeof(xzynq->meminfo.rx_bds);
	xzynq->meminfo.rx_bds = mmap(NULL, size, PROT_READ | PROT_WRITE
			| PROT_NOCACHE, /* MAP_PRIVATE |*/MAP_ANON | MAP_PHYS,
			NOFD, 0);

	xzynq_bd_clear(&bd_template);
	xzynq_bdring_create(&(xzynq_get_rx_ring(xzynq)),
			(uint32_t) xzynq->meminfo.rx_bds,
			(uint32_t) xzynq->meminfo.rx_bds,
			XZYNQ_EMACPS_BD_ALIGNMENT, XZYNQ_EMACPS_NUM_MBUF_RX);

	/* Clone our BD template, the last will be set with WRAP bit */
	xzynq_bdring_clone(&(xzynq_get_rx_ring(xzynq)), &bd_template,
			XZYNQ_EMACPS_RECV);

	/* Allocate array of mbuf pointers for receiving */
	size = XZYNQ_EMACPS_NUM_MBUF_RX * sizeof(xzynq->meminfo.rx_mbuf);
	xzynq->meminfo.rx_mbuf = malloc(size, M_DEVBUF, M_NOWAIT);
	memset(xzynq->meminfo.rx_mbuf, 0, size);

	/* Allocate RX buffers */
	xzynq_setup_rx_buffers(xzynq, 1); /* invalidate the cache */

	/* Allocate the TX ring buffer */
	size = XZYNQ_EMACPS_NUM_MBUF_TX * sizeof(xzynq->meminfo.tx_bds);
	xzynq->meminfo.tx_bds = mmap(NULL, size, PROT_READ | PROT_WRITE
			| PROT_NOCACHE, /* MAP_PRIVATE |*/MAP_ANON | MAP_PHYS,
			NOFD, 0);

	xzynq_bd_clear(&bd_template);
	xzynq_bd_set_status(&bd_template, XZYNQ_EMACPS_TXBUF_USED_MASK);
	xzynq_bdring_create(&(xzynq_get_tx_ring(xzynq)),
			(uint32_t) xzynq->meminfo.tx_bds,
			(uint32_t) xzynq->meminfo.tx_bds,
			XZYNQ_EMACPS_BD_ALIGNMENT, XZYNQ_EMACPS_NUM_MBUF_TX);

	/* Clone our BD template, the last will be set with WRAP bit */
	xzynq_bdring_clone(&(xzynq_get_tx_ring(xzynq)), &bd_template,
			XZYNQ_EMACPS_SEND);

	/* Allocate array of mbuf pointers for sending */
	size = XZYNQ_EMACPS_NUM_MBUF_TX * sizeof(xzynq->meminfo.tx_mbuf);
	xzynq->meminfo.tx_mbuf = malloc(size, M_DEVBUF, M_NOWAIT);
	memset(xzynq->meminfo.tx_mbuf, 0, size);

	/*
	 * Write the base address in the GEM controller
	 * for TX/RX DMA
	 */
	out32(xzynq->regbase + XZYNQ_EMACPS_RXQBASE_OFFSET, xzynq_vtophys(
			(void *) xzynq->meminfo.rx_bds));

	out32(xzynq->regbase + XZYNQ_EMACPS_TXQBASE_OFFSET, xzynq_vtophys(
			(void *) xzynq->meminfo.tx_bds));
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
int xzynq_attach(struct device *parent, struct device *self, void *aux)
{
	xzynq_dev_t *xzynq;
	struct ifnet *ifp;
	struct attach_args *attach_args;
	struct _iopkt_self *iopkt;
	char *options;
	int32_t err;
	int kernmask = 0; /* Need to fix a bug without kermask */

	attach_args = aux;
	iopkt = attach_args->iopkt;
	options = attach_args->options;
	xzynq = (xzynq_dev_t*) self;
	xzynq->iopkt = iopkt;
	ifp = &xzynq->ecom.ec_if;
	ifp->if_softc = xzynq;

	xzynq->meminfo.rx_bds = NULL;
	xzynq->meminfo.tx_bds = NULL;
	xzynq->meminfo.rx_mbuf = NULL;
	xzynq->meminfo.tx_mbuf = NULL;

	xzynq->gpio_regbase = mmap_device_io(XZYNQ_GPIO_SIZE, XZYNQ_GPIO_BASE);
	if (xzynq->gpio_regbase == MAP_DEVICE_FAILED) {
		slogf(_SLOGC_NETWORK, _SLOG_ERROR,
				"%s: Error mapping device at address %08x\n",
				ifp->if_xname, XZYNQ_GPIO_BASE);
		return EIO;
	}

	/* Setin interface name */
	strcpy(ifp->if_xname, xzynq->dev.dv_xname);
	strcpy((char *) xzynq->cfg.uptype, "en");
	strcpy((char *) xzynq->cfg.device_description, "xzynq");

	/* Configure Interrupts and memory mappings */
	xzynq->cfg.num_irqs = 1;
	xzynq->cfg.irq[0] = XZYNQ_IRQ_GEM0;
	xzynq->regbase = mmap_device_io(XZYNQ_EMAC_SIZE, XZYNQ_EMAC0_BASE);
	if (xzynq->regbase == MAP_DEVICE_FAILED) {
		slogf(_SLOGC_NETWORK, _SLOG_ERROR,
				"%s: Error mapping device at address %08x\n",
				ifp->if_xname, XZYNQ_EMAC0_BASE);
		return EIO;
	}

	/* Ethernet stats we are interested in */
	xzynq->stats.un.estats.valid_stats = NIC_ETHER_STAT_INTERNAL_TX_ERRORS
			| NIC_ETHER_STAT_INTERNAL_RX_ERRORS
			| NIC_ETHER_STAT_NO_CARRIER
			| NIC_ETHER_STAT_XCOLL_ABORTED
			| NIC_ETHER_STAT_SINGLE_COLLISIONS
			| NIC_ETHER_STAT_MULTI_COLLISIONS
			| NIC_ETHER_STAT_LATE_COLLISIONS
			| NIC_ETHER_STAT_TX_DEFERRED
			| NIC_ETHER_STAT_ALIGN_ERRORS
			| NIC_ETHER_STAT_FCS_ERRORS
			| NIC_ETHER_STAT_JABBER_DETECTED
			| NIC_ETHER_STAT_OVERSIZED_PACKETS
			| NIC_ETHER_STAT_SHORT_PACKETS;

	/* Generic networking stats we are interested in */
	xzynq->stats.valid_stats = NIC_STAT_TX_FAILED_ALLOCS
			| NIC_STAT_RX_FAILED_ALLOCS | NIC_STAT_RXED_MULTICAST
			| NIC_STAT_RXED_BROADCAST | NIC_STAT_TXED_BROADCAST
			| NIC_STAT_TXED_MULTICAST;

	xzynq->cfg.priority = XZYNQ_EMAPS_DEFAULT_PRIO; /* default priority */
	xzynq->cfg.lan = xzynq->dev.dv_unit;
	xzynq->cfg.media_rate = -1;
	xzynq->cfg.duplex = -1;
	xzynq->force_link = -1;

	xzynq->cfg.flags |= NIC_FLAG_LINK_DOWN;

	xzynq->cfg.num_mem_windows = 1;
	xzynq->cfg.mem_window_base[0] = XZYNQ_EMAC0_BASE;
	xzynq->cfg.mem_window_size[0] = XZYNQ_EMAC_SIZE;

	/* Setup interrupt entry */
	if ((err = interrupt_entry_init(&xzynq->inter, 0, NULL,
			XZYNQ_EMAPS_DEFAULT_PRIO)) != EOK) {
		slogf(_SLOGC_NETWORK, _SLOG_ERROR,
				"%s: can't init the RX interrupt\n",
				ifp->if_xname);
		return err;
	}

	/* Setup RX interrupt */
	xzynq->inter.func = xzynq_process_interrupt;
	xzynq->inter.arg = xzynq;
	if (kernmask) {
		xzynq->isrp = xzynq_isr_kermask;
		xzynq->inter.enable = xzynq_enable_interrupt_kermask;
	} else {
		xzynq->isrp = xzynq_isr;
		xzynq->inter.enable = xzynq_enable_interrupt;
	}

	/* Number of tx descriptors needs to be split up between interfaces (2)*/
	xzynq->cfg.mtu = ETH_MAX_PKT_LEN;
	xzynq->cfg.mru = ETH_MAX_PKT_LEN;
	xzynq->cfg.flags |= NIC_FLAG_MULTICAST;
	xzynq->cfg.mac_length = ETH_MAC_LEN;
	xzynq->num_if = 0;

	/* Set the MAC Address */
	xzynq_read_eeproom_mac_address(xzynq);
	xzynq_set_mac_address(xzynq, 0);
	memcpy(xzynq->cfg.permanent_address, xzynq->cfg.current_address, 6);

	/* Hook up so media devctls work */
	bsd_mii_initmedia(xzynq);

	/* Setup ethercomm */
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = xzynq_ioctl;
	ifp->if_start = xzynq_start;
	ifp->if_init = xzynq_init;
	ifp->if_stop = xzynq_stop;
	IFQ_SET_READY(&ifp->if_snd);

	xzynq->iid = -1; /* Not yet attached */

	/* Default options */
	xzynq->probe_phy = 1;

	/* Parse options */
	if (xzynq_parse_options(xzynq, options, &xzynq->cfg) == -1) {
		err = errno;
		slogf(_SLOGC_NETWORK, _SLOG_ERROR,
				"%s: error while parsing the options\n",
				ifp->if_xname);
		return err;
	}

	/* If media was not specified on cmdline, default to NIC_MEDIA_802_3 */
	if (xzynq->cfg.media == -1) {
		xzynq->cfg.media = NIC_MEDIA_802_3;
		xzynq->stats.media = xzynq->cfg.media;
	} else {
		xzynq->stats.media = xzynq->cfg.media;
	}

	if (xzynq->cfg.mtu == 0 || xzynq->cfg.mtu > ETH_MAX_PKT_LEN) {
		xzynq->cfg.mtu = ETH_MAX_PKT_LEN;
	}

	if (xzynq->cfg.mru == 0 || xzynq->cfg.mru > ETH_MAX_PKT_LEN) {
		xzynq->cfg.mru = ETH_MAX_PKT_LEN;
	}

	/* Attach the interrupt */
	if (xzynq->iid == -1) {
		if ((err = InterruptAttach_r(xzynq->cfg.irq[0], xzynq->isrp,
				xzynq, sizeof(*xzynq), _NTO_INTR_FLAGS_TRK_MSK))
				< 0) {
			slogf(
					_SLOGC_NETWORK, _SLOG_ERROR,
					"%s: can't attach to the interrupt %d\n",
					ifp->if_xname, xzynq->cfg.irq[0]);
			return err;
		}

		xzynq->iid = err;
	}

	if_attach(ifp);
	ether_ifattach(ifp, xzynq->cfg.current_address);

	/* Start House-keeping periodic callout */
	callout_init(&xzynq->hk_callout);

	xzynq->sd_hook = shutdownhook_establish(xzynq_shutdown, xzynq);
	slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s: Interface attached\n",
			ifp->if_xname);
	return (EOK);
}

static uint32_t gpio_mask_data(unsigned pin, unsigned data)
{
	/*
	 * Return the 32 bit value to be written to the Mask/Data register where
	 * the upper 16 bits is the mask and lower 16 bits is the data.
	 */
	uint32_t mask = 0xFFFF ^ (1 << pin);

	data = (data & 1) << pin;

	return (mask << 16) | (data & 0xFFFF);
}

/*****************************************************************************/
/* Resets all modules.                                                       */
/*****************************************************************************/
void xzynq_hw_reset(xzynq_dev_t *xzynq)
{
	uint32_t reg;

	/* Hardware Reset of the PHY via MIO11 (GPIO11) */
	out32(xzynq->gpio_regbase + XZYNQ_GPIOPS_DATA_LSW_OFFSET, gpio_mask_data(11, 0));
	usleep(10000);
	out32(xzynq->gpio_regbase + XZYNQ_GPIOPS_DATA_LSW_OFFSET, gpio_mask_data(11, 1));

	/* Setup hardware with default values */
	out32(xzynq->regbase + XZYNQ_EMACPS_NWCTRL_OFFSET,
			(XZYNQ_EMACPS_NWCTRL_STATCLR_MASK
					| XZYNQ_EMACPS_NWCTRL_MDEN_MASK)
					&
					~XZYNQ_EMACPS_NWCTRL_LOOPEN_MASK);

	out32(xzynq->regbase + XZYNQ_EMACPS_NWCFG_OFFSET,
			(0x7 << XZYNQ_EMACPS_NWCFG_MDC_SHIFT_MASK)
					| XZYNQ_EMACPS_NWCFG_100_MASK
					| XZYNQ_EMACPS_NWCFG_FDEN_MASK
					| XZYNQ_EMACPS_NWCFG_UCASTHASHEN_MASK);

	out32(
			xzynq->regbase + XZYNQ_EMACPS_DMACR_OFFSET,
			((((XZYNQ_EMACPS_RX_BUF_SIZE / XZYNQ_EMACPS_NUM_MBUF_RX)
					+ ((XZYNQ_EMACPS_RX_BUF_SIZE
							% XZYNQ_EMACPS_NUM_MBUF_RX) ? 1
							: 0))
					<< XZYNQ_EMACPS_DMACR_RXBUF_SHIFT)
					& XZYNQ_EMACPS_DMACR_RXBUF_MASK)
					| XZYNQ_EMACPS_DMACR_RXSIZE_MASK
					| XZYNQ_EMACPS_DMACR_TXSIZE_MASK);

	out32(xzynq->regbase + XZYNQ_EMACPS_TXSR_OFFSET, 0x0);
	out32(xzynq->regbase + XZYNQ_EMACPS_RXQBASE_OFFSET, 0x0);
	out32(xzynq->regbase + XZYNQ_EMACPS_TXQBASE_OFFSET, 0x0);
	out32(xzynq->regbase + XZYNQ_EMACPS_RXSR_OFFSET, 0x0);
	out32(xzynq->regbase + XZYNQ_EMACPS_IDR_OFFSET,
			XZYNQ_EMACPS_IXR_ALL_MASK);

	reg = in32(xzynq->regbase + XZYNQ_EMACPS_ISR_OFFSET);
	out32(xzynq->regbase + XZYNQ_EMACPS_ISR_OFFSET, reg);

	out32(xzynq->regbase + XZYNQ_EMACPS_PHYMNTNC_OFFSET, 0x0);

	/* Disable the receiver */
	reg = in32(xzynq->regbase + XZYNQ_EMACPS_NWCTRL_OFFSET);
	reg &= ~XZYNQ_EMACPS_NWCTRL_RXEN_MASK;
	out32(xzynq->regbase + XZYNQ_EMACPS_NWCTRL_OFFSET, reg);
}

/*****************************************************************************/
/* Shutdown all interfaces and disable host interface                        */
/*****************************************************************************/
void xzynq_stop(struct ifnet *ifp, int disable)
{
	uint32_t reg;
	xzynq_dev_t *xzynq = ifp->if_softc;

	/* Disable all interrupts */
	out32(xzynq->regbase + XZYNQ_EMACPS_IDR_OFFSET,
			XZYNQ_EMACPS_IXR_ALL_MASK);

	/* Disable the receiver & transmitter */
	reg = in32(xzynq->regbase + XZYNQ_EMACPS_NWCTRL_OFFSET);
	reg &= ~XZYNQ_EMACPS_NWCTRL_RXEN_MASK;
	reg &= ~XZYNQ_EMACPS_NWCTRL_TXEN_MASK;
	out32(xzynq->regbase + XZYNQ_EMACPS_NWCTRL_OFFSET, reg);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
void xzynq_shutdown(void *arg)
{
	xzynq_dev_t *xzynq = (xzynq_dev_t *) arg;
	xzynq_stop(&xzynq->ecom.ec_if, 1);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
int xzynq_detach(struct device *dev, int flags)
{
	xzynq_dev_t *xzynq;
	struct ifnet *ifp;
	struct _iopkt_self *iopkt;

	xzynq = (xzynq_dev_t*) dev;
	ifp = &xzynq->ecom.ec_if;
	iopkt = xzynq->iopkt;
	(void)iopkt;

	/* Don't init() while we're dying. */
	xzynq->dying = 1;

	cache_fini(&xzynq->meminfo.cachectl);

	/* Free up allocated resources */
	xzynq_cleanup_descriptors(xzynq);

	/* Reset and stop the hardware */
	xzynq_hw_reset(xzynq);

	/* Detach from the interrupt */
	if (xzynq->iid != -1) {
		InterruptDetach(xzynq->iid);
		xzynq->iid = -1;
	}
	interrupt_entry_remove(&xzynq->inter, NULL); /* Must be 'the stack' to call this */

	/* Detach Interface */
	ether_ifdetach(ifp);
	if_detach(ifp);

	if (xzynq->gpio_regbase) {
		munmap_device_io(xzynq->gpio_regbase, XZYNQ_GPIO_SIZE);
		xzynq->gpio_regbase = NULL;
	}

	if (xzynq->regbase) {
		munmap_device_io(xzynq->regbase, XZYNQ_EMAC_SIZE);
		xzynq->regbase = NULL;
	}

	slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s: Inteface detached\n",
			ifp->if_xname);
	return EOK;
}

/****************************************************************************/
/* TODO: Add support for MULTI-interface                                    */
/****************************************************************************/
int xzynq_entry(void *dll_hdl, struct _iopkt_self *iopkt, char *options)
{
	int32_t single = 1;
	struct device *dev = NULL;
	struct attach_args attach_args;

	/* Ask for IO permission */
	if (ThreadCtl(_NTO_TCTL_IO_PRIV, 0) == -1) {
		perror("ThreadCtl");
		return -1;
	}

	/* Initialize arguments/parameters for xzynq_attach() */
	memset(&attach_args, 0x00, sizeof(attach_args));
	attach_args.iopkt = iopkt;
	attach_args.options = options;

	/* Call dev_attach for every instance found */
	if (dev_attach("xzynq", options, &xzynq_ca, &attach_args, &single,
			&dev, NULL) != EOK) {
		return ENODEV;
	}

	dev->dv_dll_hdl = dll_hdl;

#ifdef CONFIG_PMM
	funcs.standby = xzynq_pm_standby;
	funcs.resume = xzynq_pm_resume;
	pmm_init(&funcs, "eth0");
#endif

	return EOK;
}

/**************************************************************************/
/* HouseKeeping                                                           */
/**************************************************************************/
void xzynq_hk_callout(void *arg)
{
	xzynq_dev_t *xzynq = arg;
	nic_config_t *cfg = &xzynq->cfg;

	/*
	 * we will probe the PHY if:
	 *   the user has forced it from the cmd line, or
	 *   or the link is considered down
	 */
	if (xzynq->probe_phy || cfg->media_rate <= 0 || cfg->flags
			& NIC_FLAG_LINK_DOWN) {
		/*
		 * directly call drvr lib to probe PHY link state which in turn
		 * will call xzynq_mdi_callback() above if link state changes
		 */
		MDI_MonitorPhy(xzynq->mdi);
	}

	/* Re-call the callout in 2s */
	callout_msec(&xzynq->hk_callout, 2 * 1000, xzynq_hk_callout, xzynq);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int xzynq_init(struct ifnet *ifp)
{
	uint32_t tmp, phyid;
	xzynq_dev_t *xzynq = ifp->if_softc;
	struct nw_work_thread *wtp = WTP;
	struct _iopkt_self *iopkt = xzynq->iopkt;

	if (xzynq->dying)
		return 1;

	/* Reset the NIC */
	xzynq_hw_reset(xzynq);

	/* Setup descriptors */
	xzynq_cleanup_descriptors(xzynq);
	xzynq_setup_descriptors(xzynq);

	/* Configure the Network Configuration */
	out32(xzynq->regbase + XZYNQ_EMACPS_NWCFG_OFFSET,
			(0x7 << XZYNQ_EMACPS_NWCFG_MDC_SHIFT_MASK) |
			XZYNQ_EMACPS_NWCFG_FCSREM_MASK |
			XZYNQ_EMACPS_NWCFG_PAUSEEN_MASK |
			XZYNQ_EMACPS_NWCFG_PAUSECOPYDI_MASK |
			XZYNQ_EMACPS_NWCFG_HDRXEN_MASK |
			XZYNQ_EMACPS_NWCFG_RXCHKSUMEN_MASK
			);

	/* Configure the DMA */
	tmp = (0x19 << XZYNQ_EMACPS_DMACR_RXBUF_SHIFT); /* 1536 */
	tmp |= XZYNQ_EMACPS_DMACR_TXSIZE_MASK;
	tmp |= XZYNQ_EMACPS_DMACR_RXSIZE_MASK;
	tmp |= XZYNQ_EMACPS_DMACR_TCPCKSUM_MASK;
	tmp &= ~XZYNQ_EMACPS_DMACR_ENDIAN_MASK;
	tmp |= (XZYNQ_EMACPS_DMACR_BLENGTH_MASK & 16); /* INCR16 */
	out32(xzynq->regbase + XZYNQ_EMACPS_DMACR_OFFSET, tmp);

	/* Disable all the MAC Interrupts */
	out32(xzynq->regbase + XZYNQ_EMACPS_IDR_OFFSET,
			XZYNQ_EMACPS_IXR_ALL_MASK);

	/*
	 * Setup for Network Control register.
	 *	- Enable Receive operation.
	 *	- Enable Transmit operation.
	 *	- Enable MDIO operation.
	 */
	tmp = XZYNQ_EMACPS_NWCTRL_MDEN_MASK | XZYNQ_EMACPS_NWCTRL_TXEN_MASK
			| XZYNQ_EMACPS_NWCTRL_RXEN_MASK;
	out32(xzynq->regbase + XZYNQ_EMACPS_NWCTRL_OFFSET, tmp);

	/* Find the PHY */
	phyid = xzynq_mdi_find_phy(xzynq);

	/* Configure the PHY */
	xzynq_mdi_configure_phy(xzynq, phyid);

	/* Notify link state */
	MDI_MonitorPhy(xzynq->mdi);

	ifp->if_flags_tx |= IFF_RUNNING;
	ifp->if_flags_tx &= ~IFF_OACTIVE;
	NW_SIGUNLOCK_P(&ifp->if_snd_ex, iopkt, wtp);
	ifp->if_flags |= IFF_RUNNING;

	/* Enable RX interrupts */
	out32(xzynq->regbase + XZYNQ_EMACPS_IDR_OFFSET, XZYNQ_EMACPS_IXR_ALL_MASK);
	out32(xzynq->regbase + XZYNQ_EMACPS_IER_OFFSET, XZYNQ_EMACPS_IXR_FRAMERX_MASK);

	callout_msec(&xzynq->hk_callout, 10 * 1000, xzynq_hk_callout, xzynq);

	return EOK;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/lib/io-pkt/sys/dev_qnx/xzynq/xzynq.c $ $Rev: 752035 $")
#endif
