/*
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzhangjin@gmail.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/err.h>
#include <linux/platform_device.h>

#include <asm/bootinfo.h>

static struct platform_device notebook_a1004_pdev = {
	.name = "loongson3a_notebook_a1004",
	.id = -1,
};

static struct platform_device itx_a1101_pdev = {
	.name = "loongson3a_itx_a1101",
	.id = -1,
};

static int __init loongson3a_platform_init(void)
{
	struct platform_device *pdev = NULL;

	switch (mips_machtype) {
	case MACH_LEMOTE_3A_A1004:
		pdev = &notebook_a1004_pdev;
		break;
	case MACH_LEMOTE_3A_A1101:
		pdev = &itx_a1101_pdev;
		break;
	default:
		break;

	}

	if (pdev != NULL)
		return platform_device_register(pdev);

	return -ENODEV;
}

arch_initcall(loongson3a_platform_init);
