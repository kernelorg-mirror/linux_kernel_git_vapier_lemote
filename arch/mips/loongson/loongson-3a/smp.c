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

void loongson3_raw_writeq(unsigned long action, void * addr)   // write a value to mem 
{                                                           // the value is action     
    *((unsigned long *)addr) = action;
};

unsigned long loongson3_raw_readq(void * addr)                 // read a value from mem 
{                                                           
    return *((unsigned long *)addr);                         // the value will be return     
};

static void *mailbox_set0_regs[] = {                        // addr for core_set0 reg,
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

static void *mailbox_clear0_regs[] = {                      // addr for core_clear0 reg,
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

static void *mailbox_regs0[] = {                            // addr for core_status0 reg
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

static void *mailbox_en0_regs[] = {                        // addr for core_set0 reg,
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


static volatile void *mailbox_buf[] = {                              // addr for core_buf regs
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
 * SMP init and finish on secondary CPUs
 */
void loongson3_smp_init(void)
{
	unsigned int imask = STATUSF_IP7 | STATUSF_IP6 | STATUSF_IP5 | 
                         STATUSF_IP4 | STATUSF_IP3 | STATUSF_IP2 ;
		                 //STATUSF_IP1 | STATUSF_IP0;  // interrupt for software
    int i;
	/* Set interrupt mask, but don't enable */
	change_c0_status(ST0_IM, imask);                   

#if 1 /* gx */
    printk("\n CPU#%d call smp_init!!!! \n", smp_processor_id());
    for (i = 0; i < NR_CPUS; i++) {
        loongson3_raw_writeq(0xffffffff, mailbox_en0_regs[i]);
    }
    printk("\n CPU#%d done smp_init en=%x!!!! \n",  smp_processor_id(), *(int *)(mailbox_en0_regs[smp_processor_id()]));
#endif
}

void loongson3_smp_finish(void)
{
      int tmp;

      tmp = (read_c0_count() + 1000000);
      write_c0_compare(tmp);
      local_irq_enable();
      printk("\n %s, CPU#%d CP0_ST=%x\n", __FUNCTION__, smp_processor_id(), read_c0_status());
}

/*
 * These are routines for dealing with the sb1250 smp capabilities
 * independent of board/firmware
 */

/*
 * Simple enough; everything is set up, so just poke the appropriate mailbox
 * register, and we should be set
 */
void core_send_ipi(int cpu, unsigned int action)
{
	loongson3_raw_writeq((u64)action, mailbox_set0_regs[cpu]);
}

void loongson3_ipi_interrupt(struct pt_regs *regs)
{

	int cpu = smp_processor_id();
	unsigned int action;


#if 0    
	kstat_this_cpu.irqs[63]++;
#endif    
	/* Load the mailbox register to figure out what we're supposed to do */
	action = loongson3_raw_readq(mailbox_regs0[cpu]);
    
	/* Clear the mailbox to clear the interrupt */
	loongson3_raw_writeq((u64)action,mailbox_clear0_regs[cpu]);
	//loongson3_raw_writeq((u64)0xf,mailbox_clear0_regs[cpu]);

	/*
	 * Nothing to do for SMP_RESCHEDULE_YOURSELF; returning from the
	 * interrupt will do the reschedule for us
	 */
#if 0 /* gx */
	if (action & SMP_CALL_FUNCTION)
	  prom_printf("cpu#%d IPI action =%x\n", smp_processor_id(), action);
#endif

	if (action & SMP_CALL_FUNCTION) {
	  smp_call_function_interrupt();

	}

#if 0 /* gx for time */
{
extern unsigned int abscount;
          if(!cpu) {
	   if (action & 0x4)  abscount = read_c0_count();
	    //while(1) printk("abscount=%u\n");
	  }
}	
#endif
}
int loongson3_cpu_start(int cpu, void(*fn)(void), long sp, long gp, long a1)
{
    int res;
    volatile unsigned long long startargs[4];

    startargs[0] = (long)fn;
    startargs[1] = sp;
    startargs[2] = gp;
    startargs[3] = a1;

#if 0
    prom_printf("\n writeq buf is begin ! \n");
    prom_printf("\n CPU#%d mailbox_buf=%p ! \n", cpu, mailbox_buf[cpu]);
    prom_printf("fn=%p\n", fn);
    prom_printf("sp=%lx\n", sp);
    prom_printf("gp=%lx\n", gp);
    prom_printf("a1=%lx\n", a1);
#endif   
    
    loongson3_raw_writeq(startargs[3], mailbox_buf[cpu]+0x18);
    loongson3_raw_writeq(startargs[2], mailbox_buf[cpu]+0x10);
    loongson3_raw_writeq(startargs[1], mailbox_buf[cpu]+0x8);
    loongson3_raw_writeq(startargs[0], mailbox_buf[cpu]+0x0);
 
#if 0
    prom_printf("\n readbackbuf is begin! \n");
    prom_printf("fn=%p\n",  *((unsigned int *)(mailbox_buf[cpu]+0x0)));
    prom_printf("sp=%lx\n", *((unsigned int *)(mailbox_buf[cpu]+0x8)));
    prom_printf("gp=%lx\n", *((unsigned int *)(mailbox_buf[cpu]+0x10)));
    prom_printf("a1=%lx\n", *((unsigned int *)(mailbox_buf[cpu]+0x18)));
#endif   
    res = 0;

    return res;
}

int loongson3_cpu_stop(unsigned int i)
{
    return 0;
}

//void __init plat_smp_setup(void)
void __init loongson3_plat_smp_setup(void)
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

void __init plat_prepare_cpus(unsigned int max_cpus)
{
}

/*
 * Setup the PC, SP, and GP of a secondary processor and start it
 * running!
 */
void prom_boot_secondary(int cpu, struct task_struct *idle)
{
	int retval;

    printk("\n BOOT CPU#%d...\n", cpu);
	retval = loongson3_cpu_start(cpu_logical_map(cpu), &smp_bootstrap,     
			       __KSTK_TOS(idle),                                   
			       (unsigned long)task_thread_info(idle), 0);
	if (retval != 0)
		printk("!!!!!!!loongson3_start_cpu(%i) returned with err%i \n" , cpu, retval);
}

/*
 * Code to run on secondary just after probing the CPU
 */
void prom_init_secondary(void)
{                 
    extern void loongson3_smp_init(void);     
    loongson3_smp_init();                    
}

/*
 * Do any tidying up before marking online and running the idle
 * loop
 */
void prom_smp_finish(void)
{
    extern void loongson3_smp_finish(void);      
    loongson3_smp_finish();                     
}

/*
 * Final cleanup after all secondaries booted
 */
void prom_cpus_done(void)
{
}

static void loongson3_send_ipi_single(int cpu, unsigned int action)
{
#if 0 /* gx */
	prom_printf("---cpu=%d, action=0x%lx\n", cpu, action);
	dump_stack();
#endif
    	loongson3_raw_writeq((((u64)action)), mailbox_set0_regs[cpu]);
}

//void loongson3_ipi_send_ipi_mask(cpumask_t mask, unsigned int action) //cww
void loongson3_ipi_send_ipi_mask(const struct cpumask *mask, unsigned int action)
{
        unsigned int i;

        for_each_cpu(i, mask)  
                loongson3_send_ipi_single(i, action);
}

struct plat_smp_ops loongson3_smp_ops = {
       .send_ipi_single = core_send_ipi,
       .send_ipi_mask = loongson3_ipi_send_ipi_mask,
       .init_secondary = prom_init_secondary,
       .smp_finish = loongson3_smp_finish,
       .cpus_done = prom_cpus_done,
       .boot_secondary = prom_boot_secondary,
       .smp_setup = loongson3_plat_smp_setup,
       .prepare_cpus = plat_prepare_cpus,
};
