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
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/cpufreq.h>
#include <asm/processor.h>
#include <asm/clock.h>
#include <asm/tlbflush.h>
#include <loongson.h>

#include "smp.h"

DEFINE_PER_CPU(int, cpu_state);
DEFINE_PER_CPU(uint32_t, core0_c0count);
 
extern int cpufreq_enabled;
extern uint64_t cmos_read64(unsigned long addr);
extern void cmos_write64(uint64_t data, unsigned long addr);
 
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

	printk(KERN_DEBUG "timer_int(%d)\n", cpu);
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
 *  * These are routines for dealing with the sb1250 smp capabilities
 *   * independent of board/firmware
 *    */
int loongson3_send_irq_by_ipi(int cpu,int irqs)
{
        loongson3_ipi_write64((u64)(irqs<<4) , (void *)ipi_mailbox_buf[cpu]);
        return 0;
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

#define MAX_LOOPS 1380

/*
 * SMP init and finish on secondary CPUs
 */
void __cpuinit loongson3_init_secondary(void)
{
	int i;
	uint32_t initcount;
 	unsigned int cpu = smp_processor_id();	
	unsigned int imask = STATUSF_IP7 | STATUSF_IP6 |
                            STATUSF_IP3 | STATUSF_IP2;

	/* Set interrupt mask, but don't enable */
	change_c0_status(ST0_IM, imask);                   

	printk(KERN_DEBUG "\n CPU#%d call init_secondary!!!! \n", cpu);
	for (i = 0; i < nr_cpu_loongson; i++) {
		loongson3_ipi_write32(0xffffffff, ipi_en0_regs[i]);
	}
 
	per_cpu(cpu_state, cpu) = CPU_ONLINE;

	i = 0;
	__get_cpu_var(core0_c0count) = 0;
	loongson3_send_ipi_single(0, SMP_ASK_C0COUNT);
	while(!__get_cpu_var(core0_c0count))
		i++;

	if(i > MAX_LOOPS) i = MAX_LOOPS;
	initcount = __get_cpu_var(core0_c0count) + i * 2;
	write_c0_count(initcount);
	write_c0_compare(initcount + 1000000);
	printk(KERN_DEBUG "\n CPU#%d done init_secondary en=%x!!!! \n", cpu, *(int *)(ipi_en0_regs[cpu]));
}

void __cpuinit loongson3_smp_finish(void)
{
	local_irq_enable();
	loongson3_ipi_write64(0, (void *)(ipi_mailbox_buf[smp_processor_id()]+0x0));
	printk(KERN_DEBUG "\n %s, CPU#%d CP0_ST=%x\n", __FUNCTION__, smp_processor_id(), read_c0_status());
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

	if (action & SMP_ASK_C0COUNT) {
		int i;
		uint32_t c0count = read_c0_count();
		for(i = 1; i < nr_cpu_loongson; i++)
			per_cpu(core0_c0count, i) = c0count;
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

	printk(KERN_DEBUG "CPU %d, fn=%lx, sp=%lx, gp=%lx\n", cpu, (long)fn, sp, gp);

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

 	cmos_write64(0x0, 0x40);  //pc
 	cmos_write64(0x0, 0x48);  //sp 

	//cpus_clear(phys_cpu_present_map);
	//cpu_set(0, phys_cpu_present_map);
	cpus_clear(cpu_possible_map);
	cpu_set(0, cpu_possible_map);

	__cpu_number_map[0] = 0;
	__cpu_logical_map[0] = 0;

	for (i = 1, num = 0; i < nr_cpu_loongson; i++) {
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
	init_cpu_present(&cpu_possible_map);
	per_cpu(cpu_state, smp_processor_id()) = CPU_ONLINE;
}

/*
 * Setup the PC, SP, and GP of a secondary processor and start it
 * running!
 */
void __cpuinit loongson3_boot_secondary(int cpu, struct task_struct *idle)
{
	int retval;

#ifdef CONFIG_LOONGSON2_CPUFREQ
	if(system_state != SYSTEM_BOOTING){
		struct cpufreq_freqs freqs;
		struct clk *cpuclk = clk_get(NULL, "cpu_clk");

		freqs.cpu   = 0;
		freqs.old   = cpuclk->rate;
		freqs.new   = cpu_clock_freq / 1000;
		freqs.flags = 0;

		cpufreq_enabled = 0;
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
		cpuclk->rate = cpu_clock_freq / 1000;
		LOONGSON_CHIPCFG0 |= 0x7;	/* Set to highest frequency */
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	}
#endif

	printk(KERN_DEBUG "\n BOOT CPU#%d...\n", cpu);
	retval = loongson3_cpu_start(cpu_logical_map(cpu), &smp_bootstrap,     
			       __KSTK_TOS(idle),                                   
			       (unsigned long)task_thread_info(idle), 0);
	if (retval != 0)
		printk(KERN_DEBUG "!!!!!!loongson3_cpu_start(%i) returned with err%i \n" , cpu, retval);
}


/*
 * Final cleanup after all secondaries booted
 */
void __init loongson3_cpus_done(void)
{
	disable_unused_cpus();
}

#ifdef CONFIG_HOTPLUG_CPU

static DEFINE_SPINLOCK(smp_reserve_lock);

extern void fixup_irqs(void);
extern void (*flush_cache_all)(void);

static int loongson3_cpu_disable(void)
{
	unsigned long flags;
	unsigned int cpu = smp_processor_id();

	if(cpu == 0) return -EBUSY;
	spin_lock(&smp_reserve_lock);
	cpu_clear(cpu, cpu_online_map);
	cpu_clear(cpu, cpu_callin_map);
	local_irq_save(flags);
	fixup_irqs();
	local_irq_restore(flags);
	flush_cache_all();
	local_flush_tlb_all();
	spin_unlock(&smp_reserve_lock);

	return 0;
}


static void loongson3_cpu_die(unsigned int cpu)
{
#ifdef CONFIG_LOONGSON2_CPUFREQ
	if(num_online_cpus() == 1)
		cpufreq_enabled = 1;
#endif

	while (per_cpu(cpu_state, cpu) != CPU_DEAD)
		cpu_relax();

	mb();
}

void play_dead_uncached(int *state_addr)
{
	__asm__ __volatile__(
		"      .set push                         \n" 
		"      .set noreorder                    \n" 
		"      li $t0, 0x80000000                \n" //KSEG0
		"      li $t1, 512                       \n" //num of entries
		"flushloop:                              \n"
		"      cache 0, 0($t0)                   \n"	
		"      cache 0, 1($t0)                   \n"	
		"      cache 0, 2($t0)                   \n"	
		"      cache 0, 3($t0)                   \n"	
		"      cache 1, 0($t0)                   \n"	
		"      cache 1, 1($t0)                   \n"	
		"      cache 1, 2($t0)                   \n"	
		"      cache 1, 3($t0)                   \n"
		"      addiu $t0, $t0, 0x20              \n"
		"      bnez  $t1, flushloop              \n"
		"      addiu $t1, $t1, -1                \n"
		"      li    $t0, 0x7                    \n" //*state_addr = CPU_DEAD; 
		"      sw    $t0, 0($a0)                 \n" 
		"      sync                              \n"
		"      cache 21, 0($a0)                  \n"	
		"      .set pop                          \n"); 

	__asm__ __volatile__(
		"      .set push                         \n" 
		"      .set noreorder                    \n" 
		"      .set mips64                       \n" 
		"      mfc0  $t2, $15, 1                 \n" 
		"      andi  $t2, 0x3ff                  \n" 
		"      .set mips3                        \n" 
		"      dli   $t0, 0x900000003ff01000     \n" 
		"      andi  $t3, $t2, 0x3               \n" 
		"      sll   $t3, 8                      \n"  //cpu id
		"      or    $t0, $t0, $t3               \n" 
		"      andi  $t1, $t2, 0xc               \n" 
		"      dsll  $t1, 42                     \n"  //node id
		"      or    $t0, $t0, $t1               \n" 
		"waitforinit:                            \n" 
		"      li    $a0, 0x100                  \n" 
		"idleloop:                               \n" 
		"      bnez  $a0, idleloop               \n" 
		"      addiu $a0, -1                     \n" 
		"      lw    $v0, 0x20($t0)              \n"   //PC
		"      nop                               \n" 
		"      beqz  $v0, waitforinit            \n" 
		"      nop                               \n" 
		"      ld    $sp, 0x28($t0)              \n"   //SP
		"      nop                               \n" 
		"      ld    $gp, 0x30($t0)              \n"   //GP
		"      nop                               \n" 
		"      ld    $a1, 0x38($t0)              \n" 
		"      nop                               \n" 
#if 1
		"      lui   $a0, 0x8000                 \n" //a0=KSEG0 a1=dcache_size a2=icache_size
		"      li    $a1, (1<<14)                \n"
		"      li    $a2, (1<<14)                \n"
		"      mtc0  $0, $29                     \n" //Init dcache $29=CP0_TAGHI
		"      mtc0  $0, $28                     \n" //$28=CP0_TAGLO
		"      li    $t0, 0x22                   \n"
		"      mtc0  $t0, $26                    \n" // $26=CP0_ECC
		"      addu  $t1, $0, $a0                \n"
		"      addu  $t2, $a0, $a2               \n"
		"1:    slt   $a3, $t1, $t2               \n"
		"      beq   $a3, $0, 1f                 \n"
		"      nop                               \n"
		"      cache 0x09, 0x0($t1)              \n" //0x09=Index_Store_Tag_D
		"      cache 0x09, 0x1($t1)              \n"
		"      cache 0x09, 0x2($t1)              \n"
		"      cache 0x09, 0x3($t1)              \n"
		"      beq   $0, $0, 1b                  \n"
		"      addiu $t1, $t1, 0x20              \n"
		"1:    addu  $t1, $0, $a0                \n" //Init icache
		"      addu  $t2, $a0, $a1               \n"
		"      mtc0  $0, $29                     \n"
		"      mtc0  $0, $28                     \n"
		"      mtc0  $0, $26                     \n"
		"1:    slt   $a3, $t1, $t2               \n"
		"      beq   $a3, $0, 1f                 \n"
		"      nop                               \n"
		"      cache 0x08, 0x0($t1)              \n" //0x08=Index_Store_Tag_I
		"      cache 0x08, 0x1($t1)              \n"
		"      cache 0x08, 0x2($t1)              \n"
		"      cache 0x08, 0x3($t1)              \n"
		"      beq   $0, $0, 1b                  \n"
		"      addiu $t1, $t1, 0x20              \n" //finish
#endif

		"1:    mtc0  $0, $6                      \n" //Init TLB  // $6=CP0_WIRED
		"      li    $t0, 0x3000                 \n"
		"      mtc0  $t0, $5                     \n"             // $5=CP0_PAGEMASK pagesize=16K 
		"      lui   $a0, 0x8000                 \n"  //a0=KSEG0 a1=tlbsize t0=index t1=addr
		"      addiu $a1, $0, 64                 \n"
		"      mtc0  $0, $2                      \n"             // $2=CP0_ENTRYLO0 
		"      mtc0  $0, $3                      \n"             // $3=CP0_ENTRYLO1
		"      mfc0  $t0, $6                     \n"             // $6=CP0_WIRED
		"      addu  $t1, $0, $a0                \n"
		"1:    sltu  $a3, $t0, $a1               \n"
		"      beq   $a3, $0, 1f                 \n"
		"      nop                               \n"
		"      mtc0  $t1, $10                    \n"            // $10=CP0_ENTRYHI 
		"      mtc0  $t0, $0                     \n"            // $0=CP0_INDEX
		"      tlbwi                             \n"
		"      addiu $t1, $t1, 0x8000            \n"  //32K
		"      beq   $0, $0, 1b                  \n"
		"      addiu $t0, $t0, 1                 \n"
		"1:    tlbp                              \n"  //TLB finish
		"      li    $t0, 4                      \n"
		"      mtc0  $t0, $22                    \n"
		"      nop                               \n" 
		"      jr  $v0                           \n" 
		"      nop                               \n"
		"      .set pop                          \n"); 
}

void play_dead(void)
{
	int *state_addr;
	unsigned int cpu = smp_processor_id();
	void (*func)(int *);

	idle_task_exit();
	func = CKSEG1ADDR(play_dead_uncached);
	state_addr = &per_cpu(cpu_state, cpu);
	mb();
	func(state_addr);
}

#define CPU_POST_DEAD_FROZEN	(CPU_POST_DEAD | CPU_TASKS_FROZEN)
static int __cpuinit loongson3_cpu_callback(struct notifier_block *nfb,
	unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;

	switch (action) {
	case CPU_POST_DEAD:
	case CPU_POST_DEAD_FROZEN:
		printk(KERN_DEBUG "Disable clock for CPU%d\n", cpu);
		LOONGSON_CHIPCFG0 &= ~(1 << (12 + cpu));
		break;
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		LOONGSON_CHIPCFG0 |= 1 << (12 + cpu);
		break;
	}

	return NOTIFY_OK;
}

static int __cpuinit register_loongson3_notifier(void)
{
	hotcpu_notifier(loongson3_cpu_callback, 0);
	return 0;
}
late_initcall(register_loongson3_notifier);

#endif

struct plat_smp_ops loongson3_smp_ops = {
	.send_ipi_single = loongson3_send_ipi_single,
	.send_ipi_mask = loongson3_send_ipi_mask,
	.init_secondary = loongson3_init_secondary,
	.smp_finish = loongson3_smp_finish,
	.cpus_done = loongson3_cpus_done,
	.boot_secondary = loongson3_boot_secondary,
	.smp_setup = loongson3_smp_setup,
	.prepare_cpus = loongson3_prepare_cpus,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable = loongson3_cpu_disable,
	.cpu_die = loongson3_cpu_die,
#endif
};
