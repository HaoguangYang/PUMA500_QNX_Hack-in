/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include <proto.h>

static xadc_dev_t *dev;

int main(int argc, char *argv[])
{
	int id;
	pthread_attr_t attr;
	resmgr_connect_funcs_t xadc_connect_funcs;
	resmgr_io_funcs_t xadc_io_funcs;
	dispatch_t *dpp;
	resmgr_attr_t rattr;
	dispatch_context_t *ctp;
	iofunc_attr_t xadc_ioattr;

	/* Initialize the dispatch interface */
	dpp = dispatch_create();
	if (!dpp) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"XADC error: Failed to create dispatch interface\n");
		goto fail;
	}

	/* Initialize the resource manager attributes */
	memset(&rattr, 0, sizeof(rattr));

	/* Initialize the connect functions */
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &xadc_connect_funcs,
			_RESMGR_IO_NFUNCS, &xadc_io_funcs);
	xadc_connect_funcs.open = xadc_io_open;
	xadc_io_funcs.devctl = xadc_io_devctl;
	iofunc_attr_init(&xadc_ioattr, S_IFCHR | 0666, NULL, NULL);

	/* Attach the device name */
	id = resmgr_attach(dpp, &rattr, XADC_NAME, _FTYPE_ANY, 0,
			&xadc_connect_funcs, &xadc_io_funcs, &xadc_ioattr);
	if (id == -1) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"XADC error: Failed to attach pathname\n");
		goto fail;
	}

	/* Allocate a context structure */
	ctp = dispatch_context_alloc(dpp);

	dev = malloc(sizeof(xadc_dev_t));
	if (!dev) {
		goto fail;
	}

	/* The XADC uses the devcfg interface, so all the register offsets are
	 * based on the devcfg physical address
	 */
	dev->reglen = XZYNQ_DEVCFG_SIZE;
	dev->physbase = XZYNQ_DEVCFG_BASEADDR;

	dev->regbase = mmap_device_io(dev->reglen, dev->physbase);
	if (dev->regbase == MAP_DEVICE_FAILED) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"Failed to map XADC registers\n");
		goto fail;
	}

	/* Initialize storage for alarms to handle */
	dev->alarm_notifications
			= malloc(XADC_NUM_ALARMS * sizeof(struct sigevent));
	if (!dev->alarm_notifications) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"Failed to allocate memory for XADC alarm notifications\n");
		goto fail;
	}
	dev->alarm_notification_ids = malloc(XADC_NUM_ALARMS * sizeof(int));
	if (!dev->alarm_notification_ids) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"Failed to allocate memory for XADC alarm notification addresses\n");
		goto fail;
	}
	memset(dev->alarm_notification_ids, -1, XADC_NUM_ALARMS * sizeof(int));

	/* Start the interrupt handling thread */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(NULL, &attr, &xadc_intr_thread, dev);

	/* Run in the background */
	if (procmgr_daemon(EXIT_SUCCESS, PROCMGR_DAEMON_NOCLOSE
			| PROCMGR_DAEMON_NODEVNULL ) == -1) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "%s:  procmgr_daemon",
				argv[0]);
		goto fail;
	}

	while (1) {
		if ((ctp = dispatch_block(ctp)) == NULL) {
			slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
					"XADC error: Block error\n");
			goto fail;
		}
		dispatch_handler(ctp);
	}

	free(dev);
	free(dev->alarm_notifications);
	free(dev->alarm_notification_ids);

	return EXIT_SUCCESS;

	fail: if (dev != NULL) {
		if (dev->alarm_notifications != NULL) {
			free(dev->alarm_notifications);
		}
		if (dev->alarm_notification_ids != NULL) {
			free(dev->alarm_notification_ids);
		}
		free(dev);
	}
	return EXIT_FAILURE;
}

/*
 * Initialize the XADC
 */
int xadc_io_open(resmgr_context_t *ctp, io_open_t *msg,
		RESMGR_HANDLE_T *handle, void *extra)
{
	xadc_cfg_init(dev);
	return (iofunc_open_default(ctp, msg, handle, extra));
}

/*
 * The entire XADC API is handled through devctl messages. The functionality is
 * implemented in init.c, lib.c, and intr.c. This function mainly just translates
 * a devctl call into a library call. The main exception is in setting alarm
 * enable bits and handling interrupts behind the scenes.
 */
int xadc_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
	void *dptr;
	int status, nbytes;
	_Uint32t intr_mask;
	int id;

	status = iofunc_devctl_default(ctp, msg, ocb);
	if (status != _RESMGR_DEFAULT) {
		return status;
	}

	/*
	if (xadc_cmd_fifo_status(dev) || xadc_data_fifo_status(dev)) {
		return EBUSY;
	}
	*/

	nbytes = 0;
	dptr = _DEVCTL_DATA(msg->i);

	switch (msg->i.dcmd) {
	case DCMD_XADC_CFG_INIT:
		xadc_cfg_init(dev);
		break;
	case DCMD_XADC_SET_CFG_REG:
		xadc_set_config_register(dev, *((_Uint32t *) dptr));
		break;
	case DCMD_XADC_GET_CFG_REG:
		*((_Uint32t *) dptr) = xadc_get_config_register(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_GET_MISC_STATUS:
		*((_Uint32t *) dptr) = xadc_get_misc_status(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_SET_MISC_CTRL_REG:
		xadc_set_misc_ctrl_register(dev, *((_Uint32t *) dptr));
		break;
	case DCMD_XADC_GET_MISC_CTRL_REG:
		*((_Uint32t *) dptr) = xadc_get_misc_ctrl_register(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_RESET:
		xadc_reset(dev);
		break;
	case DCMD_XADC_GET_ADC_DATA:
		((xadc_data_read_t *) dptr)->data = xadc_get_adc_data(dev,
				((xadc_data_read_t *) dptr)->channel);
		nbytes = sizeof(xadc_data_read_t);
		break;
	case DCMD_XADC_GET_CALIB_COEFF:
		((xadc_calib_coeff_read_t *) dptr)->calib_coeff
				= xadc_get_calib_coefficient(dev,
						((xadc_calib_coeff_read_t *) dptr)->coeff_type);
		nbytes = sizeof(xadc_calib_coeff_read_t);
		break;
	case DCMD_XADC_GET_MIN_MAX_MSRMT:
		((xadc_min_max_measure_read_t *) dptr)->min_max_measure
				= xadc_get_min_max_measurement(
						dev,
						((xadc_min_max_measure_read_t *) dptr)->measurement_type);
		nbytes = sizeof(xadc_min_max_measure_read_t);
		break;
	case DCMD_XADC_SET_AVG:
		xadc_set_avg(dev, *((_Uint8t *) dptr));
		break;
	case DCMD_XADC_GET_AVG:
		*((_Uint8t *) dptr) = xadc_get_avg(dev);
		nbytes = sizeof(_Uint8t);
		break;
	case DCMD_XADC_SET_SINGLE_CH_PARAM:
		((xadc_single_ch_params_t *) dptr)->result_code
				= xadc_set_single_ch_params(
						dev,
						((xadc_single_ch_params_t *) dptr)->channel,
						((xadc_single_ch_params_t *) dptr)->increase_acq_cycles,
						((xadc_single_ch_params_t *) dptr)->is_event_mode,
						((xadc_single_ch_params_t *) dptr)->is_differential_mode);
		nbytes = sizeof(xadc_single_ch_params_t);
		break;
	case DCMD_XADC_SET_ALARM_ENABLES:
		/* If we are enabling the alarms then save the id of the caller to
		 * send a notification when we are done. Else save -1, which indicates
		 * a disabled alarm.
		 */
		if (((xadc_alarm_enables_write_t *) dptr)->enable == XADC_ALARM_ENABLE) {
			id = ctp->rcvid;
		} else {
			id = -1;
		}

		intr_mask = xadc_alarm_to_interrupt_mask(
				((xadc_alarm_enables_write_t *) dptr)->mask);
		if (intr_mask & XADC_INTX_ALM0_MASK) {
			dev->alarm_notifications[0]
					= ((xadc_alarm_enables_write_t *) dptr)->event;
			dev->alarm_notification_ids[0] = id;
		}
		if (intr_mask & XADC_INTX_ALM1_MASK) {
			dev->alarm_notifications[1]
					= ((xadc_alarm_enables_write_t *) dptr)->event;
			dev->alarm_notification_ids[1] = id;
		}
		if (intr_mask & XADC_INTX_ALM2_MASK) {
			dev->alarm_notifications[2]
					= ((xadc_alarm_enables_write_t *) dptr)->event;
			dev->alarm_notification_ids[2] = id;
		}
		if (intr_mask & XADC_INTX_ALM3_MASK) {
			dev->alarm_notifications[3]
					= ((xadc_alarm_enables_write_t *) dptr)->event;
			dev->alarm_notification_ids[3] = id;
		}
		if (intr_mask & XADC_INTX_ALM4_MASK) {
			dev->alarm_notifications[4]
					= ((xadc_alarm_enables_write_t *) dptr)->event;
			dev->alarm_notification_ids[4] = id;
		}
		if (intr_mask & XADC_INTX_ALM5_MASK) {
			dev->alarm_notifications[5]
					= ((xadc_alarm_enables_write_t *) dptr)->event;
			dev->alarm_notification_ids[5] = id;
		}
		if (intr_mask & XADC_INTX_ALM6_MASK) {
			dev->alarm_notifications[6]
					= ((xadc_alarm_enables_write_t *) dptr)->event;
			dev->alarm_notification_ids[6] = id;
		}

		xadc_intr_clear(dev, intr_mask);

		/*
		 * Update alarm enables register as well as interrupt register
		 */
		if (((xadc_alarm_enables_write_t *) dptr)->enable == XADC_ALARM_ENABLE) {
			xadc_intr_enable(dev, intr_mask);
			xadc_set_alarm_enables(dev,
					(((xadc_alarm_enables_write_t *) dptr)->mask)
							| xadc_get_alarm_enables(dev));
		} else {
			xadc_intr_disable(dev, intr_mask);
			xadc_set_alarm_enables(dev,
					(~(((xadc_alarm_enables_write_t *) dptr)->mask))
							& xadc_get_alarm_enables(dev));
		}

		break;
	case DCMD_XADC_GET_ALARM_ENABLES:
		*((_Uint16t *) dptr) = xadc_get_alarm_enables(dev);
		nbytes = sizeof(_Uint16t);
		break;
	case DCMD_XADC_SET_CALIB_ENABLES:
		xadc_set_calib_enables(dev, *((_Uint16t *) dptr));
		break;
	case DCMD_XADC_GET_CALIB_ENABLES:
		*((_Uint16t *) dptr) = xadc_get_calib_enables(dev);
		nbytes = sizeof(_Uint16t);
		break;
	case DCMD_XADC_SET_SEQUENCER_MODE:
		xadc_set_sequencer_mode(dev, *((_Uint8t *) dptr));
		break;
	case DCMD_XADC_GET_SEQUENCER_MODE:
		*((_Uint8t *) dptr) = xadc_get_sequencer_mode(dev);
		nbytes = sizeof(_Uint8t);
		break;
	case DCMD_XADC_SET_ADC_CLK_DIVISOR:
		xadc_set_adc_clk_divisor(dev, *((_Uint8t *) dptr));
		break;
	case DCMD_XADC_GET_ADC_CLK_DIVISOR:
		*((_Uint8t *) dptr) = xadc_get_adc_clk_divisor(dev);
		nbytes = sizeof(_Uint8t);
		break;
	case DCMD_XADC_SET_SEQ_CH_ENABLES:
		((xadc_seq_write_t *) dptr)->result_code = xadc_set_seq_ch_enables(dev,
				((xadc_seq_write_t *) dptr)->mask);
		nbytes = sizeof(xadc_seq_write_t);
		break;
	case DCMD_XADC_GET_SEQ_CH_ENABLES:
		*((_Uint32t *) dptr) = xadc_get_seq_ch_enables(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_SET_SEQ_AVG_ENABLES:
		((xadc_seq_write_t *) dptr)->result_code = xadc_set_seq_avg_enables(
				dev, ((xadc_seq_write_t *) dptr)->mask);
		nbytes = sizeof(xadc_seq_write_t);
		break;
	case DCMD_XADC_GET_SEQ_AVG_ENABLES:
		*((_Uint32t *) dptr) = xadc_get_seq_avg_enables(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_SET_SEQ_INPUT_MODE:
		((xadc_seq_write_t *) dptr)->result_code = xadc_set_seq_input_mode(dev,
				((xadc_seq_write_t *) dptr)->mask);
		nbytes = sizeof(xadc_seq_write_t);
		break;
	case DCMD_XADC_GET_SEQ_INPUT_MODE:
		*((_Uint32t *) dptr) = xadc_get_seq_input_mode(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_SET_SEQ_ACQ_TIME:
		((xadc_seq_write_t *) dptr)->result_code = xadc_set_seq_acq_time(dev,
				((xadc_seq_write_t *) dptr)->mask);
		nbytes = sizeof(xadc_seq_write_t);
		break;
	case DCMD_XADC_GET_SEQ_ACQ_TIME:
		*((_Uint32t *) dptr) = xadc_get_seq_acq_time(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_SET_ALARM_THRESHOLD:
		xadc_set_alarm_threshold(dev,
				((xadc_alarm_threshold_t *) dptr)->alarm_thr_reg,
				((xadc_alarm_threshold_t *) dptr)->value);
		break;
	case DCMD_XADC_GET_ALARM_THRESHOLD:
		((xadc_alarm_threshold_t *) dptr)->value = xadc_get_alarm_threshold(
				dev, ((xadc_alarm_threshold_t *) dptr)->alarm_thr_reg);
		nbytes = sizeof(xadc_alarm_threshold_t);
		break;
	case DCMD_XADC_ENABLE_USER_OVER_TEMP:
		xadc_enable_user_over_temp(dev);
		break;
	case DCMD_XADC_DISABLE_USER_OVER_TEMP:
		xadc_disable_user_over_temp(dev);
		break;
	case DCMD_XADC_SET_SEQUENCER_EVENT:
		xadc_set_sequencer_event(dev, *((int *) dptr));
		break;
	case DCMD_XADC_GET_SAMPLING_MODE:
		*((int *) dptr) = xadc_get_sampling_mode(dev);
		nbytes = sizeof(int);
		break;
	case DCMD_XADC_SET_MUX_MODE:
		xadc_set_mux_mode(dev, ((xadc_mux_mode_t *) dptr)->mux_mode,
				((xadc_mux_mode_t *) dptr)->channel);
		break;
	case DCMD_XADC_SET_POWER_DOWN_MODE:
		xadc_set_power_down_mode(dev, *((_Uint32t *) dptr));
		break;
	case DCMD_XADC_GET_POWER_DOWN_MODE:
		*((_Uint32t *) dptr) = xadc_get_power_down_mode(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_WRITE_INTERNAL_REG:
		xadc_write_internal_reg(dev, ((xadc_internal_reg_t *) dptr)->offset,
				((xadc_internal_reg_t *) dptr)->data);
		break;
	case DCMD_XADC_READ_INTERNAL_REG:
		((xadc_internal_reg_t *) dptr)->data = xadc_read_internal_reg(dev,
				((xadc_internal_reg_t *) dptr)->offset);
		nbytes = sizeof(xadc_internal_reg_t);
		break;
	case DCMD_XADC_INTR_ENABLE:
		xadc_intr_enable(dev, *((_Uint32t *) dptr));
		break;
	case DCMD_XADC_INTR_DISABLE:
		xadc_intr_disable(dev, *((_Uint32t *) dptr));
		break;
	case DCMD_XADC_INTR_GET_ENABLED:
		*((_Uint32t *) dptr) = xadc_intr_get_enabled(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_INTR_GET_STATUS:
		*((_Uint32t *) dptr) = xadc_intr_get_status(dev);
		nbytes = sizeof(_Uint32t);
		break;
	case DCMD_XADC_INTR_CLEAR:
		xadc_intr_clear(dev, *((_Uint32t *) dptr));
		break;
	default:
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,
				"XADC error: devctl error\n");
		return ENOTTY;
	}

	if (nbytes == 0) {
		return (EOK);
	} else {
		msg->o.ret_val = 0;
		msg->o.nbytes = nbytes;
		return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + nbytes));
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/xadc/main.c $ $Rev: 752035 $")
#endif
