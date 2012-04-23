/*
 * Based on Ocelot Linux port, which is
 * Copyright 2001 MontaVista Software Inc.
 * Author: jsun@mvista.com or jsun@junsun.net
 *
 * Copyright 2003 ICT CAS
 * Author: Michael Guo <guoyi@ict.ac.cn>
 *
 * Copyright (C) 2007 Lemote Inc. & Insititute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzhangjin@gmail.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/module.h>
#include <asm/bootinfo.h>
#include <loongson.h>
#include <boot_param.h>

#define BOOT_PARAM

#undef CONFIG_NR_CPUS
#define CONFIG_NR_CPUS nr_cpu_loongson
struct boot_params *bp;
struct loongson_params *lp; 

struct efi_memory_map_loongson *emap;
struct efi_cpuinfo_loongson *ecpu;
struct system_loongson *esys;
struct irq_source_routing_table *eirq_source;

u64 pci_mem_start_addr,pci_mem_end_addr;
u64 memstart, highmemstart;
u32 memsize, highmemsize;
u64 ht_control_base;
u64 loongson_pciio_base;
u64 poweroff_addr;
u64 reboot_addr;
u64 vbios_addr;

unsigned int nr_cpu_loongson;
enum loongson_cpu_type cputype;

u32 cpu_clock_freq;
EXPORT_SYMBOL(cpu_clock_freq);

#define parse_even_earlier(res, option, p)				\
do {									\
	if (strncmp(option, (char *)p, strlen(option)) == 0)		\
			strict_strtol((char *)p + strlen(option"="),	\
					10, &res);			\
} while (0)

void __init prom_init_env(void)
{
	/* pmon passes arguments in 32bit pointers */
	int i;
#ifndef BOOT_PARAM
	unsigned long bus_clock;
	unsigned int processor_id;
	long l;
	/* firmware arguments are initialized in head.S */
	_prom_envp = (int *)fw_arg2;

	l = (long)*_prom_envp;
	while (l != 0) {
		parse_even_earlier(bus_clock, "busclock", l);
		parse_even_earlier(cpu_clock_freq, "cpuclock", l);
		parse_even_earlier(memsize, "memsize", l);
		parse_even_earlier(highmemsize, "highmemsize", l);
		_prom_envp++;
		l = (long)*_prom_envp;
	}
	if (memsize == 0)
		memsize = 256;
	if (bus_clock == 0)
		bus_clock = 66000000;
	if (cpu_clock_freq == 0) {
		processor_id = (&current_cpu_data)->processor_id;
		switch (processor_id & PRID_REV_MASK) {
		case PRID_REV_LOONGSON2E:
			cpu_clock_freq = 533080000;
			break;
		case PRID_REV_LOONGSON2F:
			cpu_clock_freq = 797000000;
			break;
		case PRID_REV_LOONGSON3A:
			cpu_clock_freq = 797000000;
			break;
		default:
			cpu_clock_freq = 100000000;
			break;
		}
	}
	
	pr_info("busclock=%ld, cpuclock=%ld, memsize=%ld, highmemsize=%ld, sharevram=%d, vramsize=%d\n",
		bus_clock, cpu_clock_freq, memsize, highmemsize, sharevram, vramsize);
#else
	u32 mem_type;
	u32 node_id;

	bp = (struct boot_params *)fw_arg2;
	lp = &(bp->efi.smbios.lp);

	emap 	= (struct efi_memory_map_loongson *)((u64)lp+lp->memory_offset);
	ecpu	= (struct efi_cpuinfo_loongson *)((u64)lp + lp->cpu_offset);
	eirq_source = (struct irq_source_routing_table *)((u64)lp+lp->irq_offset);

	//parse memory information
	for (i = 0; i < emap->nr_map; i++){
		mem_type = emap->map[i].mem_type;
		node_id = emap->map[i].node_id;

		if(node_id == 0){
			switch(mem_type){
			case SYSTEM_RAM_LOW:
				memsize = emap->map[i].mem_size;	
				memstart = emap->map[i].mem_start;
				break;
			case SYSTEM_RAM_HIGH:
				highmemsize = emap->map[i].mem_size;	
				highmemstart = emap->map[i].mem_start;
				break;
		   	}
		}
	}

	cpu_clock_freq = ecpu->cpu_clock_freq;
	cputype = ecpu->cputype;
	nr_cpu_loongson = ecpu->nr_cpus; 

	pci_mem_start_addr = eirq_source->pci_mem_start_addr;
	pci_mem_end_addr = eirq_source->pci_mem_end_addr;
	loongson_pciio_base = eirq_source->pci_io_start_addr;
  
	pr_info("memstart %llx size %u, highmemstart %llx size %u\n",
                memstart, memsize, highmemstart, highmemsize);
	
	pr_info("pci-start:%llx,pci-end:%llx,pciio base:%llx\n", pci_mem_start_addr, pci_mem_end_addr, loongson_pciio_base);

        poweroff_addr = bp->reset_system.Shutdown;
        reboot_addr = bp->reset_system.ResetWarm;
        printk("shutdown:%llx reset:%llx\n",poweroff_addr,reboot_addr);

	vbios_addr = bp->efi.smbios.vga_bios;
	printk("vbios locate in %llx\n", vbios_addr);

	ht_control_base = 0x90000EFDFB000000; // has no interface now
#endif
}
