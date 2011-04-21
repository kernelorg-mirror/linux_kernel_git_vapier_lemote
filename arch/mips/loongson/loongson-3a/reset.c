/*
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Copyright (C) 2007 Lemote, Inc. & Institute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 */

#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/reboot.h>
#include <asm/system.h>

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pm.h>
#include <linux/delay.h>

#include <ec_wpce775l.h>

static void delay(void)
{
	volatile int i;
	for (i=0; i<0x10000; i++);
}

void mach_prepare_reboot(void)
{
	printk(KERN_ERR "mach_prepare_reboot start\n");
	ec_write_noindex(CMD_RESET, BIT_RESET_ON);
	printk(KERN_ERR "mach_prepare_reboot end\n");
	delay();
}

void mach_prepare_shutdown(void)
{
	ec_write_noindex(CMD_RESET, BIT_PWROFF_ON);
	delay();
}
