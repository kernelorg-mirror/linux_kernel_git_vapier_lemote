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
#include <asm/bootinfo.h>

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pm.h>
#include <linux/pci.h>
#include <ec_wpce775l.h>

#define PM_INDEX        0xCD6
#define PM_DATA         0xCD7

void pmio_write(int index, u8 value)
{
	outb(index, PM_INDEX);
	outb(value, PM_DATA);
}

void set_watchdog_base(u32 base)
{
	pmio_write(0x6c, (base >> 0) & 0xff);
	pmio_write(0x6d, (base >> 8) & 0xff);
	pmio_write(0x6e, (base >> 16) & 0xff);
	pmio_write(0x6f, (base >> 24) & 0xff);
}

#ifdef CONFIG_32BIT
u32 * watchdog_base = 0xbe010000;
#else
u32 * watchdog_base = (u32 *)0x90000e007f000000;
#endif

void enable_watchdog(void)
{
	struct pci_dev * pdev;
	struct resource *r;

	pdev = pci_get_device(PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SBX00_SMBUS, NULL);

	pmio_write(0x69, 0); //enable watchdog

        r = request_mem_region((u32)watchdog_base, 0x1000, "watchdog");
        if (!r) {
                printk(KERN_ERR "requeset watchdog region failed!\n");
                return ;
        }

	set_watchdog_base((u32)watchdog_base); // not in standard mem region

	pci_write_config_byte(pdev, 0x41, 0xff); //eanble smbus watchdog decode
}

void start_watchdog_poweroff(void)
{
	*watchdog_base = 5; // powroff whan watchdog timeout
	*(watchdog_base + 1) = 0x500; // set counter
	*watchdog_base |= 0x80; //start watchdog
}

void watchdog_poweroff(void)
{
	enable_watchdog();
	start_watchdog_poweroff();
	
	printk(KERN_ERR "Ohh, poweroff not work???? \n");
}

/* 
 * 3A ITX(A1101) reset method 
 *
 * Loongson3A have 16 GPIOs,
 * GPIO1,GPIO3,GPIO4 & GPIO14 are used in driver watchdog chip(MAX6369)
 *
 * GPIO14 should keep high/low to start watchdog counter.
 *
 * GPIO1, GPIO4 & GPIO3 are link to watchdong pin SET2, SET1, SET0.
 * SET2, SET1, SET0 determine watchdog timing characteristics, the timing table as follow:
 *
 * [SET2, SET1, SET0]     watchdog timeout period
 *
 *   [0, 0, 0]			  1 ms
 *   [0, 0, 1]			 10 ms
 *   [0, 1, 0]			 30 ms
 *   [0, 1, 1]			Disable
 *   [1, 0, 0]			100 ms
 *   [1, 0, 1]			   1 s
 *   [1, 1, 0]			  10 s
 *   [1, 1, 1]			  60 s
 */

#define GPIO1	(1<<1)
#define GPIO3	(1<<3)
#define GPIO4	(1<<4)
#define GPIO14	(1<<14)

#ifdef CONFIG_64BIT
#define LOONGSON3A_GPIO_OUTPUT_DATA	0xffffffffbfe0011c
#define LOONGSON3A_GPIO_OUTPUT_ENABLE	0xffffffffbfe00120
#else
#define LOONGSON3A_GPIO_OUTPUT_DATA	0xbfe0011c
#define LOONGSON3A_GPIO_OUTPUT_ENABLE	0xbfe00120
#endif

static void loongson3a_gpio_out_low(u32 gpio)
{
	u32 reg;

	/* set output low level*/
	reg = *(u32 *)LOONGSON3A_GPIO_OUTPUT_DATA;
	reg &= ~(gpio);
	*(u32 *)LOONGSON3A_GPIO_OUTPUT_DATA = reg;

	/* enable output*/
	reg = *(u32 *)LOONGSON3A_GPIO_OUTPUT_ENABLE;
	reg &= ~(gpio);
	*(u32 *)LOONGSON3A_GPIO_OUTPUT_ENABLE = reg;
}

static void loongson3a_gpio_out_high(u32 gpio)
{
	u32 reg;

	/* set output high level*/
	reg = *(u32 *)LOONGSON3A_GPIO_OUTPUT_DATA;
	reg |= (gpio);
	*(u32 *)LOONGSON3A_GPIO_OUTPUT_DATA = reg;

	/* enable output*/
	reg = *(u32 *)LOONGSON3A_GPIO_OUTPUT_ENABLE;
	reg &= ~(gpio);
	*(u32 *)LOONGSON3A_GPIO_OUTPUT_ENABLE = reg;
}

static void enable_cpu_watchdog(void)
{
	loongson3a_gpio_out_high(GPIO14);
	
	/* [SET2, SET1, SET0] ---  [0, 0, 0]  ---  1ms */
	loongson3a_gpio_out_low(GPIO1);
	loongson3a_gpio_out_low(GPIO4);
	loongson3a_gpio_out_low(GPIO3);
	
	/* start watchdog timer */
	loongson3a_gpio_out_low (GPIO14);
}

static void itx_a1101_reboot(void)
{
	enable_cpu_watchdog();
}

static void notebook_a1004_reboot(void)
{
	ec_write_noindex(CMD_RESET, RESET_ON);
}

void mach_prepare_reboot(void)
{
	switch (mips_machtype) {
	case	MACH_LEMOTE_3A_A1004:
		notebook_a1004_reboot();
		break;
	case	MACH_LEMOTE_3A_A1101:
		itx_a1101_reboot();
		break;
	default:
		break;
	}
}

static void notebook_a1004_shutdown(void)
{
	ec_write_noindex(CMD_RESET, PWROFF_ON);
}

static void itx_a1101_shutdown(void)
{
	watchdog_poweroff();
}

void mach_prepare_shutdown(void)
{	
	switch (mips_machtype) {
	case	MACH_LEMOTE_3A_A1004:
		notebook_a1004_shutdown();
		break;
	case	MACH_LEMOTE_3A_A1101:
		itx_a1101_shutdown();
		break;
	default:
		break;
	}
}
