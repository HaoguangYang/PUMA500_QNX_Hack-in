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

/*
 * Listen for interrupt on the XADC. If found, deliver them to any
 * user code that may be waiting for notification.
 */
void *
xadc_intr_thread(void *arg)
{
	_Uint32t intr_status = 0;
	xadc_dev_t *dev;

	dev = (xadc_dev_t *) arg;

	/* Listen for interrupts on the XADC */
	ThreadCtl(_NTO_TCTL_IO, 0);
	SIGEV_INTR_INIT(&dev->xadc_intr);
	InterruptAttachEvent(XZYNQ_IRQ_XADC, &dev->xadc_intr,
			_NTO_INTR_FLAGS_TRK_MSK);

	while (1) {
		InterruptWait(0, NULL);

		/* Find which alarms have been raised */
		intr_status = xadc_intr_get_status(dev);

		/* Deliver notifications to the relevant users */
		if ((intr_status & XADC_INTX_ALM0_MASK)
				&& dev->alarm_notification_ids[0] != -1) {
			MsgDeliverEvent(dev->alarm_notification_ids[0],
					&dev->alarm_notifications[0]);
		}
		if ((intr_status & XADC_INTX_ALM1_MASK)
				&& dev->alarm_notification_ids[1] != -1) {
			MsgDeliverEvent(dev->alarm_notification_ids[1],
					&dev->alarm_notifications[1]);
		}
		if ((intr_status & XADC_INTX_ALM2_MASK)
				&& dev->alarm_notification_ids[2] != -1) {
			MsgDeliverEvent(dev->alarm_notification_ids[2],
					&dev->alarm_notifications[2]);
		}
		if ((intr_status & XADC_INTX_ALM3_MASK)
				&& dev->alarm_notification_ids[3] != -1) {
			MsgDeliverEvent(dev->alarm_notification_ids[3],
					&dev->alarm_notifications[3]);
		}
		if ((intr_status & XADC_INTX_ALM4_MASK)
				&& dev->alarm_notification_ids[4] != -1) {
			MsgDeliverEvent(dev->alarm_notification_ids[4],
					&dev->alarm_notifications[4]);
		}
		if ((intr_status & XADC_INTX_ALM5_MASK)
				&& dev->alarm_notification_ids[5] != -1) {
			MsgDeliverEvent(dev->alarm_notification_ids[5],
					&dev->alarm_notifications[5]);
		}
		if ((intr_status & XADC_INTX_ALM6_MASK)
				&& dev->alarm_notification_ids[6] != -1) {
			MsgDeliverEvent(dev->alarm_notification_ids[6],
					&dev->alarm_notifications[6]);
		}
		if ((intr_status & XADC_INTX_OT_MASK) && dev->alarm_notification_ids[7]
				!= -1) {
			MsgDeliverEvent(dev->alarm_notification_ids[7],
					&dev->alarm_notifications[7]);
		}
		if (intr_status & XADC_INTX_DFIFO_GTH_MASK) {
			//Do nothing
		}
		if (intr_status & XADC_INTX_CFIFO_LTH_MASK) {
			//Do nothing
		}

		/* Clear the interrupts */
		xadc_intr_clear(dev, intr_status);

		InterruptUnmask(XZYNQ_IRQ_XADC, NULL);
	}

	return NULL;
}

/*
 * Given a mask of enabled alarms, return a mask of
 * the corresponding interrupts to enable.
 */
_Uint32t xadc_alarm_to_interrupt_mask(_Uint16t alarm_mask)
{
	_Uint32t intr_mask = 0x0;

	if (alarm_mask & XADC_CFR1_OT_MASK) {
		intr_mask |= XADC_INTX_OT_MASK;
	}
	if (alarm_mask & XADC_CFR1_ALM_VCCPDRO_MASK) {
		intr_mask |= XADC_INTX_ALM6_MASK;
	}
	if (alarm_mask & XADC_CFR1_ALM_VCCPAUX_MASK) {
		intr_mask |= XADC_INTX_ALM5_MASK;
	}
	if (alarm_mask & XADC_CFR1_ALM_VCCPINT_MASK) {
		intr_mask |= XADC_INTX_ALM4_MASK;
	}
	if (alarm_mask & XADC_CFR1_ALM_VCCPDRO_MASK) {
		intr_mask |= XADC_INTX_ALM6_MASK;
	}
	if (alarm_mask & XADC_CFR1_ALM_VBRAM_MASK) {
		intr_mask |= XADC_INTX_ALM3_MASK;
	}
	if (alarm_mask & XADC_CFR1_ALM_VCCAUX_MASK) {
		intr_mask |= XADC_INTX_ALM2_MASK;
	}
	if (alarm_mask & XADC_CFR1_ALM_VCCINT_MASK) {
		intr_mask |= XADC_INTX_ALM1_MASK;
	}
	if (alarm_mask & XADC_CFR1_ALM_TEMP_MASK) {
		intr_mask |= XADC_INTX_ALM0_MASK;
	}
	return intr_mask;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/support/xzynq/xadc/intr.c $ $Rev: 752035 $")
#endif
