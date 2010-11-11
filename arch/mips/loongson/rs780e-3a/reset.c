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

extern void _wrmsr(u32 reg, u32 hi, u32 lo);
extern void _rdmsr(u32 reg, u32 *hi, u32 *lo);

//static void loongson3a_restart(char *command)
void mach_prepare_reboot(void)
{
//#ifdef CONFIG_LEMOTE_FULONG2F
#if defined(CONFIG_LEMOTE_FULONG2F) || defined(CONFIG_GODSON2G_FPGA) || defined(CONFIG_LOONGSON3A_EVA) || defined(CONFIG_LOONGSON3A_SERVER) || defined(CONFIG_LOONGSON3A_RS780E)
	u32 hi, lo;
	_rdmsr(0xe0000014, &hi, &lo);
	lo |= 0x00000001;
	_wrmsr(0xe0000014, hi, lo);
#else

#ifdef CONFIG_32BIT
	*(unsigned long *)0xbfe00104 &= ~(1 << 2);
	*(unsigned long *)0xbfe00104 |= (1 << 2);
#else
	*(unsigned long *)0xffffffffbfe00104 &= ~(1 << 2);
	*(unsigned long *)0xffffffffbfe00104 |= (1 << 2);
#endif
#endif
	printk("Hard reset not take effect!!\n");
	__asm__ __volatile__ (
					".long 0x3c02bfc0\n"
					".long 0x00400008\n"
					:::"v0"
					);
}


static void delay(void)
{
	volatile int i;
	for (i=0; i<0x10000; i++);
}

//static void loongson3a_halt(void)
void mach_prepare_shutdown(void)
{
//#ifdef CONFIG_LEMOTE_FULONG2F
#if defined(CONFIG_LEMOTE_FULONG2F) || defined(CONFIG_GODSON2G_FPGA) || defined(CONFIG_LOONGSON3A_EVA) || defined(CONFIG_LOONGSON3A_SERVER) || defined(CONFIG_LOONGSON3A_RS780E)
#ifdef CONFIG_32BIT
	u32 base;
#else
	u64 base;
#endif
	u32 hi, lo, val;
	
	_rdmsr(0x8000000c, &hi, &lo);
#ifdef CONFIG_32BIT
	base = (lo & 0xff00) | 0xbfd00000;
#else
	base = (lo & 0xff00) | 0xffffffffbfd00000ULL;
#endif
	val = *(volatile unsigned int *)(base + 0x04);
	val = (val & ~(1 << (16 + 13))) | (1 << 13);
	delay();
	*(__volatile__ u32 *)(base + 0x04) = val;
	delay();
	val = (val & ~(1 << (13))) | (1 << (16 + 13));
	delay();
	*(__volatile__ u32 *)(base + 0x00) = val;
	delay();
#else
#ifdef CONFIG_32BIT
	*(unsigned short*)0xbfd0b000 = 0x8100;
	delay();
	*(unsigned short *)0xbfd0b004 = 0x2800;
	delay();
	*(unsigned int *)0xbfe00148 = 0x120002;
#else
	*(unsigned short *)0xffffffffbfd0b000 = 0x8100;
	delay();
	*(unsigned short *)0xffffffffbfd0b004 = 0x2800;
	delay();
	*(unsigned int *)0xffffffffbfe00148 = 0x120002;
#endif
#endif

}
