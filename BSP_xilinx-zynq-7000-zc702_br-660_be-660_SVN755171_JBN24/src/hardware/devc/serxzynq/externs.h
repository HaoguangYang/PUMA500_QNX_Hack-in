/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems.
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

#ifdef DEFN
    #define EXT
    #define INIT1(a)                = { a }
#else
    #define EXT extern
    #define INIT1(a)
#endif

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <termios.h>
#include <devctl.h>
#include <sys/dcmd_chr.h>
#include <sys/iomsg.h>
#include <atomic.h>
#include <hw/inout.h>
#include <sys/io-char.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <pthread.h>
#include <sys/rsrcdbmgr.h>
#include <sys/dispatch.h>

#include <arm/xzynq.h>


#define FIFO_SIZE 64

typedef struct dev_xzynq {
    TTYDEV          tty;
    unsigned        intr[2];
    int             iid[2];
    unsigned        clk;
    unsigned        div;
    unsigned        fifo;
    uintptr_t       base;
    unsigned        cr;
    unsigned        mr;
    unsigned        brgr;
    int             isr;
} DEV_XZYNQ;

EXT DEV_XZYNQ *devptr;

typedef struct ttyinit_xzynq {
    TTYINIT     tty;
    unsigned    intr[2];   /* Interrupts */
    int         isr;
} TTYINIT_XZYNQ;

EXT TTYCTRL        ttyctrl;

extern unsigned mphys(void *);

#include "proto.h"

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/devc/serxzynq/externs.h $ $Rev: 752035 $")
#endif
