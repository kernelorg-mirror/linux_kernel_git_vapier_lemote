#
# Makefile for ICT CAS loongson3a board.
#

obj-y	+= irq.o dbg_io.o ec_wpce775l.o platform.o loongson3_gpio.o cpu_temprature.o loongson_hwmon.o lvds-op.o
obj-$(CONFIG_I2C_PIIX4) += emc1412.o

obj-$(CONFIG_SMP)	+= smp.o

obj-$(CONFIG_NUMA)	+= numa.o

obj-$(CONFIG_SUSPEND) += pm.o sleep.o

obj-$(CONFIG_LOONGSON3_CPUAUTOPLUG) += loongson3_cpuautoplug.o

obj-$(CONFIG_RS780_HPET) += hpet.o
#EXTRA_AFLAGS := $(CFLAGS)
