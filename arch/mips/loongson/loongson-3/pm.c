/*
 *  Lemote loongson3a family machines' specific suspend support
 *
 *  Copyright (C) 2009 Lemote Inc.
 *  Author: Wu Zhangjin <wuzhangjin@gmail.com>
 *  Author: Chen Huacai <chenhuacai@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/i8042.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>

#include <asm/i8259.h>
#include <asm/mipsregs.h>
#include <asm/bootinfo.h>

#include <loongson.h>

#include <ec_wpce775l.h>
#include "htregs.h"
#include "irqregs.h"

#define I8042_KBD_IRQ		0x01
#define SCI_IRQ			0x07
#define I8042_CTR_KBDINT	0x01
#define I8042_CTR_KBDDIS	0x10

static unsigned char i8042_ctr;
extern void irq_router_init(void);
extern void sci_interrupt_setup(void);

static int i8042_enable_kbd_port(void)
{
	if (i8042_command(&i8042_ctr, I8042_CMD_CTL_RCTR)) {
		pr_err("i8042.c: Can't read CTR while enabling i8042 kbd port."
		       "\n");
		return -EIO;
	}

	i8042_ctr &= ~I8042_CTR_KBDDIS;
	i8042_ctr |= I8042_CTR_KBDINT;

	if (i8042_command(&i8042_ctr, I8042_CMD_CTL_WCTR)) {
		i8042_ctr &= ~I8042_CTR_KBDINT;
		i8042_ctr |= I8042_CTR_KBDDIS;
		pr_err("i8042.c: Failed to enable KBD port.\n");

		return -EIO;
	}

	return 0;
}

void setup_wakeup_events(void)
{
	int irq_mask;

	switch (mips_machtype) {
	case MACH_LEMOTE_3A_A1004:
		/* open the keyboard irq in i8259A */
		outb_p((0xff & ~(1 << I8042_KBD_IRQ)), PIC_MASTER_IMR);

		/* enable keyboard port */
		i8042_enable_kbd_port();

		/* Wakeup CPU via SCI lid open event */
		irq_mask = inb_p(PIC_MASTER_IMR);
		outb_p(irq_mask & ~(1 << SCI_IRQ), PIC_MASTER_IMR);

		break;

	default:
		break;
	}
}

static struct delayed_work lid_task;
static int initialized;
extern void ls3anb_sci_event_handler(int event);
static void lid_update_task(struct work_struct *work)
{
#ifdef CONFIG_LS3A_LAPTOP
	ls3anb_sci_event_handler(SCI_EVENT_NUM_LID);
#endif
}

int wakeup_loongson(void)
{
	int irq;

	/* query the interrupt number */
	irq = HT_irq_vector_reg0;
	if (irq < 0)
		return 0;

	if (irq & (1<<I8042_KBD_IRQ)) {
		return 1;
	}

	else if(irq & (1<<SCI_IRQ)) {
		int sci_event;

		/* query the event number */
		sci_event = ec_query_get_event_num();
		if (sci_event < 0)
			return 0;
		if (sci_event == SCI_EVENT_NUM_LID) {
			int status;
			/* check the LID status */
			status = ec_read(INDEX_DEVICE_STATUS);
			/* wakeup cpu when peple open the LID */
			if (status & (1<<BIT_DEVICE_LID)) {
				if (initialized == 0) {
					INIT_DELAYED_WORK(&lid_task,
							lid_update_task);
					initialized = 1;
				}
				schedule_delayed_work(&lid_task, 1);
				return 1;
			}
		}
	}

	return 0;
}

void mach_suspend(suspend_state_t state)
{
	/* Workaround: disable spurious IRQ1 via EC */
	if (state == PM_SUSPEND_STANDBY) {
		ec_write_noindex(CMD_RESET, 4);
		mdelay(997);
	}
}

void mach_resume(suspend_state_t state)
{
	if (state == PM_SUSPEND_MEM) {
		irq_router_init();
		sci_interrupt_setup();
	}
}
