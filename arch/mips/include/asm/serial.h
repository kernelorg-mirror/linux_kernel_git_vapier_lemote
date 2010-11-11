#ifndef _ASM_SERIAL_H
#define _ASM_SERIAL_H

#include <asm-generic/serial.h>

#if defined(CONFIG_CPU_LOONGSON3A)
#ifdef CONFIG_CPU_UART
	#define BASE_BAUD (33000000 / 16)
#else
	#define BASE_BAUD (3686400 / 16)
#endif
#endif

/* Standard COM flags (except for COM4, because of the 8514 problem) */
#ifdef CONFIG_SERIAL_DETECT_IRQ
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST | ASYNC_AUTO_IRQ)
#define STD_COM4_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_AUTO_IRQ)
#else
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)
#define STD_COM4_FLAGS ASYNC_BOOT_AUTOCONF
#endif

#if defined(CONFIG_CPU_LOONGSON3A)
#ifdef CONFIG_CPU_UART
#undef STD_SERIAL_PORT_DEFNS
#define STD_SERIAL_PORT_DEFNS \
	/* UART CLK   PORT IRQ     FLAGS        */				\
	{ .baud_base = BASE_BAUD, .irq = 58, 					\
	  .flags = STD_COM_FLAGS, .iomem_base = (u8*)(0xffffffffbfe001e0), 	\
	  .io_type = SERIAL_IO_MEM}						
#else //for HT LPC UART
#undef STD_SERIAL_PORT_DEFNS
#define STD_SERIAL_PORT_DEFNS \
	/* UART CLK   PORT IRQ     FLAGS        */				\
	{ .baud_base = BASE_BAUD, .irq = 58, 					\
	  .flags = STD_COM_FLAGS, .iomem_base = (u8*)(0xffffffffbff003f8), 	\
	  .io_type = SERIAL_IO_MEM}	    					

#endif
#endif

#define SERIAL_PORT_DFNS				\
	STD_SERIAL_PORT_DEFNS

#endif /* _ASM_SERIAL_H */
