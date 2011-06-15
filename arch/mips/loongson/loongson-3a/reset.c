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

#ifdef CONFIG_32BIG
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

static void itx_a1101_reboot(void)
{
	u32 reg;
	struct pci_dev * pdev;

	pdev = pci_get_device(PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SBX00_SMBUS, NULL);

	pci_read_config_dword(pdev, 0xa8, &reg); //eanble smbus watchdog decode
	reg &= ~(1 << (5 + 8)); // enable gpio output
	reg &= ~(1 << 5); // output low level
	pci_write_config_dword(pdev, 0xa8, reg);
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
