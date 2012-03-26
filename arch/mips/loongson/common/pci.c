/*
 * Copyright (C) 2007 Lemote, Inc. & Institute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/pci.h>
#include <pci.h>
#include <loongson.h>
#include <boot_param.h>

static struct resource loongson_pci_mem_resource = {
	.name   = "pci memory space",
	.flags  = IORESOURCE_MEM,
};

static struct resource loongson_pci_io_resource = {
	.name   = "pci io space",
	.start  = LOONGSON_PCI_IO_START,
	.end    = IO_SPACE_LIMIT,
	.flags  = IORESOURCE_IO,
};

static struct pci_controller  loongson_pci_controller = {
	.pci_ops        = &loongson_pci_ops,
	.io_resource    = &loongson_pci_io_resource,
	.mem_resource   = &loongson_pci_mem_resource,
	.mem_offset     = 0x00000000UL,
	.io_offset      = 0x00000000UL,
};

extern int sb700_acpi_init(void);

static int __init pcibios_init(void)
{
	loongson_pci_controller.io_map_base = mips_io_port_base;
	loongson_pci_mem_resource.start = pci_mem_start_addr;
	loongson_pci_mem_resource.end = pci_mem_end_addr;

	register_pci_controller(&loongson_pci_controller);

#ifdef CONFIG_CPU_LOONGSON3
	/* SCI setup, daway 2011-03-11 */
	sb700_acpi_init();
#endif
	return 0;
}

arch_initcall(pcibios_init);
