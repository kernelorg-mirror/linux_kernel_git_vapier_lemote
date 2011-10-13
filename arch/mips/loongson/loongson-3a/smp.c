/*
 * Copyright (C) 2000, 2001, 2002, 2003 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <asm/processor.h>

#include "smp.h"

void loongson3_ipi_write64(uint64_t action, void * addr)   // write a value to mem 
{                                                           // the value is action     
	*((uint64_t *)addr) = action;
};

uint64_t loongson3_ipi_read64(void * addr)                 // read a value from mem 
{                                                           
	return *((uint64_t *)addr);                         // the value will be return     
};

void loongson3_ipi_write32(uint32_t action, void * addr)   // write a value to mem 
{                                                           // the value is action     
	*((uint32_t *)addr) = action;
};

uint32_t loongson3_ipi_read32(void * addr)                 // read a value from mem 
{                                                           
	return *((uint32_t *)addr);                         // the value will be return     
};
static void *ipi_set0_regs[] = {                        // addr for core_set0 reg,
	(void *)(smp_core_group0_base + smp_core0_offset + SET0),              // which is a 32 bit reg
	(void *)(smp_core_group0_base + smp_core1_offset + SET0),              // When the bit of core_set0 is 1,
	(void *)(smp_core_group0_base + smp_core2_offset + SET0),              // the bit of core_status0 become 1
	(void *)(smp_core_group0_base + smp_core3_offset + SET0),              // immediately                       
	(void *)(smp_core_group1_base + smp_core0_offset + SET0),              // which is a 32 bit reg
	(void *)(smp_core_group1_base + smp_core1_offset + SET0),              // When the bit of core_set0 is 1,
	(void *)(smp_core_group1_base + smp_core2_offset + SET0),              // the bit of core_status0 become 1
	(void *)(smp_core_group1_base + smp_core3_offset + SET0),              // immediately                       
	(void *)(smp_core_group2_base + smp_core0_offset + SET0),              // which is a 32 bit reg
	(void *)(smp_core_group2_base + smp_core1_offset + SET0),              // When the bit of core_set0 is 1,
	(void *)(smp_core_group2_base + smp_core2_offset + SET0),              // the bit of core_status0 become 1
	(void *)(smp_core_group2_base + smp_core3_offset + SET0),              // immediately                       
	(void *)(smp_core_group3_base + smp_core0_offset + SET0),              // which is a 32 bit reg
	(void *)(smp_core_group3_base + smp_core1_offset + SET0),              // When the bit of core_set0 is 1,
	(void *)(smp_core_group3_base + smp_core2_offset + SET0),              // the bit of core_status0 become 1
	(void *)(smp_core_group3_base + smp_core3_offset + SET0),              // immediately                       
};

static void *ipi_clear0_regs[] = {                      // addr for core_clear0 reg,
	(void *)(smp_core_group0_base + smp_core0_offset + CLEAR0),            // which is a 32 bit reg
	(void *)(smp_core_group0_base + smp_core1_offset + CLEAR0),            // When the bit of core_clear0 is 1,
	(void *)(smp_core_group0_base + smp_core2_offset + CLEAR0),            // the bit of core_status0 become 0
	(void *)(smp_core_group0_base + smp_core3_offset + CLEAR0),            // immediately
	(void *)(smp_core_group1_base + smp_core0_offset + CLEAR0),            // which is a 32 bit reg
	(void *)(smp_core_group1_base + smp_core1_offset + CLEAR0),            // When the bit of core_clear0 is 1,
	(void *)(smp_core_group1_base + smp_core2_offset + CLEAR0),            // the bit of core_status0 become 0
	(void *)(smp_core_group1_base + smp_core3_offset + CLEAR0),            // immediately
	(void *)(smp_core_group2_base + smp_core0_offset + CLEAR0),            // which is a 32 bit reg
	(void *)(smp_core_group2_base + smp_core1_offset + CLEAR0),            // When the bit of core_clear0 is 1,
	(void *)(smp_core_group2_base + smp_core2_offset + CLEAR0),            // the bit of core_status0 become 0
	(void *)(smp_core_group2_base + smp_core3_offset + CLEAR0),            // immediately
	(void *)(smp_core_group3_base + smp_core0_offset + CLEAR0),            // which is a 32 bit reg
	(void *)(smp_core_group3_base + smp_core1_offset + CLEAR0),            // When the bit of core_clear0 is 1,
	(void *)(smp_core_group3_base + smp_core2_offset + CLEAR0),            // the bit of core_status0 become 0
	(void *)(smp_core_group3_base + smp_core3_offset + CLEAR0),            // immediately
};

static void *ipi_status_regs0[] = {                            // addr for core_status0 reg
	(void *)(smp_core_group0_base + smp_core0_offset + STATUS0),           // which is a 32 bit reg
	(void *)(smp_core_group0_base + smp_core1_offset + STATUS0),           // the reg is read only
	(void *)(smp_core_group0_base + smp_core2_offset + STATUS0),
	(void *)(smp_core_group0_base + smp_core3_offset + STATUS0),
	(void *)(smp_core_group1_base + smp_core0_offset + STATUS0),           // which is a 32 bit reg
	(void *)(smp_core_group1_base + smp_core1_offset + STATUS0),           // the reg is read only
	(void *)(smp_core_group1_base + smp_core2_offset + STATUS0),
	(void *)(smp_core_group1_base + smp_core3_offset + STATUS0),
	(void *)(smp_core_group2_base + smp_core0_offset + STATUS0),           // which is a 32 bit reg
	(void *)(smp_core_group2_base + smp_core1_offset + STATUS0),           // the reg is read only
	(void *)(smp_core_group2_base + smp_core2_offset + STATUS0),
	(void *)(smp_core_group2_base + smp_core3_offset + STATUS0),
	(void *)(smp_core_group3_base + smp_core0_offset + STATUS0),           // which is a 32 bit reg
	(void *)(smp_core_group3_base + smp_core1_offset + STATUS0),           // the reg is read only
	(void *)(smp_core_group3_base + smp_core2_offset + STATUS0),
	(void *)(smp_core_group3_base + smp_core3_offset + STATUS0),
};

static void *ipi_en0_regs[] = {                        // addr for core_set0 reg,
	(void *)(smp_core_group0_base + smp_core0_offset + EN0),              // which is a 32 bit reg
	(void *)(smp_core_group0_base + smp_core1_offset + EN0),              // When the bit of core_set0 is 1,
	(void *)(smp_core_group0_base + smp_core2_offset + EN0),              // the bit of core_status0 become 1
	(void *)(smp_core_group0_base + smp_core3_offset + EN0),              // immediately                       
	(void *)(smp_core_group1_base + smp_core0_offset + EN0),              // which is a 32 bit reg
	(void *)(smp_core_group1_base + smp_core1_offset + EN0),              // When the bit of core_set0 is 1,
	(void *)(smp_core_group1_base + smp_core2_offset + EN0),              // the bit of core_status0 become 1
	(void *)(smp_core_group1_base + smp_core3_offset + EN0),              // immediately                       
	(void *)(smp_core_group2_base + smp_core0_offset + EN0),              // which is a 32 bit reg
	(void *)(smp_core_group2_base + smp_core1_offset + EN0),              // When the bit of core_set0 is 1,
	(void *)(smp_core_group2_base + smp_core2_offset + EN0),              // the bit of core_status0 become 1
	(void *)(smp_core_group2_base + smp_core3_offset + EN0),              // immediately                       
	(void *)(smp_core_group3_base + smp_core0_offset + EN0),              // which is a 32 bit reg
	(void *)(smp_core_group3_base + smp_core1_offset + EN0),              // When the bit of core_set0 is 1,
	(void *)(smp_core_group3_base + smp_core2_offset + EN0),              // the bit of core_status0 become 1
	(void *)(smp_core_group3_base + smp_core3_offset + EN0),              // immediately                       
};


static volatile void *ipi_mailbox_buf[] = {                              // addr for core_buf regs
	(void *)(smp_core_group0_base + smp_core0_offset + BUF),               // a group of regs with 0x40 byte size
	(void *)(smp_core_group0_base + smp_core1_offset + BUF),               // which could be used for  
	(void *)(smp_core_group0_base + smp_core2_offset + BUF),               // transfer args , r/w , uncached
	(void *)(smp_core_group0_base + smp_core3_offset + BUF),
	(void *)(smp_core_group1_base + smp_core0_offset + BUF),               // a group of regs with 0x40 byte size
	(void *)(smp_core_group1_base + smp_core1_offset + BUF),               // which could be used for  
	(void *)(smp_core_group1_base + smp_core2_offset + BUF),               // transfer args , r/w , uncached
	(void *)(smp_core_group1_base + smp_core3_offset + BUF),
	(void *)(smp_core_group2_base + smp_core0_offset + BUF),               // a group of regs with 0x40 byte size
	(void *)(smp_core_group2_base + smp_core1_offset + BUF),               // which could be used for  
	(void *)(smp_core_group2_base + smp_core2_offset + BUF),               // transfer args , r/w , uncached
	(void *)(smp_core_group2_base + smp_core3_offset + BUF),
	(void *)(smp_core_group3_base + smp_core0_offset + BUF),               // a group of regs with 0x40 byte size
	(void *)(smp_core_group3_base + smp_core1_offset + BUF),               // which could be used for  
	(void *)(smp_core_group3_base + smp_core2_offset + BUF),               // transfer args , r/w , uncached
	(void *)(smp_core_group3_base + smp_core3_offset + BUF),
};



void loongson3_timer_interrupt(struct pt_regs * regs)
{
#if 0
	int cpu = smp_processor_id();
	int irq = 63;

	printk("timer_int(%d)\n", cpu);
	if (cpu == 0) {
		/*
		 * CPU 0 handles the global timer interrupt job
		 */
		ll_timer_interrupt(63, regs);
	}
	else {
		/*
		 * other CPUs should just do profiling and process accounting
		 */
		ll_local_timer_interrupt(63, regs);
	}
#endif
}

/*
 * Simple enough; everything is set up, so just poke the appropriate mailbox
 * register, and we should be set
 */
static void loongson3_send_ipi_single(int cpu, unsigned int action)
{
	loongson3_ipi_write32((u32)action, ipi_set0_regs[cpu]);
}

static void loongson3_send_ipi_mask(const struct cpumask *mask, unsigned int action)
{
	unsigned int i;

	for_each_cpu(i, mask)  
		loongson3_send_ipi_single(i, action);
}

/*
 * SMP init and finish on secondary CPUs
 */
void loongson3_init_secondary(void)
{
	int i;
	unsigned int imask = STATUSF_IP7 | STATUSF_IP6 | STATUSF_IP5 | 
                         STATUSF_IP4 | STATUSF_IP3 | STATUSF_IP2 ;
		                 //STATUSF_IP1 | STATUSF_IP0;  // interrupt for software
	/* Set interrupt mask, but don't enable */
	change_c0_status(ST0_IM, imask);                   

	printk("\n CPU#%d call init_secondary!!!! \n", smp_processor_id());
	for (i = 0; i < NR_CPUS; i++) {
		loongson3_ipi_write32(0xffffffff, ipi_en0_regs[i]);
	}
	printk("\n CPU#%d done init_secondary en=%x!!!! \n",  smp_processor_id(), *(int *)(ipi_en0_regs[smp_processor_id()]));
}

void loongson3_smp_finish(void)
{
	int tmp;

	tmp = (read_c0_count() + 1000000);
	write_c0_compare(tmp);
	local_irq_enable();
	printk("\n %s, CPU#%d CP0_ST=%x\n", __FUNCTION__, smp_processor_id(), read_c0_status());
}

void loongson3_ipi_interrupt(struct pt_regs *regs)
{

	int cpu = smp_processor_id();
	unsigned int action;


#if 0    
	kstat_this_cpu.irqs[63]++;
#endif    
	/* Load the mailbox register to figure out what we're supposed to do */
	action = loongson3_ipi_read32(ipi_status_regs0[cpu]);
    
	/* Clear the mailbox to clear the interrupt */
	loongson3_ipi_write32((u32)action, ipi_clear0_regs[cpu]);
	//loongson3_ipi_write32((u32)0xf, ipi_clear0_regs[cpu]);

	/*
	 * Nothing to do for SMP_RESCHEDULE_YOURSELF; returning from the
	 * interrupt will do the reschedule for us
	 */

	if (action & SMP_CALL_FUNCTION) {
		smp_call_function_interrupt();
	}
}

int loongson3_cpu_start(int cpu, void(*fn)(void), long sp, long gp, long a1)
{
	int res = 0;
	volatile unsigned long long startargs[4];

	startargs[0] = (long)fn;
	startargs[1] = sp;
	startargs[2] = gp;
	startargs[3] = a1;

	loongson3_ipi_write64(startargs[3], (void*)(ipi_mailbox_buf[cpu]+0x18));
	loongson3_ipi_write64(startargs[2], (void*)(ipi_mailbox_buf[cpu]+0x10));
	loongson3_ipi_write64(startargs[1], (void*)(ipi_mailbox_buf[cpu]+0x8));
	loongson3_ipi_write64(startargs[0], (void*)(ipi_mailbox_buf[cpu]+0x0));

	return res;
}

int loongson3_cpu_stop(unsigned int i)
{
	return 0;
}

void __init loongson3_smp_setup(void)
{
	int i, num;

	//cpus_clear(phys_cpu_present_map);
	//cpu_set(0, phys_cpu_present_map);
	cpus_clear(cpu_possible_map);
	cpu_set(0, cpu_possible_map);

	__cpu_number_map[0] = 0;
	__cpu_logical_map[0] = 0;

	for (i = 1, num = 0; i < NR_CPUS; i++) {
		if (loongson3_cpu_stop(i) == 0) {
			//cpu_set(i, phys_cpu_present_map);
			cpu_set(i, cpu_possible_map);
			__cpu_number_map[i] = ++num;
			__cpu_logical_map[num] = i;
		}
	}
	printk(KERN_INFO "Detected %i available secondary CPU(s)\n", num);
}

void __init loongson3_prepare_cpus(unsigned int max_cpus)
{
}

/*
 * Setup the PC, SP, and GP of a secondary processor and start it
 * running!
 */
void loongson3_boot_secondary(int cpu, struct task_struct *idle)
{
	int retval;

	printk("\n BOOT CPU#%d...\n", cpu);
	retval = loongson3_cpu_start(cpu_logical_map(cpu), &smp_bootstrap,     
			       __KSTK_TOS(idle),                                   
			       (unsigned long)task_thread_info(idle), 0);
	if (retval != 0)
		printk("!!!!!!loongson3_cpu_start(%i) returned with err%i \n" , cpu, retval);
}


/*
 * Final cleanup after all secondaries booted
 */
void loongson3_cpus_done(void)
{
}

struct plat_smp_ops loongson3_smp_ops = {
	.send_ipi_single = loongson3_send_ipi_single,
	.send_ipi_mask = loongson3_send_ipi_mask,
	.init_secondary = loongson3_init_secondary,
	.smp_finish = loongson3_smp_finish,
	.cpus_done = loongson3_cpus_done,
	.boot_secondary = loongson3_boot_secondary,
	.smp_setup = loongson3_smp_setup,
	.prepare_cpus = loongson3_prepare_cpus,
};
