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

#ifndef _XADC_LIB_H_INCLUDED
#define _XADC_LIB_H_INCLUDED

/* Indexes for the different channels. */
#define XADC_CH_TEMP		0x0  /**< On Chip Temperature */
#define XADC_CH_VCCINT	0x1  /**< VCCINT */
#define XADC_CH_VCCAUX	0x2  /**< VCCAUX */
#define XADC_CH_VPVN		0x3  /**< VP/VN Dedicated analog inputs */
#define XADC_CH_VREFP		0x4  /**< VREFP */
#define XADC_CH_VREFN		0x5  /**< VREFN */
#define XADC_CH_VBRAM		0x6  /**< On-chip VBRAM Data Reg, 7 series */
#define XADC_CH_SUPPLY_CALIB	0x07 /**< Supply Calib Data Reg */
#define XADC_CH_ADC_CALIB	0x08 /**< ADC Offset Channel Reg */
#define XADC_CH_GAINERR_CALIB 0x09 /**< Gain Error Channel Reg  */
#define XADC_CH_VCCPINT	0x0D /**< On-chip PS VCCPINT Channel , Zynq */
#define XADC_CH_VCCPAUX	0x0E /**< On-chip PS VCCPAUX Channel , Zynq */
#define XADC_CH_VCCPDRO	0x0F /**< On-chip PS VCCPDRO Channel , Zynq */
#define XADC_CH_AUX_MIN	 16 /**< Channel number for 1st Aux Channel */
#define XADC_CH_AUX_MAX	 31 /**< Channel number for Last Aux channel */

/* Indexes for reading the Calibration Coefficient Data. */
#define XADC_CALIB_SUPPLY_COEFF     0 /**< Supply Offset Calib Coefficient */
#define XADC_CALIB_ADC_COEFF        1 /**< ADC Offset Calib Coefficient */
#define XADC_CALIB_GAIN_ERROR_COEFF 2 /**< Gain Error Calib Coefficient*/

/* Indexes for reading the Minimum/Maximum Measurement Data. */
#define XADC_MAX_TEMP		0 /**< Maximum Temperature Data */
#define XADC_MAX_VCCINT	1 /**< Maximum VCCINT Data */
#define XADC_MAX_VCCAUX	2 /**< Maximum VCCAUX Data */
#define XADC_MAX_VBRAM	3 /**< Maximum VBRAM Data */
#define XADC_MIN_TEMP		4 /**< Minimum Temperature Data */
#define XADC_MIN_VCCINT	5 /**< Minimum VCCINT Data */
#define XADC_MIN_VCCAUX	6 /**< Minimum VCCAUX Data */
#define XADC_MIN_VBRAM	7 /**< Minimum VBRAM Data */
#define XADC_MAX_VCCPINT	8 /**< Maximum VCCPINT Register , Zynq */
#define XADC_MAX_VCCPAUX	9 /**< Maximum VCCPAUX Register , Zynq */
#define XADC_MAX_VCCPDRO	0xA /**< Maximum VCCPDRO Register , Zynq */
#define XADC_MIN_VCCPINT	0xC /**< Minimum VCCPINT Register , Zynq */
#define XADC_MIN_VCCPAUX	0xD /**< Minimum VCCPAUX Register , Zynq */
#define XADC_MIN_VCCPDRO	0xE /**< Minimum VCCPDRO Register , Zynq */

/* Alarm Threshold(Limit) Register (ATR) indexes. */
#define XADC_ATR_TEMP_UPPER	 0 /**< High user Temperature */
#define XADC_ATR_VCCINT_UPPER  1 /**< VCCINT high voltage limit register */
#define XADC_ATR_VCCAUX_UPPER  2 /**< VCCAUX high voltage limit register */
#define XADC_ATR_OT_UPPER	 3 /**< VCCAUX high voltage limit register */
#define XADC_ATR_TEMP_LOWER	 4 /**< Upper Over Temperature limit Reg */
#define XADC_ATR_VCCINT_LOWER	 5 /**< VCCINT high voltage limit register */
#define XADC_ATR_VCCAUX_LOWER	 6 /**< VCCAUX low voltage limit register  */
#define XADC_ATR_OT_LOWER	 7 /**< Lower Over Temperature limit */
#define XADC_ATR_VBRAM_UPPER_  8 /**< VRBAM Upper Alarm Reg, 7 Series */
#define XADC_ATR_VCCPINT_UPPER 9 /**< VCCPINT Upper Alarm Reg, Zynq */
#define XADC_ATR_VCCPAUX_UPPER 0xA /**< VCCPAUX Upper Alarm Reg, Zynq */
#define XADC_ATR_VCCPDRO_UPPER 0xB /**< VCCPDRO Upper Alarm Reg, Zynq */
#define XADC_ATR_VBRAM_LOWER	 0xC /**< VRBAM Lower Alarm Reg, 7 Series */
#define XADC_ATR_VCCPINT_LOWER 0xD /**< VCCPINT Lower Alarm Reg , Zynq */
#define XADC_ATR_VCCPAUX_LOWER 0xE /**< VCCPAUX Lower Alarm Reg , Zynq */
#define XADC_ATR_VCCPDRO_LOWER 0xF /**< VCCPDRO Lower Alarm Reg , Zynq */

/* Averaging to be done for the channels. */
#define XADC_AVG_0_SAMPLES	0  /**< No Averaging */
#define XADC_AVG_16_SAMPLES	1  /**< Average 16 samples */
#define XADC_AVG_64_SAMPLES	2  /**< Average 64 samples */
#define XADC_AVG_256_SAMPLES	3  /**< Average 256 samples */

/* Channel Sequencer Modes of operation */
#define XADC_SEQ_MODE_SAFE		0  /**< Default Safe Mode */
#define XADC_SEQ_MODE_ONEPASS		1  /**< Onepass through Sequencer */
#define XADC_SEQ_MODE_CONTINPASS	2  /**< Continuous Cycling Sequencer */
#define XADC_SEQ_MODE_SINGCHAN	3  /**< Single channel -No Sequencing */
#define XADC_SEQ_MODE_SIMUL_SAMPLING	4  /**< Simultaneous sampling */
#define XADC_SEQ_MODE_INDEPENDENT	8  /**< Independent mode */

/* Power Down Modes */
#define XADC_PD_MODE_NONE		0  /**< No Power Down  */
#define XADC_PD_MODE_ADCB		1  /**< Power Down ADC B */
#define XADC_PD_MODE_XADC		2  /**< Power Down ADC A and ADC B */

/* Alarm masks */
#define XADC_CFR1_ALM_VCCPDRO_MASK	  	0x0800 /**< Alm 6 - VCCPDRO, Zynq  */
#define XADC_CFR1_ALM_VCCPAUX_MASK	  	0x0400 /**< Alm 5 - VCCPAUX, Zynq */
#define XADC_CFR1_ALM_VCCPINT_MASK	  	0x0200 /**< Alm 4 - VCCPINT, Zynq */
#define XADC_CFR1_ALM_VBRAM_MASK	  	0x0100 /**< Alm 3 - VBRAM, 7 series */
#define XADC_CFR1_ALM_ALL_MASK			0x0F0F /**< Mask for all alarms */
#define XADC_CFR1_ALM_VCCAUX_MASK		0x0008 /**< Alarm 2 - VCCAUX Enable */
#define XADC_CFR1_ALM_VCCINT_MASK		0x0004 /**< Alarm 1 - VCCINT Enable */
#define XADC_CFR1_ALM_TEMP_MASK			0x0002 /**< Alarm 0 - Temperature */
#define XADC_CFR1_OT_MASK				0x0001 /**< Over Temperature Enable */

#define XADC_NUM_ALARMS	8 /* The number of alarms that are available to be enabled */
#define XADC_ALARM_ENABLE	0	/* Use to set the enable bit in an alarm_enables_write_t */
#define XADC_ALARM_DISABLE	1

/* Calibration masks */
#define XADC_CFR1_CAL_VALID_MASK	  		0x00F0 /**< Valid Calibration Mask */
#define XADC_CFR1_CAL_PS_GAIN_OFFSET_MASK 	0x0080 /**< Calibration 3 -Power Supply Gain/Offset Enable */
#define XADC_CFR1_CAL_PS_OFFSET_MASK	  	0x0040 /**< Calibration 2 -Power Supply Offset Enable */
#define XADC_CFR1_CAL_ADC_GAIN_OFFSET_MASK 	0x0020 /**< Calibration 1 -ADC Gain Offset Enable */
#define XADC_CFR1_CAL_ADC_OFFSET_MASK	 	0x0010 /**< Calibration 0 -ADC Offset Enable */
#define XADC_CFR1_CAL_DISABLE_MASK			0x0000 /**< No Calibration */

/* XADC Interrupt Status/Mask Register Bit definitions */
#define XADC_INTX_ALL_MASK   	   		0x000003FF /**< Alarm Signals Mask  */
#define XADC_INTX_CFIFO_LTH_MASK 		0x00000200 /**< CMD FIFO less than threshold */
#define XADC_INTX_DFIFO_GTH_MASK 		0x00000100 /**< Data FIFO greater than threshold */
#define XADC_INTX_OT_MASK	   			0x00000080 /**< Over temperature Alarm Status */
#define XADC_INTX_ALM_ALL_MASK   		0x0000007F /**< Alarm Signals Mask  */
#define XADC_INTX_ALM6_MASK	   			0x00000040 /**< Alarm 6 Mask  */
#define XADC_INTX_ALM5_MASK	   			0x00000020 /**< Alarm 5 Mask  */
#define XADC_INTX_ALM4_MASK	   			0x00000010 /**< Alarm 4 Mask  */
#define XADC_INTX_ALM3_MASK	   			0x00000008 /**< Alarm 3 Mask  */
#define XADC_INTX_ALM2_MASK	   			0x00000004 /**< Alarm 2 Mask  */
#define XADC_INTX_ALM1_MASK	   			0x00000002 /**< Alarm 1 Mask  */
#define XADC_INTX_ALM0_MASK	   			0x00000001 /**< Alarm 0 Mask  */

/*
 * Resource Manager Interface
 */

#define XADC_NAME	"/dev/xadc"

typedef struct {
	_Uint8t channel;
	_Uint16t data;
} xadc_data_read_t;

typedef struct {
	_Uint8t coeff_type;
	_Uint16t calib_coeff;
} xadc_calib_coeff_read_t;

typedef struct {
	_Uint8t measurement_type;
	_Uint16t min_max_measure;
} xadc_min_max_measure_read_t;

typedef struct {
	_Uint8t channel;
	int increase_acq_cycles;
	int is_event_mode;
	int is_differential_mode;
	int result_code;
} xadc_single_ch_params_t;

typedef struct {
	_Uint16t mask;
	struct sigevent event;
	int enable;
} xadc_alarm_enables_write_t;

typedef struct {
	_Uint32t mask;
	int result_code;
} xadc_seq_write_t;

typedef struct {
	_Uint8t alarm_thr_reg;
	_Uint16t value;
} xadc_alarm_threshold_t;

typedef struct {
	int mux_mode;
	_Uint8t channel;
} xadc_mux_mode_t;

typedef struct {
	_Uint32t offset;
	_Uint32t data;
} xadc_internal_reg_t;

/*
 * The following devctls are used by a client application
 * to control the XADC interface.
 */

#include <devctl.h>

#define _DCMD_XADC   _DCMD_MISC

#define DCMD_XADC_CFG_INIT					__DION (_DCMD_XADC, 1)
#define DCMD_XADC_SET_CFG_REG				__DIOT (_DCMD_XADC, 2, _Uint32t)
#define DCMD_XADC_GET_CFG_REG				__DIOF (_DCMD_XADC, 3, _Uint32t)
#define DCMD_XADC_GET_MISC_STATUS			__DIOF (_DCMD_XADC, 4, _Uint32t)
#define DCMD_XADC_SET_MISC_CTRL_REG			__DIOT (_DCMD_XADC, 5, _Uint32t)
#define DCMD_XADC_GET_MISC_CTRL_REG			__DIOF (_DCMD_XADC, 6, _Uint32t)
#define DCMD_XADC_RESET						__DION (_DCMD_XADC, 7)
#define DCMD_XADC_GET_ADC_DATA				__DIOTF(_DCMD_XADC, 8, xadc_data_read_t)
#define DCMD_XADC_GET_CALIB_COEFF			__DIOTF(_DCMD_XADC, 9, xadc_calib_coeff_read_t)
#define DCMD_XADC_GET_MIN_MAX_MSRMT			__DIOTF(_DCMD_XADC, 10, xadc_min_max_measure_read_t)
#define DCMD_XADC_SET_AVG					__DIOT (_DCMD_XADC, 11, _Uint8t)
#define DCMD_XADC_GET_AVG					__DIOF (_DCMD_XADC, 12, _Uint8t)
#define DCMD_XADC_SET_SINGLE_CH_PARAM		__DIOTF(_DCMD_XADC, 13, xadc_single_ch_params_t)
#define DCMD_XADC_SET_ALARM_ENABLES			__DIOT (_DCMD_XADC, 14, xadc_alarm_enables_write_t)
#define DCMD_XADC_GET_ALARM_ENABLES			__DIOF (_DCMD_XADC, 15, _Uint16t)
#define DCMD_XADC_SET_CALIB_ENABLES			__DIOT (_DCMD_XADC, 16, _Uint16t)
#define DCMD_XADC_GET_CALIB_ENABLES			__DIOF (_DCMD_XADC, 17, _Uint16t)
#define DCMD_XADC_SET_SEQUENCER_MODE		__DIOT (_DCMD_XADC, 18, _Uint8t)
#define DCMD_XADC_GET_SEQUENCER_MODE		__DIOF (_DCMD_XADC, 19, _Uint8t)
#define DCMD_XADC_SET_ADC_CLK_DIVISOR		__DIOT (_DCMD_XADC, 20, _Uint8t)
#define DCMD_XADC_GET_ADC_CLK_DIVISOR		__DIOF (_DCMD_XADC, 21, _Uint8t)
#define DCMD_XADC_SET_SEQ_CH_ENABLES		__DIOTF(_DCMD_XADC, 22, xadc_seq_write_t)
#define DCMD_XADC_GET_SEQ_CH_ENABLES		__DIOF (_DCMD_XADC, 23, _Uint32t)
#define DCMD_XADC_SET_SEQ_AVG_ENABLES		__DIOTF(_DCMD_XADC, 24, xadc_seq_write_t)
#define DCMD_XADC_GET_SEQ_AVG_ENABLES		__DIOF (_DCMD_XADC, 25, _Uint32t)
#define DCMD_XADC_SET_SEQ_INPUT_MODE		__DIOTF(_DCMD_XADC, 26, xadc_seq_write_t)
#define DCMD_XADC_GET_SEQ_INPUT_MODE		__DIOF (_DCMD_XADC, 27, _Uint32t)
#define DCMD_XADC_SET_SEQ_ACQ_TIME			__DIOTF(_DCMD_XADC, 28, xadc_seq_write_t)
#define DCMD_XADC_GET_SEQ_ACQ_TIME			__DIOF (_DCMD_XADC, 29, _Uint32t)
#define DCMD_XADC_SET_ALARM_THRESHOLD		__DIOT (_DCMD_XADC, 30, xadc_alarm_threshold_t)
#define DCMD_XADC_GET_ALARM_THRESHOLD		__DIOTF(_DCMD_XADC, 31, xadc_alarm_threshold_t)
#define DCMD_XADC_ENABLE_USER_OVER_TEMP		__DION (_DCMD_XADC, 32)
#define DCMD_XADC_DISABLE_USER_OVER_TEMP	__DION (_DCMD_XADC, 33)
#define DCMD_XADC_SET_SEQUENCER_EVENT		__DIOT (_DCMD_XADC, 34, int)
#define DCMD_XADC_GET_SAMPLING_MODE			__DIOF (_DCMD_XADC, 35, int)
#define DCMD_XADC_SET_MUX_MODE				__DIOT (_DCMD_XADC, 36, xadc_mux_mode_t)
#define DCMD_XADC_SET_POWER_DOWN_MODE		__DIOT (_DCMD_XADC, 37, _Uint32t)
#define DCMD_XADC_GET_POWER_DOWN_MODE		__DIOF (_DCMD_XADC, 38, _Uint32t)
#define DCMD_XADC_WRITE_INTERNAL_REG		__DIOT (_DCMD_XADC, 39, xadc_internal_reg_t)
#define DCMD_XADC_READ_INTERNAL_REG			__DIOTF(_DCMD_XADC, 40, xadc_internal_reg_t)
#define DCMD_XADC_INTR_ENABLE				__DIOT (_DCMD_XADC, 41, _Uint32t)
#define DCMD_XADC_INTR_DISABLE				__DIOT (_DCMD_XADC, 42, _Uint32t)
#define DCMD_XADC_INTR_GET_ENABLED			__DIOF (_DCMD_XADC, 43, _Uint32t)
#define DCMD_XADC_INTR_GET_STATUS			__DIOF (_DCMD_XADC, 44, _Uint32t)
#define DCMD_XADC_INTR_CLEAR				__DIOT (_DCMD_XADC, 45, _Uint32t)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/xadc/public/hw/xadc.h $ $Rev: 752035 $")
#endif
