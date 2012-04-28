#ifndef __ASM_SUSPEND_H
#define __ASM_SUSPEND_H

static inline int arch_prepare_suspend(void) { return 0; }

static inline int arch_finish_suspend(void)
{
#ifdef CONFIG_CPU_LOONGSON3
	extern void disable_unused_cpus(void);
	disable_unused_cpus();
#endif
	return 0;
}
/* References to section boundaries */
extern const void __nosave_begin, __nosave_end;

#endif /* __ASM_SUSPEND_H */
