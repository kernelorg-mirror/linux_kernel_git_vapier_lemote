/*
 * Copyright (C) 2007 Lemote Inc. & Insititute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/irq_cpu.h>
#include <asm/i8259.h>
#include <asm/mipsregs.h>
#include <asm/delay.h>
#include <asm/cevt-r4k.h>
#include <asm/hpet.h>
#include <asm/mips-boards/bonito64.h>
#include "irqregs.h"
#include "htregs.h"

extern void prom_printf(char *fmt, ...);
extern void loongson3_ipi_interrupt(struct pt_regs *regs);

#define __printf_HT_vector(A) \
static inline void _printf_HT_vector##A(void) \
{ \
	prom_printf("%x \n", HT_irq_vector_reg##A); \
}

__printf_HT_vector(0)
__printf_HT_vector(1)
__printf_HT_vector(2)
__printf_HT_vector(3)
__printf_HT_vector(4)
__printf_HT_vector(5)
__printf_HT_vector(6)
__printf_HT_vector(7)

void  printf_HT_vector(void)
{
	_printf_HT_vector0();
	_printf_HT_vector1();
	_printf_HT_vector2();
	_printf_HT_vector3();
	_printf_HT_vector4();
	_printf_HT_vector5();
	_printf_HT_vector6();
	_printf_HT_vector7();
}

asmlinkage void mach_irq_dispatch(unsigned int pending)
{
    	unsigned int irq;

    	if (pending & CAUSEF_IP7) {
    		do_IRQ(63);
#ifdef CONFIG_SMP
    	} else if (pending & CAUSEF_IP6 /* 0x4000 */){  //smp ipi
    		loongson3_ipi_interrupt(NULL);
#endif
    	} else if (pending & CAUSEF_IP2) { // For LPC
#ifdef CONFIG_CPU_UART
		//prom_printf("======NB\n");
         	do_IRQ(58);
#else
    		irq = *(volatile unsigned int*)(0xffffffffbfe00200 + 0x08);
    		if((irq & 0x10))
         		do_IRQ(58);
#endif
     	} else if (pending & CAUSEF_IP3) {
		irq = HT_irq_vector_reg0;
		HT_irq_vector_reg0 = irq;
		/*
		HT_irq_vector_reg1 = HT_irq_vector_reg1;
		HT_irq_vector_reg2 = HT_irq_vector_reg2;
		HT_irq_vector_reg3 = HT_irq_vector_reg3;
		HT_irq_vector_reg4 = HT_irq_vector_reg4;
		HT_irq_vector_reg5 = HT_irq_vector_reg5;
		HT_irq_vector_reg6 = HT_irq_vector_reg6;
		HT_irq_vector_reg7 = HT_irq_vector_reg7;
		*/

		if (irq & 0x1)
			do_IRQ(0);
		if ((irq & 0x8000)){
			do_IRQ(15);
		}
		if ((irq & 0x4000)){
			do_IRQ(14);
		}
		if (irq & 0x1000)
			do_IRQ(12);
		if (irq & 0x100)
			do_IRQ(8);
		if (irq & 0x80)  //daway 2011-03-11, SCI->IRQ7
			do_IRQ(7);
		if (irq & 0x40)
			do_IRQ(6);
		if (irq & 0x20)
			do_IRQ(5);
		if (irq & 0x10)
			do_IRQ(4);
		if (irq & 0x8)
			do_IRQ(3);
		if (irq & 0x2)
			do_IRQ(1);
		if (irq & (~(0x8000 | 0x4000 | 0x1000 | 0x100 | 0x80 | 0x40 | 0x20 | 0x10 | 0x8 | 0x2 | 0x1)))
			prom_printf("more interrupt from HT is %x\n", irq);
    	} else { //(pending &(~(CAUSEF_IP7 |CAUSEF_IP6|CAUSEF_IP3|CAUSEF_IP2)))
		prom_printf("spurious interrupt\n");
		spurious_interrupt();
	}
}

static struct irqaction cascade_irqaction = {
	.handler = no_action,
	.name = "cascade",
};

void irq_router_init(void)
{
	unsigned int t;

	/* Route the LPC interrupt to Core0 INT0 */
	INT_router_regs_lpc_int = 0x11;
	IO_control_regs_Intenset = (0x1<<10);//Enable lpc interrupts

	/* Route the HT interrupt to Core0 INT1 */
	INT_router_regs_HT1_int0 = 0x21;
	INT_router_regs_HT1_int1 = 0x21;
	INT_router_regs_HT1_int2 = 0x21;
	INT_router_regs_HT1_int3 = 0x21;
	INT_router_regs_HT1_int4 = 0x21;
	INT_router_regs_HT1_int5 = 0x21;
	INT_router_regs_HT1_int6 = 0x21;
	INT_router_regs_HT1_int7 = 0x21;
	/* Enable the all HT interrupt */
	//HT1
	HT_irq_enable_reg0 = 0xffffffff;
	HT_irq_enable_reg1 = 0x00000000;
	HT_irq_enable_reg2 = 0x00000000;
	HT_irq_enable_reg3 = 0x00000000;
	HT_irq_enable_reg4 = 0x00000000;
	HT_irq_enable_reg5 = 0x00000000;
	HT_irq_enable_reg6 = 0x00000000;
	HT_irq_enable_reg7 = 0x00000000;
	/* Enable the IO interrupt controller */ 
	t = IO_control_regs_Inten; 
	printk("the old IO inten is %x\n", t);
	IO_control_regs_Intenset = t | (0xffff << 16);
	t = IO_control_regs_Inten;
	printk("the new IO inten is %x\n", t);

#ifndef CONFIG_CPU_UART
	/* Enable the LPC interrupt */
	/* the all interrupt enable bit */
	*(volatile unsigned int*)(0xffffffffbfe00200 + 0x00) = 0x80000000;
	/* the 18-bit interrpt enable bit */ 
	*(volatile unsigned int*)(0xffffffffbfe00200 + 0x04) = 0x0;
#endif
}

void __init mach_init_irq(void)
{
	/*
	* Clear all of the interrupts while we change the able around a bit.
	* int-handler is not on bootstrap
	*/
	clear_c0_status(ST0_IM | ST0_BEV);
	local_irq_disable();

	irq_router_init();
	/* most bonito irq should be level triggered */
	/* 
	* Mask out all interrupt by writing "1" to all bit position in 
	* the interrupt reset reg. 
	*/

	/* init all controller
	*   0-15         ------> i8259 interrupt
	*   16-47        ------> bonito interrupt
	*   NR_IRQS - 1  ------> r4k timer interrupt
	*/
	/* Sets the first-level interrupt dispatcher. */

	mips_cpu_irq_init();	
	init_i8259_irqs();

	/* bonito irq at IP6 */
	/* 8259 irq at IP3 */
	setup_irq(56 + 3, &cascade_irqaction);
	/* open the serial port irq */
#ifndef CONFIG_CPU_UART
	set_c0_status(STATUSF_IP2);
#endif
	set_c0_status(STATUSF_IP6);
	printk("init_IRQ done\n");
}

void fixup_irqs(void)
{
	int irq;
	struct irq_desc *desc;
	cpumask_t new_affinity;
	unsigned long flags;
	int do_set_affinity;
	int cpu;

	cpu = smp_processor_id();

	for (irq = 0; irq < NR_IRQS; irq++) {
		desc = irq_to_desc(irq);

		if (desc->chip == &no_irq_chip)
			continue;

		/* Timer IRQ */
		if (irq == HPET_T0_IRQ)
			continue;

		if (irq == c0_compare_irqaction.irq) {
			clear_c0_status(STATUSF_IP7);
		}
		else {
			raw_spin_lock_irqsave(&desc->lock, flags);
			/*
			 * If this irq has an action, it is in use and
			 * must be migrated if it has affinity to this
			 * cpu.
			 */
			if (desc->action && cpumask_test_cpu(cpu, desc->affinity)) {
				if (cpumask_weight(desc->affinity) > 1) {
					/*
					 * It has multi CPU affinity,
					 * just remove this CPU from
					 * the affinity set.
					 */
					cpumask_copy(&new_affinity, desc->affinity);
					cpumask_clear_cpu(cpu, &new_affinity);
				} else {
					/*
					 * Otherwise, put it on lowest
					 * numbered online CPU.
					 */
					cpumask_clear(&new_affinity);
					cpumask_set_cpu(cpumask_first(cpu_online_mask), &new_affinity);
				}
				do_set_affinity = 1;
			} else {
				do_set_affinity = 0;
			}

			if (irq == 58) {
				INT_router_regs_lpc_int = 0x11;
				IO_control_regs_Intenset = (0x1<<10);//Enable lpc interrupts
			}
			raw_spin_unlock_irqrestore(&desc->lock, flags);

			if (do_set_affinity)
				irq_set_affinity(irq, &new_affinity);
		}
	}
	clear_c0_status(ST0_IM);
}

