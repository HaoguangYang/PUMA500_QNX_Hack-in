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

/****************************************************************************/
/**
 *
 * The functions sets the contents of the Config Register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	data is the 32 bit data to be written to the Register.
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_set_config_register(xadc_dev_t *dev, _Uint32t data)
{
	out32(dev->regbase + XZYNQ_XADC_CFG_OFFSET, data);

}

/****************************************************************************/
/**
 *
 * The functions reads the contents of the Config Register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	A 32-bit value representing the contents of the Config Register.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint32t xadc_get_config_register(xadc_dev_t *dev)
{
	/*
	 * Read the Config Register and return the value.
	 */
	return in32(dev->regbase + XZYNQ_XADC_CFG_OFFSET);
}

/****************************************************************************/
/**
 *
 * The functions reads the contents of the Miscellaneous Status Register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	A 32-bit value representing the contents of the Miscellaneous Status Register.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint32t xadc_get_misc_status(xadc_dev_t *dev)
{
	/*
	 * Read the Miscellaneous Status Register and return the value.
	 */
	return in32(dev->regbase + XZYNQ_XADC_MSTS_OFFSET);
}

/****************************************************************************/
/**
 *
 * The functions sets the contents of the Miscellaneous Control register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	data is the 32 bit data to be written to the Register.
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_set_misc_ctrl_register(xadc_dev_t *dev, _Uint32t data)
{
	/*
	 * Write to the Miscellaneous control register Register.
	 */
	out32(dev->regbase + XZYNQ_XADC_MCTL_OFFSET, data);
}

/****************************************************************************/
/**
 *
 * The functions reads the contents of the Miscellaneous control register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	A 32-bit value representing the contents of the Config Register.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint32t xadc_get_misc_ctrl_register(xadc_dev_t *dev)
{
	/*
	 * Read the Miscellaneous control register and return the value.
	 */
	return in32(dev->regbase + XZYNQ_XADC_MCTL_OFFSET);
}

/****************************************************************************/
/**
 *
 * Get the ADC converted data for the specified channel.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	channel is the channel number. Use the XADC_CH_* defined in the file xadc.h.
 *		The valid channels are
 *		- 0 to 6
 *		- 13 to 31
 *
 * @return	A 16-bit value representing the ADC converted data for the
 *		specified channel. The XADC Monitor/ADC device guarantees
 * 		a 10 bit resolution for the ADC converted data and data is the
 *		10 MSB bits of the 16 data read from the device.
 *
 * @note		The channels 7,8,9 are used for calibration of the device and
 *		hence there is no associated data with this channel.
 *
 *****************************************************************************/
_Uint16t xadc_get_adc_data(xadc_dev_t *dev, _Uint8t channel)
{
	_Uint32t reg_data;

	reg_data = xadc_read_internal_reg(dev, (XZYNQ_XADC_TEMP_OFFSET + channel));
	return (_Uint16t) reg_data;
}

/****************************************************************************/
/**
 *
 * This function gets the calibration coefficient data for the specified
 * parameter.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	coeff_type specifies the calibration coefficient
 *		to be read. Use XADC_CALIB_* constants defined in xadc.h to
 *		specify the calibration coefficient to be read.
 *
 * @return	A 16-bit value representing the calibration coefficient.
 *		The XADC device guarantees a 10 bit resolution for
 *		the ADC converted data and data is the 10 MSB bits of the 16
 *		data read from the device.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint16t xadc_get_calib_coefficient(xadc_dev_t *dev, _Uint8t coeff_type)
{
	_Uint32t reg_data;

	/*
	 * Read the selected calibration coefficient.
	 */
	reg_data = xadc_read_internal_reg(dev,
			(XZYNQ_XADC_ADC_A_SUPPLY_CALIB_OFFSET + coeff_type));
	return (_Uint16t) reg_data;
}

/****************************************************************************/
/**
 *
 * This function reads the Minimum/Maximum measurement for one of the
 * specified parameters. Use XADC_MAX_* and XADC_MIN_* constants defined in
 * xadc.h to specify the parameters (Temperature, VccInt, VccAux, VBram,
 * VccPInt, VccPAux and VccPDro).
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	measurement_type specifies the parameter for which the
 *		Minimum/Maximum measurement has to be read.
 *		Use XADC_MAX_* and XADC_MIN_* constants defined in xadc.h to
 *		specify the data to be read.
 *
 * @return	A 16-bit value representing the maximum/minimum measurement for
 *		specified parameter.
 *		The XADC device guarantees a 10 bit resolution for
 *		the ADC converted data and data is the 10 MSB bits of the 16
 *		data read from the device.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint16t xadc_get_min_max_measurement(xadc_dev_t *dev, _Uint8t measurement_type)
{
	_Uint32t reg_data;
	/*
	 * Read and return the specified Minimum/Maximum measurement.
	 */
	reg_data = xadc_read_internal_reg(dev, (XZYNQ_XADC_MAX_TEMP_OFFSET
			+ measurement_type));
	return (_Uint16t) reg_data;
}

/****************************************************************************/
/**
 *
 * This function sets the number of samples of averaging that is to be done for
 * all the channels in both the single channel mode and sequence mode of
 * operations.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	average is the number of samples of averaging programmed to the
 *		Configuration Register 0. Use the XADC_AVG_* definitions defined
 *		in xadc.h file :
 *		- XADC_AVG_0_SAMPLES for no averaging
 *		- XADC_AVG_16_SAMPLES for 16 samples of averaging
 *		- XADC_AVG_64_SAMPLES for 64 samples of averaging
 *		- XADC_AVG_256_SAMPLES for 256 samples of averaging
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_set_avg(xadc_dev_t *dev, _Uint8t average)
{
	_Uint32t reg_data;

	/*
	 * Write the averaging value into the Configuration Register 0.
	 */
	reg_data = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET)
			& (~XZYNQ_XADC_CFR0_AVG_VALID_MASK);

	reg_data |= (((_Uint32t) average << XZYNQ_XADC_CFR0_AVG_SHIFT));
	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET, reg_data);

}

/****************************************************************************/
/**
 *
 * This function returns the number of samples of averaging configured for all
 * the channels in the Configuration Register 0.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	The averaging read from the Configuration Register 0 is
 *		returned. Use the XADC_AVG_* bit definitions defined in
 *		xadc.h file to interpret the returned value :
 *		- XADC_AVG_0_SAMPLES means no averaging
 *		- XADC_AVG_16_SAMPLES means 16 samples of averaging
 *		- XADC_AVG_64_SAMPLES means 64 samples of averaging
 *		- XADC_AVG_256_SAMPLES means 256 samples of averaging
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint8t xadc_get_avg(xadc_dev_t *dev)
{
	_Uint32t average;

	/*
	 * Read the averaging value from the Configuration Register 0.
	 */
	average = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET)
			& XZYNQ_XADC_CFR0_AVG_VALID_MASK;

	return ((_Uint8t) (average >> XZYNQ_XADC_CFR0_AVG_SHIFT));
}

/****************************************************************************/
/**
 *
 * The function sets the given parameters in the Configuration Register 0 in
 * the single channel mode.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	channel is the channel number for the singel channel mode.
 *		The valid channels are 0 to 5, 8, and 16 to 31.
 *		If the external Mux is used then this specifies the channel
 *		connected to the external Mux. Please read the Device Spec
 *		to know which channels are valid.
 * @param 	increase_acq_cycles is a boolean parameter which specifies whether
 *		the Acquisition time for the external channels has to be
 *		increased to 10 ADCCLK cycles (specify true) or remain at the
 *		default 4 ADCCLK cycles (specify false). This parameter is
 *		only valid for the external channels.
 * @param 	is_differential_mode is a boolean parameter which specifies
 *		unipolar(specify false) or differential mode (specify true) for
 *		the analog inputs. The 	input mode is only valid for the
 *		external channels.
 *
 * @return
 *		- XADC_XST_SUCCESS if the given values were written successfully to
 *		the Configuration Register 0.
 *		- XADC_XST_FAILURE if the channel sequencer is enabled or the input
 *		parameters are not valid for the selected channel.
 *
 * @note
 *		- The number of samples for the averaging for all the channels
 *		is set by using the function xadc_set_avg.
 *		- The calibration of the device is done by doing a ADC
 *		conversion on the calibration channel(channel 8). The input
 *		parameters increase_acq_cycles, is_differential_mode and
 *		is_event_mode are not valid for this channel
 *
 *
 *****************************************************************************/
int xadc_set_single_ch_params(xadc_dev_t *dev, _Uint8t channel,
		int increase_acq_cycles, int is_event_mode, int is_differential_mode)
{
	_Uint32t reg_value;

	/*
	 * Check if the device is in single channel mode else return failure
	 */
	if ((xadc_get_sequencer_mode(dev) != XADC_SEQ_MODE_SINGCHAN)) {
		return XZYNQ_FAILURE;
	}

	/*
	 * Read the Configuration Register 0.
	 */
	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET)
			& XZYNQ_XADC_CFR0_AVG_VALID_MASK;

	/*
	 * Select the number of acquisition cycles. The acquisition cycles is
	 * only valid for the external channels.
	 */
	if (increase_acq_cycles == true) {
		if (((channel >= XADC_CH_AUX_MIN) && (channel <= XADC_CH_AUX_MAX))
				|| (channel == XADC_CH_VPVN)) {
			reg_value |= XZYNQ_XADC_CFR0_ACQ_MASK;
		} else {
			return XZYNQ_FAILURE;
		}

	}

	/*
	 * Select the input mode. The input mode is only valid for the
	 * external channels.
	 */
	if (is_differential_mode == true) {

		if (((channel >= XADC_CH_AUX_MIN) && (channel <= XADC_CH_AUX_MAX))
				|| (channel == XADC_CH_VPVN)) {
			reg_value |= XZYNQ_XADC_CFR0_DU_MASK;
		} else {
			return XZYNQ_FAILURE;
		}
	}

	/*
	 * Select the ADC mode.
	 */
	if (is_event_mode == true) {
		reg_value |= XZYNQ_XADC_CFR0_EC_MASK;
	}

	/*
	 * Write the given values into the Configuration Register 0.
	 */
	reg_value |= (channel & XZYNQ_XADC_CFR0_CHANNEL_MASK);
	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET, reg_value);

	return XZYNQ_SUCCESS;
}

/****************************************************************************/
/**
 *
 * This function enables the alarm outputs for the specified alarms in the
 * Configuration Register 1.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	alm_enable_mask is the bit-mask of the alarm outputs to be enabled
 *		in the Configuration Register 1.
 *		Bit positions of 1 will be enabled. Bit positions of 0 will be
 *		disabled. This mask is formed by OR'ing XADC_CFR1_ALM_*_MASK and
 *		XADC_CFR1_OT_MASK masks defined in xadc.h.
 *
 * @return	None.
 *
 * @note		The implementation of the alarm enables in the Configuration
 *		register 1 is such that the alarms for bit positions of 1 will
 *		be disabled and alarms for bit positions of 0 will be enabled.
 *		The alarm outputs specified by the alm_enable_mask are negated
 *		before writing to the Configuration Register 1.
 *
 *
 *****************************************************************************/
void xadc_set_alarm_enables(xadc_dev_t *dev, _Uint16t alm_enable_mask)
{
	_Uint32t reg_value;

	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET);

	reg_value &= (_Uint32t) ~XZYNQ_XADC_CFR1_ALM_ALL_MASK;
	reg_value |= (~alm_enable_mask & XZYNQ_XADC_CFR1_ALM_ALL_MASK);

	/*
	 * Enable/disables the alarm enables for the specified alarm bits in the
	 * Configuration Register 1.
	 */
	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET, reg_value);
}

/****************************************************************************/
/**
 *
 * This function gets the status of the alarm output enables in the
 * Configuration Register 1.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	This the bit-mask of the enabled alarm outputs in the
 *		Configuration Register 1. Use the masks XADC_CFR1_ALM*_* and
 *		XADC_CFR1_OT_MASK defined in xadc.h to interpret the
 *		returned value.
 *		Bit positions of 1 indicate that the alarm output is enabled.
 *		Bit positions of 0 indicate that the alarm output is disabled.
 *
 *
 * @note		The implementation of the alarm enables in the Configuration
 *		register 1 is such that alarms for the bit positions of 1 will
 *		be disabled and alarms for bit positions of 0 will be enabled.
 *		The enabled alarm outputs returned by this function is the
 *		negated value of the the data read from the Configuration
 *		Register 1.
 *
 *****************************************************************************/
_Uint16t xadc_get_alarm_enables(xadc_dev_t *dev)
{
	_Uint32t reg_value;

	/*
	 * Read the status of alarm output enables from the Configuration
	 * Register 1.
	 */
	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET)
			& XZYNQ_XADC_CFR1_ALM_ALL_MASK;
	return (_Uint16t) (~reg_value & XZYNQ_XADC_CFR1_ALM_ALL_MASK);
}

/****************************************************************************/
/**
 *
 * This function enables the specified calibration in the Configuration
 * Register 1 :
 *
 * - XADC_CFR1_CAL_ADC_OFFSET_MASK : calibration 0 -ADC offset correction
 * - XADC_CFR1_CAL_ADC_GAIN_OFFSET_MASK : calibration 1 -ADC gain and offset correction
 * - XADC_CFR1_CAL_PS_OFFSET_MASK : calibration 2 -Power Supply sensor offset correction
 * - XADC_CFR1_CAL_PS_GAIN_OFFSET_MASK : calibration 3 -Power Supply sensor gain and offset correction
 * - XADC_CFR1_CAL_DISABLE_MASK : No calibration
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	calibration is the calibration to be applied.
 *		Use XADC_CFR1_CAL*_* bits defined in xadc.h.
 *		Multiple calibrations can be enabled at a time by oring the
 *		XADC_CFR1_CAL_ADC_* and XADC_CFR1_CAL_PS_* bits.
 *		calibration can be disabled by specifying
 XADC_CFR1_CAL_DISABLE_MASK;
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_set_calib_enables(xadc_dev_t *dev, _Uint16t calibration)
{
	_Uint32t reg_value;

	/*
	 * Set the specified calibration in the Configuration Register 1.
	 */
	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET);

	reg_value &= (~XZYNQ_XADC_CFR1_CAL_VALID_MASK);
	reg_value |= (calibration & XZYNQ_XADC_CFR1_CAL_VALID_MASK);
	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET, reg_value);

}

/****************************************************************************/
/**
 *
 * This function reads the value of the calibration enables from the
 * Configuration Register 1.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	The value of the calibration enables in the Configuration
 *		Register 1 :
 *		- XADC_CFR1_CAL_ADC_OFFSET_MASK : ADC offset correction
 *		- XADC_CFR1_CAL_ADC_GAIN_OFFSET_MASK : ADC gain and offset
 *				correction
 *		- XADC_CFR1_CAL_PS_OFFSET_MASK : Power Supply sensor offset
 *				correction
 *		- XADC_CFR1_CAL_PS_GAIN_OFFSET_MASK : Power Supply sensor
 *				gain and offset correction
 *		- XADC_CFR1_CAL_DISABLE_MASK : No calibration
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint16t xadc_get_calib_enables(xadc_dev_t *dev)
{
	/*
	 * Read the calibration enables from the Configuration Register 1.
	 */
	return (_Uint16t) xadc_read_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET)
			& XZYNQ_XADC_CFR1_CAL_VALID_MASK;

}

/****************************************************************************/
/**
 *
 * This function sets the specified channel Sequencer mode in the Configuration
 * Register 1 :
 *		- Default safe mode (XADC_SEQ_MODE_SAFE)
 *		- One pass through sequence (XADC_SEQ_MODE_ONEPASS)
 *		- Continuous channel sequencing (XADC_SEQ_MODE_CONTINPASS)
 *		- Single channel/Sequencer off (XADC_SEQ_MODE_SINGCHAN)
 *		- Simultaneous sampling mode (XADC_SEQ_MODE_SIMUL_SAMPLING)
 *		- Independent mode (XADC_SEQ_MODE_INDEPENDENT)
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	sequencer_mode is the sequencer mode to be set.
 *		Use XADC_SEQ_MODE_* bits defined in xadc.h.
 * @return	None.
 *
 * @note		Only one of the modes can be enabled at a time. Please
 *		read the Spec of the XADC for further information about the
 *		sequencer modes.
 *
 *
 *****************************************************************************/
void xadc_set_sequencer_mode(xadc_dev_t *dev, _Uint8t sequencer_mode)
{
	_Uint32t reg_value;

	/*
	 * Set the specified sequencer mode in the Configuration Register 1.
	 */
	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET);
	reg_value &= (~XZYNQ_XADC_CFR1_SEQ_VALID_MASK);
	reg_value |= ((sequencer_mode << XZYNQ_XADC_CFR1_SEQ_SHIFT)
			& XZYNQ_XADC_CFR1_SEQ_VALID_MASK);
	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET, reg_value);

}

/****************************************************************************/
/**
 *
 * This function gets the channel sequencer mode from the Configuration
 * Register 1.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	The channel sequencer mode :
 *		- XADC_SEQ_MODE_SAFE : Default safe mode
 *		- XADC_SEQ_MODE_ONEPASS : One pass through sequence
 *		- XADC_SEQ_MODE_CONTINPASS : Continuous channel sequencing
 *		- XADC_SEQ_MODE_SINGCHAN : Single channel/Sequencer off
 *		- XADC_SEQ_MODE_SIMUL_SAMPLING : Simulataneous sampling mode
 *		- XADC_SEQ_MODE_INDEPENDENT : Independent mode
 *
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint8t xadc_get_sequencer_mode(xadc_dev_t *dev)
{
	/*
	 * Read the channel sequencer mode from the Configuration Register 1.
	 */
	return ((_Uint8t) ((xadc_read_internal_reg(dev, XZYNQ_XADC_CFR1_OFFSET)
			& XZYNQ_XADC_CFR1_SEQ_VALID_MASK) >> XZYNQ_XADC_CFR1_SEQ_SHIFT));

}

/****************************************************************************/
/**
 *
 * The function sets the frequency of the ADCCLK by configuring the DCLK to
 * ADCCLK ratio in the Configuration Register #2
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	divisor is clock divisor used to derive ADCCLK from DCLK.
 *		Valid values of the divisor are
 *		 - 0 to 255. values 0, 1, 2 are all mapped to 2.
 *		Refer to the device specification for more details
 *
 * @return	None.
 *
 * @note		- The ADCCLK is an internal clock used by the ADC and is
 *		  synchronized to the DCLK clock. The ADCCLK is equal to DCLK
 *		  divided by the user selection in the Configuration Register 2.
 *		- There is no Assert on the minimum value of the divisor.
 *
 *****************************************************************************/
void xadc_set_adc_clk_divisor(xadc_dev_t *dev, _Uint8t divisor)
{
	/*
	 * Write the divisor value into the Configuration Register #2.
	 */
	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR2_OFFSET, divisor
			<< XZYNQ_XADC_CFR2_CD_SHIFT);

}

/****************************************************************************/
/**
 *
 * The function gets the ADCCLK divisor from the Configuration Register 2.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	The divisor read from the Configuration Register 2.
 *
 * @note		The ADCCLK is an internal clock used by the ADC and is
 *		synchronized to the DCLK clock. The ADCCLK is equal to DCLK
 *		divided by the user selection in the Configuration Register 2.
 *
 *****************************************************************************/
_Uint8t xadc_get_adc_clk_divisor(xadc_dev_t *dev)
{
	_Uint16t divisor;

	/*
	 * Read the divisor value from the Configuration Register 2.
	 */
	divisor = (_Uint16t) xadc_read_internal_reg(dev, XZYNQ_XADC_CFR2_OFFSET);

	return (_Uint8t) (divisor >> XZYNQ_XADC_CFR2_CD_SHIFT);
}

/****************************************************************************/
/**
 *
 * This function enables the specified channels in the ADC channel Selection
 * Sequencer Registers. The sequencer must be disabled before writing to these
 * regsiters.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	ch_enable_mask is the bit mask of all the channels to be enabled.
 *		Use XADC_SEQ_CH__* defined in xadc.h to specify the channel
 *		numbers. Bit masks of 1 will be enabled and bit mask of 0 will
 *		be disabled.
 *		The ch_enable_mask is a 32 bit mask that is written to the two
 *		16 bit ADC channel Selection Sequencer Registers.
 *
 * @return
 *		- XADC_XST_SUCCESS if the given values were written successfully to
 *		the ADC channel Selection Sequencer Registers.
 *		- XADC_XST_FAILURE if the channel sequencer is enabled.
 *
 * @note		None
 *
 *****************************************************************************/
int xadc_set_seq_ch_enables(xadc_dev_t *dev, _Uint32t ch_enable_mask)
{
	/*
	 * The sequencer must be disabled for writing any of these registers
	 * Return XADC_XST_FAILURE if the channel sequencer is enabled.
	 */
	if ((xadc_get_sequencer_mode(dev) != XADC_SEQ_MODE_SINGCHAN)) {
		return XZYNQ_FAILURE;
	}

	/*
	 * Enable the specified channels in the ADC channel Selection Sequencer
	 * Registers.
	 */
	xadc_write_internal_reg(dev, XZYNQ_XADC_SEQ00_OFFSET, (ch_enable_mask
			& XZYNQ_XADC_SEQ00_CH_VALID_MASK));

	xadc_write_internal_reg(dev, XZYNQ_XADC_SEQ01_OFFSET, (ch_enable_mask
			>> XZYNQ_XADC_SEQ_CH_AUX_SHIFT) & XZYNQ_XADC_SEQ01_CH_VALID_MASK);

	return XZYNQ_SUCCESS;
}

/****************************************************************************/
/**
 *
 * This function gets the channel enable bits status from the ADC channel
 * Selection Sequencer Registers.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	Gets the channel enable bits. Use XADC_SEQ_CH__* defined in
 *		xadc.h to interpret the channel numbers. Bit masks of 1
 *		are the channels that are enabled and bit mask of 0 are
 *		the channels that are disabled.
 *
 * @return	None
 *
 * @note		None
 *
 *****************************************************************************/
_Uint32t xadc_get_seq_ch_enables(xadc_dev_t *dev)
{
	_Uint32t reg_val_enable;

	/*
	 *  Read the channel enable bits for all the channels from the ADC
	 *  channel Selection Register.
	 */
	reg_val_enable = xadc_read_internal_reg(dev, XZYNQ_XADC_SEQ00_OFFSET)
			& XZYNQ_XADC_SEQ00_CH_VALID_MASK;
	reg_val_enable |= (xadc_read_internal_reg(dev, XZYNQ_XADC_SEQ01_OFFSET)
			& XZYNQ_XADC_SEQ01_CH_VALID_MASK) << XZYNQ_XADC_SEQ_CH_AUX_SHIFT;

	return reg_val_enable;
}

/****************************************************************************/
/**
 *
 * This function enables the averaging for the specified channels in the ADC
 * channel Averaging Enable Sequencer Registers. The sequencer must be disabled
 * before writing to these regsiters.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	avg_enable_ch_mask is the bit mask of all the channels for which
 *		averaging is to be enabled. Use XADC_SEQ_CH__* defined in
 *		xadc.h to specify the channel numbers. Averaging will be
 *		enabled for bit masks of 1 and disabled for bit mask of 0.
 *		The avg_enable_ch_mask is a 32 bit mask that is written to the two
 *		16 bit ADC channel Averaging Enable Sequencer Registers.
 *
 * @return
 *		- XADC_XST_SUCCESS if the given values were written successfully to
 *		the ADC channel Averaging Enables Sequencer Registers.
 *		- XADC_XST_FAILURE if the channel sequencer is enabled.
 *
 * @note		None
 *
 *****************************************************************************/
int xadc_set_seq_avg_enables(xadc_dev_t *dev, _Uint32t avg_enable_ch_mask)
{
	/*
	 * The sequencer must be disabled for writing any of these registers
	 * Return XADC_XST_FAILURE if the channel sequencer is enabled.
	 */
	if ((xadc_get_sequencer_mode(dev) != XADC_SEQ_MODE_SINGCHAN)) {
		return XZYNQ_FAILURE;
	}

	/*
	 * Enable/disable the averaging for the specified channels in the
	 * ADC channel Averaging Enables Sequencer Registers.
	 */
	xadc_write_internal_reg(dev, XZYNQ_XADC_SEQ02_OFFSET, (avg_enable_ch_mask
			& XZYNQ_XADC_SEQ02_CH_VALID_MASK));

	xadc_write_internal_reg(dev, XZYNQ_XADC_SEQ03_OFFSET, (avg_enable_ch_mask
			>> XZYNQ_XADC_SEQ_CH_AUX_SHIFT) & XZYNQ_XADC_SEQ03_CH_VALID_MASK);

	return XZYNQ_SUCCESS;
}

/****************************************************************************/
/**
 *
 * This function returns the channels for which the averaging has been enabled
 * in the ADC channel Averaging Enables Sequencer Registers.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @returns 	The status of averaging (enabled/disabled) for all the channels.
 *		Use XADC_SEQ_CH__* defined in xadc.h to interpret the
 *		channel numbers. Bit masks of 1 are the channels for which
 *		averaging is enabled and bit mask of 0 are the channels for
 *		averaging is disabled
 *
 * @note		None
 *
 *****************************************************************************/
_Uint32t xadc_get_seq_avg_enables(xadc_dev_t *dev)
{
	_Uint32t reg_val_avg;

	/*
	 * Read the averaging enable status for all the channels from the
	 * ADC channel Averaging Enables Sequencer Registers.
	 */
	reg_val_avg = xadc_read_internal_reg(dev, XZYNQ_XADC_SEQ02_OFFSET)
			& XZYNQ_XADC_SEQ02_CH_VALID_MASK;
	reg_val_avg |= (xadc_read_internal_reg(dev, XZYNQ_XADC_SEQ03_OFFSET)
			& XZYNQ_XADC_SEQ03_CH_VALID_MASK) << XZYNQ_XADC_SEQ_CH_AUX_SHIFT;

	return reg_val_avg;
}

/****************************************************************************/
/**
 *
 * This function sets the Analog input mode for the specified channels in the ADC
 * channel Analog-Input mode Sequencer Registers. The sequencer must be disabled
 * before writing to these regsiters.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	input_mode_ch_mask is the bit mask of all the channels for which
 *		the input mode is differential mode. Use XADC_SEQ_CH__* defined
 *		in xadc.h to specify the channel numbers. Differential
 *		input mode will be set for bit masks of 1 and unipolar input
 *		mode for bit masks of 0.
 *		The input_mode_ch_mask is a 32 bit mask that is written to the two
 *		16 bit ADC channel Analog-Input mode Sequencer Registers.
 *
 * @return
 *		- XADC_XST_SUCCESS if the given values were written successfully to
 *		the ADC channel Analog-Input mode Sequencer Registers.
 *		- XADC_XST_FAILURE if the channel sequencer is enabled.
 *
 * @note		None
 *
 *****************************************************************************/
int xadc_set_seq_input_mode(xadc_dev_t *dev, _Uint32t input_mode_ch_mask)
{
	/*
	 * The sequencer must be disabled for writing any of these registers
	 * Return XADC_XST_FAILURE if the channel sequencer is enabled.
	 */
	if ((xadc_get_sequencer_mode(dev) != XADC_SEQ_MODE_SINGCHAN)) {
		return XZYNQ_FAILURE;
	}

	/*
	 * Set the input mode for the specified channels in the ADC channel
	 * Analog-Input mode Sequencer Registers.
	 */
	xadc_write_internal_reg(dev, XZYNQ_XADC_SEQ04_OFFSET, (input_mode_ch_mask
			& XZYNQ_XADC_SEQ04_CH_VALID_MASK));

	xadc_write_internal_reg(dev, XZYNQ_XADC_SEQ05_OFFSET, (input_mode_ch_mask
			>> XZYNQ_XADC_SEQ_CH_AUX_SHIFT) & XZYNQ_XADC_SEQ05_CH_VALID_MASK);

	return XZYNQ_SUCCESS;
}

/****************************************************************************/
/**
 *
 * This function gets the Analog input mode for all the channels from
 * the ADC channel Analog-Input mode Sequencer Registers.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @returns 	The input mode for all the channels.
 *		Use XADC_SEQ_CH_* defined in xadc.h to interpret the
 *		channel numbers. Bit masks of 1 are the channels for which
 *		input mode is differential and bit mask of 0 are the channels
 *		for which input mode is unipolar.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint32t xadc_get_seq_input_mode(xadc_dev_t *dev)
{
	_Uint32t input_mode;

	/*
	 *  Get the input mode for all the channels from the ADC channel
	 * Analog-Input mode Sequencer Registers.
	 */
	input_mode = xadc_read_internal_reg(dev, XZYNQ_XADC_SEQ04_OFFSET)
			& XZYNQ_XADC_SEQ04_CH_VALID_MASK;
	input_mode |= (xadc_read_internal_reg(dev, XZYNQ_XADC_SEQ05_OFFSET)
			& XZYNQ_XADC_SEQ05_CH_VALID_MASK) << XZYNQ_XADC_SEQ_CH_AUX_SHIFT;

	return input_mode;
}

/****************************************************************************/
/**
 *
 * This function sets the number of Acquisition cycles in the ADC channel
 * Acquisition Time Sequencer Registers. The sequencer must be disabled
 * before writing to these regsiters.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	acq_cycles_ch_mask is the bit mask of all the channels for which
 *		the number of acquisition cycles is to be extended.
 *		Use XADC_SEQ_CH__* defined in xadc.h to specify the channel
 *		numbers. Acquisition cycles will be extended to 10 ADCCLK cycles
 *		for bit masks of 1 and will be the default 4 ADCCLK cycles for
 *		bit masks of 0.
 *		The acq_cycles_ch_mask is a 32 bit mask that is written to the two
 *		16 bit ADC channel Acquisition Time Sequencer Registers.
 *
 * @return
 *		- XADC_XST_SUCCESS if the given values were written successfully to
 *		the channel Sequencer Registers.
 *		- XADC_XST_FAILURE if the channel sequencer is enabled.
 *
 * @note		None.
 *
 *****************************************************************************/
int xadc_set_seq_acq_time(xadc_dev_t *dev, _Uint32t acq_cycles_ch_mask)
{
	/*
	 * The sequencer must be disabled for writing any of these registers
	 * Return XADC_XST_FAILURE if the channel sequencer is enabled.
	 */
	if ((xadc_get_sequencer_mode(dev) != XADC_SEQ_MODE_SINGCHAN)) {
		return XZYNQ_FAILURE;
	}

	/*
	 * Set the Acquisition time for the specified channels in the
	 * ADC channel Acquisition Time Sequencer Registers.
	 */
	xadc_write_internal_reg(dev, XZYNQ_XADC_SEQ06_OFFSET, (acq_cycles_ch_mask
			& XZYNQ_XADC_SEQ06_CH_VALID_MASK));

	xadc_write_internal_reg(dev, XZYNQ_XADC_SEQ07_OFFSET, (acq_cycles_ch_mask
			>> XZYNQ_XADC_SEQ_CH_AUX_SHIFT) & XZYNQ_XADC_SEQ07_CH_VALID_MASK);

	return XZYNQ_SUCCESS;
}

/****************************************************************************/
/**
 *
 * This function gets the status of acquisition from the ADC channel Acquisition
 * Time Sequencer Registers.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @returns 	The acquisition time for all the channels.
 *		Use XADC_SEQ_CH__* defined in xadc.h to interpret the
 *		channel numbers. Bit masks of 1 are the channels for which
 *		acquisition cycles are extended and bit mask of 0 are the
 *		channels for which acquisition cycles are not extended.
 *
 * @note		None
 *
 *****************************************************************************/
_Uint32t xadc_get_seq_acq_time(xadc_dev_t *dev)
{
	_Uint32t reg_val_acq;

	/*
	 * Get the Acquisition cycles for the specified channels from the ADC
	 * channel Acquisition Time Sequencer Registers.
	 */
	reg_val_acq = xadc_read_internal_reg(dev, XZYNQ_XADC_SEQ06_OFFSET)
			& XZYNQ_XADC_SEQ06_CH_VALID_MASK;
	reg_val_acq |= (xadc_read_internal_reg(dev, XZYNQ_XADC_SEQ07_OFFSET)
			& XZYNQ_XADC_SEQ07_CH_VALID_MASK) << XZYNQ_XADC_SEQ_CH_AUX_SHIFT;

	return reg_val_acq;
}

/****************************************************************************/
/**
 *
 * This functions sets the contents of the given Alarm Threshold Register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	alarm_thr_reg is the index of an Alarm Threshold Register to
 *		be set. Use XADC_ATR_* constants defined in xadc.h to
 *		specify the index.
 * @param	value is the 16-bit threshold value to write into the register.
 *
 * @return	None.
 *
 * @note		Use xadc_set_over_temp() to set the Over Temperature upper
 *		threshold value.
 *
 *****************************************************************************/
void xadc_set_alarm_threshold(xadc_dev_t *dev, _Uint8t alarm_thr_reg,
		_Uint16t value)
{
	/*
	 * Write the value into the specified Alarm Threshold Register.
	 */
	xadc_write_internal_reg(dev, XZYNQ_XADC_ATR_TEMP_UPPER_OFFSET
			+ alarm_thr_reg, value);

}

/****************************************************************************/
/**
 *
 * This function returns the contents of the specified Alarm Threshold Register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	alarm_thr_reg is the index of an Alarm Threshold Register
 *		to be read. Use XADC_ATR_* constants defined in 	xadc.h
 *		to specify the index.
 *
 * @return	A 16-bit value representing the contents of the selected Alarm
 *		Threshold Register.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint16t xadc_get_alarm_threshold(xadc_dev_t *dev, _Uint8t alarm_thr_reg)
{
	_Uint32t reg_data;

	/*
	 * Read the specified Alarm Threshold Register and return
	 * the value
	 */
	reg_data = xadc_read_internal_reg(dev, (XZYNQ_XADC_ATR_TEMP_UPPER_OFFSET
			+ alarm_thr_reg));

	return (_Uint16t) reg_data;
}

/****************************************************************************/
/**
 *
 * This function enables programming of the powerdown temperature for the
 * OverTemp signal in the OT Powerdown register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_enable_user_over_temp(xadc_dev_t *dev)
{
	_Uint16t ot_upper;

	/*
	 * Read the OT upper Alarm Threshold Register.
	 */
	ot_upper = xadc_read_internal_reg(dev, XZYNQ_XADC_ATR_OT_UPPER_OFFSET);
	ot_upper &= ~(XZYNQ_XADC_ATR_OT_UPPER_ENB_MASK);

	/*
	 * Preserve the powerdown value and write OT enable value the into the
	 * OT Upper Alarm Threshold Register.
	 */
	ot_upper |= XZYNQ_XADC_ATR_OT_UPPER_ENB_VAL;
	xadc_write_internal_reg(dev, XZYNQ_XADC_ATR_OT_UPPER_OFFSET, ot_upper);
}

/****************************************************************************/
/**
 *
 * This function disables programming of the powerdown temperature for the
 * OverTemp signal in the OT Powerdown register.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	None.
 *
 * @note		None.
 *
 *
 *****************************************************************************/
void xadc_disable_user_over_temp(xadc_dev_t *dev)
{
	_Uint16t ot_upper;

	/*
	 * Read the OT Upper Alarm Threshold Register.
	 */
	ot_upper = xadc_read_internal_reg(dev, XZYNQ_XADC_ATR_OT_UPPER_OFFSET);
	ot_upper &= ~(XZYNQ_XADC_ATR_OT_UPPER_ENB_MASK);

	xadc_write_internal_reg(dev, XZYNQ_XADC_ATR_OT_UPPER_OFFSET, ot_upper);
}

/****************************************************************************/
/**
 *
 * The function enables the Event mode or Continuous mode in the sequencer mode.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	is_event_mode is a boolean parameter that specifies continuous
 *		sampling (specify false) or event driven sampling mode (specify
 *		true) for the given channel.
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_set_sequencer_event(xadc_dev_t *dev, int is_event_mode)
{
	_Uint32t reg_value;

	/*
	 * Read the Configuration Register 0.
	 */
	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET)
			& (~XZYNQ_XADC_CFR0_EC_MASK);

	/*
	 * Set the ADC mode.
	 */
	if (is_event_mode == true) {
		reg_value |= XZYNQ_XADC_CFR0_EC_MASK;
	} else {
		reg_value &= ~XZYNQ_XADC_CFR0_EC_MASK;
	}

	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET, reg_value);
}

/****************************************************************************/
/**
 *
 * This function returns the sampling mode.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	The sampling mode
 *		- 0 specifies continuous sampling
 *		- 1 specifies event driven sampling mode
 *
 * @note		None.
 *
 *****************************************************************************/
int xadc_get_sampling_mode(xadc_dev_t *dev)
{
	_Uint32t mode;

	/*
	 * Read the sampling mode from the Configuration Register 0.
	 */
	mode = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET)
			& XZYNQ_XADC_CFR0_EC_MASK;
	if (mode) {

		return 1;
	}

	return (0);
}

/****************************************************************************/
/**
 *
 * This function sets the External Mux mode.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param 	mux_mode specifies whether External Mux is used
 *		- false specifies NO external MUX
 *		- true specifies External Mux is used
 * @param	channel specifies the channel to be used for the
 *		external Mux. Please read the Device Spec for which
 *		channels are valid for which mode.
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_set_mux_mode(xadc_dev_t *dev, int mux_mode, _Uint8t channel)
{
	_Uint32t reg_value;

	/*
	 * Read the Configuration Register 0.
	 */
	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET)
			& (~XZYNQ_XADC_CFR0_MUX_MASK);

	/*
	 * Select the Mux mode and the channel to be used.
	 */
	if (mux_mode == true) {
		reg_value |= XZYNQ_XADC_CFR0_MUX_MASK;
		reg_value |= (channel & XZYNQ_XADC_CFR0_CHANNEL_MASK);

	}

	/*
	 * Write the mux mode into the Configuration Register 0.
	 */
	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR0_OFFSET, reg_value);
}

/****************************************************************************/
/**
 *
 * This function sets the Power Down mode.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param 	mode specifies the Power Down mode
 *		- XADC_PD_MODE_NONE specifies NO Power Down (Both ADC A and
 *		ADC B are enabled)
 *		- XADC_PD_MODE_ADCB specfies the Power Down of ADC B
 *		- XADC_PD_MODE_XADC specifies the Power Down of
 *		both ADC A and ADC B.
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_set_power_down_mode(xadc_dev_t *dev, _Uint32t mode)
{
	_Uint32t reg_value;

	/*
	 * Read the Configuration Register 2.
	 */
	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR2_OFFSET)
			& (~XZYNQ_XADC_CFR2_PD_MASK);

	/*
	 * Select the Power Down mode.
	 */
	reg_value |= (mode << XZYNQ_XADC_CFR2_PD_SHIFT);

	xadc_write_internal_reg(dev, XZYNQ_XADC_CFR2_OFFSET, reg_value);
}

/****************************************************************************/
/**
 *
 * This function gets the Power Down mode.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	mode specifies the Power Down mode
 *		- XADC_PD_MODE_NONE specifies NO Power Down (Both ADC A and
 *		ADC B are enabled)
 *		- XADC_PD_MODE_ADCB specifies the Power Down of ADC B
 *		- XADC_PD_MODE_XADC specifies the Power Down of
 *		both ADC A and ADC B.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint32t xadc_get_power_down_mode(xadc_dev_t *dev)
{
	_Uint32t reg_value;

	/*
	 * Read the Power Down mode.
	 */
	reg_value = xadc_read_internal_reg(dev, XZYNQ_XADC_CFR2_OFFSET)
			& (~XZYNQ_XADC_CFR2_PD_MASK);

	/*
	 * Return the Power Down mode.
	 */
	return (reg_value >> XZYNQ_XADC_CFR2_PD_SHIFT);

}

/****************************************************************************/
/**
 *
 * This function enables the specified interrupts in the device.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	Mask is the bit-mask of the interrupts to be enabled.
 *		Bit positions of 1 will be enabled. Bit positions of 0 will
 *		keep the previous setting. This mask is formed by OR'ing
 *		XADC_INTX_* bits defined in xadcps_hw.h.
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_intr_enable(xadc_dev_t *dev, _Uint32t mask)
{
	_Uint32t reg_data;

	reg_data = in32(dev->regbase + XZYNQ_XADC_INT_MASK_OFFSET);
	reg_data &= ~(mask & XZYNQ_XADC_INTX_ALL_MASK);
	out32(dev->regbase + XZYNQ_XADC_INT_MASK_OFFSET, reg_data);
}

/****************************************************************************/
/**
 *
 * This function disables the specified interrupts in the device.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	Mask is the bit-mask of the interrupts to be disabled.
 *		Bit positions of 1 will be disabled. Bit positions of 0 will
 *		keep the previous setting. This mask is formed by OR'ing
 *		XADC_INTX_* bits defined in xadcps_hw.h.
 *
 * @return	None.
 *
 * @note		None
 *
 *****************************************************************************/
void xadc_intr_disable(xadc_dev_t *dev, _Uint32t mask)
{
	_Uint32t reg_data;

	reg_data = in32(dev->regbase + XZYNQ_XADC_INT_MASK_OFFSET);
	reg_data |= (mask & XZYNQ_XADC_INTX_ALL_MASK);
	out32(dev->regbase + XZYNQ_XADC_INT_MASK_OFFSET, reg_data);
}

/****************************************************************************/
/**
 *
 * This function returns the enabled interrupts read from the Interrupt Mask
 * Register (IPIER). Use the XADC_IPIXR_* constants defined in xadcps_hw.h to
 * interpret the returned value.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	A 32-bit value representing the contents of the I.
 *
 * @note		None.
 *
 *****************************************************************************/
_Uint32t xadc_intr_get_enabled(xadc_dev_t *dev)
{
	/*
	 * Return the value read from the Interrupt Enable Register.
	 */
	return (~in32(dev->regbase + XZYNQ_XADC_INT_MASK_OFFSET)
			& XZYNQ_XADC_INTX_ALL_MASK);
}

/****************************************************************************/
/**
 *
 * This function returns the interrupt status read from Interrupt Status
 * Register(IPISR). Use the XADC_IPIXR_* constants defined in xadcps_hw.h
 * to interpret the returned value.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 *
 * @return	A 32-bit value representing the contents of the IPISR.
 *
 * @note		The device must be configured at hardware build time to include
 *		interrupt component for this function to work.
 *
 *****************************************************************************/
_Uint32t xadc_intr_get_status(xadc_dev_t *dev)
{
	return (in32(dev->regbase + XZYNQ_XADC_INT_STS_OFFSET)
			& XZYNQ_XADC_INTX_ALL_MASK);
}

/****************************************************************************/
/**
 *
 * This function clears the specified interrupts in the Interrupt Status
 * Register (IPISR).
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	Mask is the bit-mask of the interrupts to be cleared.
 *		Bit positions of 1 will be cleared. Bit positions of 0 will not
 * 		change the previous interrupt status. This mask is formed by
 * 		OR'ing XADC_IPIXR_* bits which are defined in xadcps_hw.h.
 *
 * @return	None.
 *
 * @note		None.
 *
 *****************************************************************************/
void xadc_intr_clear(xadc_dev_t *dev, _Uint32t mask)
{
	_Uint32t reg_value;

	reg_value = in32(dev->regbase + XZYNQ_XADC_INT_STS_OFFSET);
	reg_value &= (mask & XZYNQ_XADC_INTX_ALL_MASK);
	out32(dev->regbase + XZYNQ_XADC_INT_STS_OFFSET, reg_value);
}

/****************************************************************************/
/**
 *
 * This function is used for writing to XADC Registers using the command FIFO.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	offset is the offset of the XADC register to be written.
 * @param	data is the data to be written.
 *
 * @return	None.
 *
 * @note		None.
 *
 *
 *****************************************************************************/
void xadc_write_internal_reg(xadc_dev_t *dev, _Uint32t offset, _Uint32t data)
{
	_Uint32t write_data;

	/* Write the data into the FIFO register */
	write_data = XADC_FORMAT_WRITE_DATA(offset, data, true);
	XADC_WRITE_FIFO(dev->regbase, write_data);

	/* Read the read FIFO after any write since for each write one
	 * location of read FIFO gets updated
	 */
	write_data = XADC_READ_FIFO(dev->regbase);
}

/****************************************************************************/
/**
 *
 * This function is used for reading from the XADC Registers using the Data FIFO.
 *
 * @param	dev is a pointer to the xadc_dev_t instance.
 * @param	offset is the offset of the XADC register to be read.
 *
 * @return	data read from the FIFO
 *
 * @note		None.
 *
 *
 *****************************************************************************/
_Uint32t xadc_read_internal_reg(xadc_dev_t *dev, _Uint32t offset)
{
	_Uint32t read_data;

	read_data = XADC_FORMAT_WRITE_DATA(offset, 0x0, false);

	/* Read cmd to FIFO */
	XADC_WRITE_FIFO(dev->regbase, read_data);

	/* Do a dummy read */
	read_data = XADC_READ_FIFO(dev->regbase);

	/* Do a dummy write to get the actual read */
	XADC_WRITE_FIFO(dev->regbase, read_data);

	/* Do a the real read */
	read_data = XADC_READ_FIFO(dev->regbase);

	return read_data;

}

_Uint8t xadc_cmd_fifo_status(xadc_dev_t *dev)
{
	return ((xadc_get_misc_status(dev) & XZYNQ_XADC_MSTS_CFIFOF_MASK) == XZYNQ_XADC_MSTS_CFIFOF_MASK);
}

_Uint8t xadc_data_fifo_status(xadc_dev_t *dev)
{
	return ((xadc_get_misc_status(dev) & XZYNQ_XADC_MSTS_DFIFOF_MASK) == XZYNQ_XADC_MSTS_DFIFOF_MASK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/xadc/lib.c $ $Rev: 752035 $")
#endif
