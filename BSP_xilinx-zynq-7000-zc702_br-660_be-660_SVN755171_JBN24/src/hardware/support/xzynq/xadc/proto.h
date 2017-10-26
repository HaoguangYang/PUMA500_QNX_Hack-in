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

#ifndef __PROTO_H_INCLUDED
#define __PROTO_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <errno.h>
#include <arm/xzynq.h>
#include <sys/procmgr.h>
#include <drvr/hwinfo.h>
#include <string.h>
#include <hw/xadc.h>

typedef struct _xadc_dev {

	/* Memory location of XADC (though really it's the devcfg) */
	unsigned reglen;
	uintptr_t regbase;
	unsigned physbase;

	/* Manage interrupts from the XADC register */
	struct sigevent xadc_intr;

	/* The list of user events to pulse when desired alarms are triggered */
	struct sigevent *alarm_notifications;
	int *alarm_notification_ids;

} xadc_dev_t;

int xadc_io_open(resmgr_context_t *ctp, io_open_t *msg,
		RESMGR_HANDLE_T *handle, void *extra);
int xadc_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb);
void xadc_reset(xadc_dev_t *dev);
void xadc_cfg_init(xadc_dev_t *dev);
void xadc_intr_enable(xadc_dev_t *dev, _Uint32t mask);
void xadc_intr_disable(xadc_dev_t *dev, _Uint32t mask);
_Uint32t xadc_intr_get_enabled(xadc_dev_t *dev);
_Uint32t xadc_intr_get_status(xadc_dev_t *dev);
void xadc_intr_clear(xadc_dev_t *dev, _Uint32t mask);
void xadc_set_config_register(xadc_dev_t *dev, _Uint32t data);
_Uint32t xadc_get_config_register(xadc_dev_t *dev);
_Uint32t xadc_get_misc_status(xadc_dev_t *dev);
void xadc_set_misc_ctrl_register(xadc_dev_t *dev, _Uint32t data);
_Uint32t xadc_get_misc_ctrl_register(xadc_dev_t *dev);
_Uint16t xadc_get_adc_data(xadc_dev_t *dev, _Uint8t channel);
_Uint16t xadc_get_calib_coefficient(xadc_dev_t *dev, _Uint8t coeff_type);
_Uint16t
		xadc_get_min_max_measurement(xadc_dev_t *dev, _Uint8t measurement_type);
void xadc_set_avg(xadc_dev_t *dev, _Uint8t average);
_Uint8t xadc_get_avg(xadc_dev_t *dev);
int xadc_set_single_ch_params(xadc_dev_t *dev, _Uint8t channel,
		int increase_acq_cycles, int is_event_mode, int is_differential_mode);
void xadc_set_alarm_enables(xadc_dev_t *dev, _Uint16t alm_enable_mask);
_Uint16t xadc_get_alarm_enables(xadc_dev_t *dev);
void xadc_set_calib_enables(xadc_dev_t *dev, _Uint16t calibration);
_Uint16t xadc_get_calib_enables(xadc_dev_t *dev);
void xadc_set_sequencer_mode(xadc_dev_t *dev, _Uint8t sequencer_mode);
_Uint8t xadc_get_sequencer_mode(xadc_dev_t *dev);
void xadc_set_adc_clk_divisor(xadc_dev_t *dev, _Uint8t divisor);
_Uint8t xadc_get_adc_clk_divisor(xadc_dev_t *dev);
int xadc_set_seq_ch_enables(xadc_dev_t *dev, _Uint32t ch_enable_mask);
_Uint32t xadc_get_seq_ch_enables(xadc_dev_t *dev);
int xadc_set_seq_avg_enables(xadc_dev_t *dev, _Uint32t avg_enable_ch_mask);
_Uint32t xadc_get_seq_avg_enables(xadc_dev_t *dev);
int xadc_set_seq_input_mode(xadc_dev_t *dev, _Uint32t input_mode_ch_mask);
_Uint32t xadc_get_seq_input_mode(xadc_dev_t *dev);
int xadc_set_seq_acq_time(xadc_dev_t *dev, _Uint32t acq_cycles_ch_mask);
_Uint32t xadc_get_seq_acq_time(xadc_dev_t *dev);
void xadc_set_alarm_threshold(xadc_dev_t *dev, _Uint8t alarm_thr_reg,
		_Uint16t value);
_Uint16t xadc_get_alarm_threshold(xadc_dev_t *dev, _Uint8t alarm_thr_reg);
void xadc_enable_user_over_temp(xadc_dev_t *dev);
void xadc_disable_user_over_temp(xadc_dev_t *dev);
void xadc_set_sequencer_event(xadc_dev_t *dev, int is_event_mode);
int xadc_get_sampling_mode(xadc_dev_t *dev);
void xadc_set_mux_mode(xadc_dev_t *dev, int mux_mode, _Uint8t channel);
void xadc_set_power_down_mode(xadc_dev_t *dev, _Uint32t mode);
_Uint32t xadc_get_power_down_mode(xadc_dev_t *dev);
void xadc_write_internal_reg(xadc_dev_t *dev, _Uint32t offset, _Uint32t data);
_Uint32t xadc_read_internal_reg(xadc_dev_t *dev, _Uint32t offset);
struct sigevent *xadc_intr_hdlr(void *area, int id);
void *xadc_intr_thread(void *arg);
_Uint32t xadc_alarm_to_interrupt_mask(_Uint16t alarm_mask);
_Uint8t xadc_cmd_fifo_status(xadc_dev_t *dev);
_Uint8t xadc_data_fifo_status(xadc_dev_t *dev);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/xadc/proto.h $ $Rev: 752035 $")
#endif
