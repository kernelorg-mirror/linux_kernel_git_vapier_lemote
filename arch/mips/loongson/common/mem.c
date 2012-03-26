/*
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <asm/bootinfo.h>
#include <loongson.h>
#include <mem.h>
#include <pci.h>
#include <boot_param.h>

extern unsigned long long memstart, highmemstart;

void __init prom_init_memory(void)
{
	phys_t	size1, size2;

	if (memstart != 0)
		add_memory_region(0, memstart, BOOT_MEM_RESERVED); // reserve the front nMB memory for BIOS runtime services

	add_memory_region(memstart, memsize << 20, BOOT_MEM_RAM);

#ifdef CONFIG_64BIT
        //Reserver bottom 8M above 4G for rs780 chip
	//check whether mem map overlap with [4G-8M, 4G)
	if ((highmemstart <= 0x100000000) && 
		((highmemstart + (highmemsize << 20)) > 0xff800000)) {
		size1 = 4088 - (highmemstart >> 20); // size < 4G-8M
		size2 = highmemsize - 8 - size1;     // size > 4G

		add_memory_region(highmemstart, size1 << 20, BOOT_MEM_RAM);
                add_memory_region(0xff800000, 0x800000, BOOT_MEM_RESERVED);

		if (size2)
			add_memory_region(0x100000000, size2 << 20, BOOT_MEM_RAM);
	} else {
		add_memory_region(highmemstart, highmemsize << 20, BOOT_MEM_RAM);
	}
#endif /* !CONFIG_64BIT */
}

/* override of arch/mips/mm/cache.c: __uncached_access */
int __uncached_access(struct file *file, unsigned long addr)
{
	if (file->f_flags & O_DSYNC)
		return 1;

	return addr >= __pa(high_memory) ||
		((addr >= LOONGSON_MMIO_MEM_START) &&
		 (addr < LOONGSON_MMIO_MEM_END)); //cww??
}

#ifdef CONFIG_CPU_SUPPORTS_UNCACHED_ACCELERATED

#include <linux/pci.h>
#include <linux/sched.h>
#include <asm/current.h>

static unsigned long uca_start, uca_end;

pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
			      unsigned long size, pgprot_t vma_prot)
{
	unsigned long offset = pfn << PAGE_SHIFT;
	unsigned long end = offset + size;

	if (__uncached_access(file, offset)) {
		if (uca_start && (offset >= uca_start) &&
		    (end <= uca_end))
			return __pgprot((pgprot_val(vma_prot) &
					 ~_CACHE_MASK) |
					_CACHE_UNCACHED_ACCELERATED);
		else
			return pgprot_noncached(vma_prot);
	}
	return vma_prot;
}

static int __init find_vga_mem_init(void)
{
	struct pci_dev *dev = 0;
	struct resource *r;
	int idx;

	if (uca_start)
		return 0;

	for_each_pci_dev(dev) {
		if ((dev->class >> 16) == PCI_BASE_CLASS_DISPLAY) {
			for (idx = 0; idx < PCI_NUM_RESOURCES; idx++) {
				r = &dev->resource[idx];
				if (!r->start && r->end)
					continue;
				if (r->flags & IORESOURCE_IO)
					continue;
				if (r->flags & IORESOURCE_MEM) {
					uca_start = r->start;
					uca_end = r->end;
					return 0;
				}
			}
		}
	}

	return 0;
}

late_initcall(find_vga_mem_init);
#endif /* !CONFIG_CPU_SUPPORTS_UNCACHED_ACCELERATED */
