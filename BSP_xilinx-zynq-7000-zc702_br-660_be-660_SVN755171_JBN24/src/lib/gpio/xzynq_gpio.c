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

#include <stdint.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <stdio.h>
#include "xzynq_gpio.h"

static uintptr_t gpio_virt_base;
static uintptr_t mio_virt_base;

int xzynq_gpio_init(void) {
	gpio_virt_base = mmap_device_io(0x1000, XZYNQ_GPIO_BASE);
	if (gpio_virt_base == (uintptr_t) MAP_FAILED) {
		perror("mmap_device_io");
		return -1;
	}

	mio_virt_base = mmap_device_io(0x1000, XZYNQ_SLCR_BASE);
	if (mio_virt_base == (uintptr_t) MAP_FAILED) {
		perror("mmap_device_io");
		return -1;
	}

	return 0;
}

void xzynq_gpio_fini(void) {
	munmap_device_io(XZYNQ_SLCR_BASE, 0x1000);
	munmap_device_io(XZYNQ_GPIO_BASE, 0x1000);
}

unsigned int xzynq_gpio_pin_table[] = {
		31, /* 0 - 31, Bank 0 */
		53, /* 32 - 53, Bank 1 */
		85, /* 54 - 85, Bank 2 */
		117 /* 86 - 117 Bank 3 */
};

static void xzynq_get_bank_pin(uint8_t pin_number, uint8_t *bank_number,
		uint8_t *pin_number_in_bank) {
	for (*bank_number = 0; *bank_number < 4; (*bank_number)++)
		if (pin_number <= xzynq_gpio_pin_table[*bank_number])
			break;

	if (*bank_number == 0) {
		*pin_number_in_bank = pin_number;
	} else {
		*pin_number_in_bank = pin_number % (xzynq_gpio_pin_table[*bank_number
				- 1] + 1);
	}
}

int xzynq_gpio_set_direction(int pin, int dir) {
	uint8_t bank, pin_number;
	uint32_t dir_mode_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	dir_mode_reg = in32(gpio_virt_base
			+ ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_DIRM_OFFSET);

	if (dir) { /* Output Direction */
		dir_mode_reg |= (1 << pin_number);
	} else { /* Input Direction */
		dir_mode_reg &= ~(1 << pin_number);
	}

	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_DIRM_OFFSET, dir_mode_reg);

	return 0;
}

int xzynq_gpio_get_direction(int pin) {
	uint8_t bank, pin_number;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	return (in32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_DIRM_OFFSET) >> pin_number) & 1;
}

int xzynq_gpio_set_output_enable(int pin, int value) {
	uint8_t bank, pin_number;
	uint32_t op_enable_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	op_enable_reg = in32(gpio_virt_base + ((bank)
			* XZYNQ_GPIOPS_REG_MASK_OFFSET) + XZYNQ_GPIOPS_OUTEN_OFFSET);

	if (value) { /*  Enable Output Enable */
		op_enable_reg |= (1 << pin_number);
	} else { /* Disable Output Enable */
		op_enable_reg &= ~(1 << pin_number);
	}

	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_OUTEN_OFFSET, op_enable_reg);

	return 0;
}

int xzynq_gpio_get_output_enable(int pin) {
	uint8_t bank, pin_number;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	return (in32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_OUTEN_OFFSET) >> pin_number) & 1;
}

int xzynq_gpio_get_input(int pin) {
	uint8_t bank, pin_number;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	return (in32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_DATA_BANK_OFFSET)
			+ XZYNQ_GPIOPS_DATA_OFFSET) >> pin_number) & 1;
}

int xzynq_gpio_set_output(int pin, int data) {
	uint8_t bank, pin_number;
	uint32_t reg_offset, value;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	if (pin_number > 15) {
		/*
		 * There are only 16 data bits in bit maskable register.
		 */
		pin_number -= 16;
		reg_offset = XZYNQ_GPIOPS_DATA_MSW_OFFSET;
	} else {
		reg_offset = XZYNQ_GPIOPS_DATA_LSW_OFFSET;
	}

	/*
	 * Get the 32 bit value to be written to the Mask/Data register where
	 * the upper 16 bits is the mask and lower 16 bits is the data.
	 */
	data &= 0x01;
	value = ~(1 << (pin_number + 16)) & ((data << pin_number) | 0xFFFF0000);

	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_DATA_MASK_OFFSET)
			+ reg_offset, value);

	return 0;
}

int xzynq_gpio_get_irq_type(int pin) {
	uint8_t bank, pin_number;
	uint32_t intr_type_reg;
	uint32_t intr_pol_reg;
	uint32_t intr_on_any_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	intr_type_reg = in32(gpio_virt_base + ((bank)
			* XZYNQ_GPIOPS_REG_MASK_OFFSET) + XZYNQ_GPIOPS_INTTYPE_OFFSET) & pin_number;

	intr_pol_reg = in32(gpio_virt_base
			+ ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTPOL_OFFSET) & pin_number;

	intr_on_any_reg = in32(gpio_virt_base + ((bank)
			* XZYNQ_GPIOPS_REG_MASK_OFFSET) + XZYNQ_GPIOPS_INTANY_OFFSET) & pin_number;

	if (intr_type_reg == 1) {
		if (intr_on_any_reg == 1) {
			printf("Type: EDGE BOTH\n");
		} else if (intr_pol_reg == 1) {
			printf("Type: EDGE RISING\n");
		} else {
			printf("Type: EDGE FALLING\n");
		}
	} else {
		if (intr_pol_reg == 1) {
			printf("Type: LEVEL HIGH\n");
		} else {
			printf("Type: LEVEL LOW\n");
		}
	}

	return 0;
}

int xzynq_gpio_set_irq_type(int pin, int irq_type) {
	uint8_t bank, pin_number;
	uint32_t intr_type_reg;
	uint32_t intr_pol_reg;
	uint32_t intr_on_any_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	intr_type_reg = in32(gpio_virt_base + ((bank)
			* XZYNQ_GPIOPS_REG_MASK_OFFSET) + XZYNQ_GPIOPS_INTTYPE_OFFSET);

	intr_pol_reg = in32(gpio_virt_base
			+ ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTPOL_OFFSET);

	intr_on_any_reg = in32(gpio_virt_base + ((bank)
			* XZYNQ_GPIOPS_REG_MASK_OFFSET) + XZYNQ_GPIOPS_INTANY_OFFSET);

	switch (irq_type) {
	case XZYNQ_GPIOPS_IRQ_TYPE_EDGE_RISING:
		intr_type_reg |= (1 << pin_number);
		intr_pol_reg |= (1 << pin_number);
		intr_on_any_reg &= ~(1 << pin_number);
		break;
	case XZYNQ_GPIOPS_IRQ_TYPE_EDGE_FALLING:
		intr_type_reg |= (1 << pin_number);
		intr_pol_reg &= ~(1 << pin_number);
		intr_on_any_reg &= ~(1 << pin_number);
		break;
	case XZYNQ_GPIOPS_IRQ_TYPE_EDGE_BOTH:
		intr_type_reg |= (1 << pin_number);
		intr_on_any_reg |= (1 << pin_number);
		break;
	case XZYNQ_GPIOPS_IRQ_TYPE_LEVEL_HIGH:
		intr_type_reg &= ~(1 << pin_number);
		intr_pol_reg |= (1 << pin_number);
		break;
	case XZYNQ_GPIOPS_IRQ_TYPE_LEVEL_LOW:
		intr_type_reg &= ~(1 << pin_number);
		intr_pol_reg &= ~(1 << pin_number);
		break;
	}

	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTTYPE_OFFSET, intr_type_reg);

	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTPOL_OFFSET, intr_pol_reg);

	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTANY_OFFSET, intr_on_any_reg);

	return 0;
}

int xzynq_gpio_irq_clear(int pin) {
	uint8_t bank, pin_number;
	uint32_t intr_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	/*
	 * Clear the specified pending interrupts.
	 */
	intr_reg = in32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTSTS_OFFSET);

	intr_reg &= (1 << pin);
	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTSTS_OFFSET, intr_reg);

	return 0;
}

int xzynq_gpio_get_irq_status(int pin) {
	uint8_t bank, pin_number;
	uint32_t intr_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	intr_reg = in32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTSTS_OFFSET);

	return (intr_reg & (1 << pin)) ? 1 : 0;
}

int xzynq_gpio_get_irq_enable(int pin) {
	uint8_t bank, pin_number;
	uint32_t intr_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	intr_reg = in32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTMASK_OFFSET);

	return (intr_reg & (1 << pin)) ? 1 : 0;
}

int xzynq_gpio_irq_enable(int pin) {
	uint8_t bank, pin_number;
	uint32_t intr_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	intr_reg = 1 << pin_number;
	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTEN_OFFSET, intr_reg);

	return 0;
}

int xzynq_gpio_irq_disable(int pin) {
	uint8_t bank, pin_number;
	uint32_t intr_reg;

	xzynq_get_bank_pin(pin, &bank, &pin_number);

	intr_reg = 1 << pin_number;
	out32(gpio_virt_base + ((bank) * XZYNQ_GPIOPS_REG_MASK_OFFSET)
			+ XZYNQ_GPIOPS_INTDIS_OFFSET, intr_reg);

	return 0;
}


int xzynq_get_mux_config(int pin) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));

	return mio_reg;
}

int xzynq_set_mux_tri_enable(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x1;

	mio_reg &= ~(1 << XZYNQ_MIO_TRI_ENABLE_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_TRI_ENABLE_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

int xzynq_set_mux_l0_sel(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x1;
	mio_reg &= ~(1 << XZYNQ_MIO_L0_SEL_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_L0_SEL_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

int xzynq_set_mux_l1_sel(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x1;
	mio_reg &= ~(1 << XZYNQ_MIO_L1_SEL_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_L1_SEL_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

int xzynq_set_mux_l2_sel(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x3;

	mio_reg &= ~(0x3 << XZYNQ_MIO_L2_SEL_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_L2_SEL_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

int xzynq_set_mux_l3_sel(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x7;
	mio_reg &= ~(0x7 << XZYNQ_MIO_L3_SEL_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_L3_SEL_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

int xzynq_set_mux_speed(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x1;
	mio_reg &= ~(1 << XZYNQ_MIO_SPEED_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_SPEED_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

int xzynq_set_mux_io_type(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x7;
	mio_reg &= ~(0x7 << XZYNQ_MIO_IO_TYPE_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_IO_TYPE_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

int xzynq_set_mux_pullup(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x1;
	mio_reg &= ~(0x1 << XZYNQ_MIO_PULLUP_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_PULLUP_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

int xzynq_set_mux_dis_rcvr(int pin, int value) {
	uint32_t mio_reg;

	if (pin < 0 || pin > 53)
		return -1;

	/* First unlock the registers */
	out32(mio_virt_base + XZYNQ_SLCR_UNLOCK_OFFSET, XZYNQ_SLCR_UNLOCK_KEY);

	/* Get the value of the pin MIO */
	mio_reg = in32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4));
	value &= 0x1;
	mio_reg &= ~(0x1 << XZYNQ_MIO_DIS_RCVR_SHIFT);

	if (value) {
		mio_reg |= (value << XZYNQ_MIO_DIS_RCVR_SHIFT);
	}

	out32(mio_virt_base + XZYNQ_MIO_OFFSET + (pin * 4), mio_reg);

	/* Lock the registers */
//	out32(mio_virt_base + XZYNQ_SLCR_LOCK_OFFSET, XZYNQ_SLCR_LOCK_KEY);

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/gpio/xzynq_gpio.c $ $Rev: 752039 $")
#endif
