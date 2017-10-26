/*
 * $QNXLicenseC:
 * Copyright 2007, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable license fees to QNX
 * Software Systems before you may reproduce, modify or distribute this software,
 * or any work that includes all or part of this software.   Free development
 * licenses are available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review this entire
 * file for other proprietary rights or license notices, as well as the QNX
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/
 * for other information.
 * $
 */

#ifndef _FPGA_LIB_H_INCLUDED
#define _FPGA_LIB_H_INCLUDED

/*
 * Resource Manager Interface
 */

#define FPGA_NAME	"/dev/fpga"

/* Types of PCAP transfers */
#define FPGA_NON_SECURE_PCAP_WRITE		1
#define FPGA_SECURE_PCAP_WRITE			2
#define FPGA_PCAP_READBACK			3
#define FPGA_CONCURRENT_SECURE_READ_WRITE	4
#define FPGA_CONCURRENT_NONSEC_READ_WRITE	5

/*
 * The following devctls are used by a client application
 * to control the FPGA interface.
 */

#include <devctl.h>

#define _DCMD_FPGA   _DCMD_MISC

#define DCMD_FPGA_RESET				__DION (_DCMD_FPGA, 1)
#define DCMD_FPGA_ENABLE_SEC			__DION (_DCMD_FPGA, 2)
#define DCMD_FPGA_DISABLE_SEC			__DION (_DCMD_FPGA, 3)
#define DCMD_FPGA_IS_PROG_DONE			__DIOF (_DCMD_FPGA, 4, _Uint8t)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/fpga/public/hw/fpga.h $ $Rev: 752035 $")
#endif
