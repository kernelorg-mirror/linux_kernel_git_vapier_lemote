/*
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzhangjin@gmail.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/bootmem.h>

#include <loongson.h>
#include <asm/smp-ops.h>

#define HT_control_regs_base	0x90000EFDFB000000
#define HT_uncache_enable_reg0	*(volatile unsigned int *)(HT_control_regs_base + 0xF0)
#define HT_uncache_base_reg0	*(volatile unsigned int *)(HT_control_regs_base + 0xF4)
#define HT_uncache_enable_reg1	*(volatile unsigned int *)(HT_control_regs_base + 0xF8)
#define HT_uncache_base_reg1	*(volatile unsigned int *)(HT_control_regs_base + 0xFC)
extern void prom_printf(char *fmt, ...);
extern struct plat_smp_ops loongson3_smp_ops;

/* Loongson CPU address windows config space base address */
unsigned long __maybe_unused _loongson_addrwincfg_base;

void __init prom_init(void)
{
	/* init base address of io space */
	set_io_port_base((unsigned long)
		ioremap(LOONGSON_PCIIO_BASE, LOONGSON_PCIIO_SIZE));

#ifdef CONFIG_CPU_SUPPORTS_ADDRWINCFG
	_loongson_addrwincfg_base = (unsigned long)
		ioremap(LOONGSON_ADDRWINCFG_BASE, LOONGSON_ADDRWINCFG_SIZE);
#endif

	prom_init_cmdline();
	prom_init_env();
#ifdef CONFIG_NUMA
	prom_init_numa_memory();
#else
	prom_init_memory();
#endif

	/*init the uart base address */
	prom_init_uart_base();

#ifdef CONFIG_SMP
	register_smp_ops(&loongson3_smp_ops);
#endif

#ifdef CONFIG_DMA_NONCOHERENT
//set HT-access uncache
	HT_uncache_enable_reg0	= 0xc0000000;
	HT_uncache_base_reg0	= 0x0080ff80;
#else
//set HT-access cache
	HT_uncache_enable_reg0	= 0x0;
	HT_uncache_enable_reg1	= 0x0;
	prom_printf("SET HT_DMA CACHED\n");
#endif

#if 0
{
	char * p = 0x900000003ff02000;
	char * end = 0x900000003ff020b8;
	for(;p<end; p+=8){
		prom_printf("======== [%p]=%p ========\n", p, *(unsigned long long volatile *)p);
	}
	char * p = 0x900000003ff00000;
	char * end = 0x900000003ff000b8;
	for(;p<end; p+=8){
		prom_printf("======== [%p]=%p ========\n", p, *(unsigned long long volatile *)p);
	}
	char * p = 0x900000003ff00100;
	char * end = 0x900000003ff001b8;
	for(;p<end; p+=8){
		prom_printf("======== [%p]=%p ========\n", p, *(unsigned long long volatile *)p);
	}
}
#endif
}

void __init prom_free_prom_memory(void)
{
}
