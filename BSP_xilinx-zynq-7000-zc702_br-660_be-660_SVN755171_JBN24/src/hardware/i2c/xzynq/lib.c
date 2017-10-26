/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
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

#include "proto.h"

int i2c_master_getfuncs(i2c_master_funcs_t *funcs, int tabsize) {
	I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
			init, xzynq_init, tabsize);
	I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
			fini, xzynq_fini, tabsize);
	I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
			send, xzynq_send, tabsize);
	I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
			recv, xzynq_recv, tabsize);
	I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
			set_slave_addr, xzynq_set_slave_addr, tabsize);
	I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
			set_bus_speed, xzynq_set_bus_speed, tabsize);
	I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
			version_info, xzynq_version_info, tabsize);
	I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
			driver_info, xzynq_driver_info, tabsize);
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/i2c/xzynq/lib.c $ $Rev: 752035 $")
#endif
