/*
 * Cpuautoplug driver for the Loongson-3 processors
 *
 * Copyright (C) 2006 - 2011 Lemote Inc.
 * Author: Huacai Chen, chenhc@lemote.com
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/proc_fs.h>
#include <linux/kernel_stat.h>
#include <linux/platform_device.h>

#include <asm/clock.h>

#include <loongson.h>

/*
 * CPU Autoplug enabled ?
 */
static int autoplug_enabled __read_mostly  = 1;

#ifndef MODULE
/*
 * Enable / Disable CPU Autoplug
 */
static int __init setup_autoplug(char *str)
{
	if (!strcmp(str, "off"))
		autoplug_enabled = 0;
	else if (!strcmp(str, "on"))
		autoplug_enabled = 1;
	else
		return 0;
	return 1;
}

__setup("autoplug=", setup_autoplug);

#endif

static int autoplug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", autoplug_enabled);

	return 0;
}

static ssize_t
autoplug_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	char val[5];

	copy_from_user(val, buf, count);
	autoplug_enabled = simple_strtol(val, NULL, 0);

	return count;
}

static int autoplug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, autoplug_proc_show, NULL);
}

static const struct file_operations autoplug_proc_fops = {
	.open		= autoplug_proc_open,
	.read		= seq_read,
	.write		= autoplug_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct cpu_autoplug_info {
	cputime64_t prev_idle;
	cputime64_t prev_wall;
	struct delayed_work work;
	unsigned int sampling_rate;
	int dec_reqs;  /* continous core-decreasing requests */
};

struct cpu_autoplug_info ap_info;

static struct workqueue_struct *kautoplugd_wq;

static inline cputime64_t get_idle_time_jiffy(cputime64_t *wall)
{
	unsigned int cpu;
	cputime64_t idle_time = cputime64_zero;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	for_each_online_cpu(cpu) {
		busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
			kstat_cpu(cpu).cpustat.system);

		busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
		busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
		busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
		busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

		idle_time = cputime64_add(idle_time, cputime64_sub(cur_wall_time, busy_time));
	}

	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_idle_time(cputime64_t *wall)
{
	unsigned int cpu;
	u64 idle_time = 0;

	for_each_online_cpu(cpu) {
		idle_time += get_cpu_idle_time_us(cpu, wall);
		if (idle_time == -1ULL)
			return get_idle_time_jiffy(wall);
	}

	return idle_time;
}

static void increase_cores(int cur_cpus)
{
	int target_cpu;

	if(cur_cpus == NR_CPUS)
		return;

	target_cpu = cpumask_next_zero(0, cpu_online_mask);
	cpu_hotplug_driver_lock();
	cpu_up(target_cpu);
	cpu_hotplug_driver_unlock();
}


static void decrease_cores(int cur_cpus)
{
	int target_cpu;

	if(cur_cpus == 1)
		return;

	target_cpu = find_last_bit(cpumask_bits(cpu_online_mask), NR_CPUS);
	cpu_hotplug_driver_lock();
	cpu_down(target_cpu);
	cpu_hotplug_driver_unlock();
}

#define INC_THRESHOLD 95
#define DEC_THRESHOLD 10

static void do_autoplug_timer(struct work_struct *work)
{
	cputime64_t cur_wall_time, cur_idle_time;
	unsigned int idle_time, wall_time;
	int delay, load, nr_cpus = num_online_cpus();

	BUG_ON(smp_processor_id() != 0);
	delay = msecs_to_jiffies(ap_info.sampling_rate);
	if(!autoplug_enabled || system_state != SYSTEM_RUNNING)
		goto out;

	cur_idle_time = get_idle_time(&cur_wall_time);

	wall_time = (unsigned int) cputime64_sub(cur_wall_time, ap_info.prev_wall);
	ap_info.prev_wall = cur_wall_time;

	idle_time = (unsigned int) cputime64_sub(cur_idle_time, ap_info.prev_idle);
	idle_time += wall_time * (NR_CPUS - nr_cpus);
	ap_info.prev_idle = cur_idle_time;

	if (unlikely(!wall_time || wall_time * NR_CPUS < idle_time))
		goto out;

	load = 100 * (wall_time * NR_CPUS - idle_time) / wall_time;

	if(load < (nr_cpus - 1) * 100 - DEC_THRESHOLD) {
		if(ap_info.dec_reqs > 2){
			ap_info.dec_reqs = 0;
			decrease_cores(nr_cpus);
		}
		else
			ap_info.dec_reqs++;
	}
	else if(load > (nr_cpus - 1) * 100 + INC_THRESHOLD) {
		ap_info.dec_reqs = 0;
		increase_cores(nr_cpus);
	}
out:
	queue_delayed_work_on(0, kautoplugd_wq, &ap_info.work, delay);
}

static struct platform_device_id platform_device_ids[] = {
	{
		.name = "ls3_cpuautoplug",
	},
	{}
};

MODULE_DEVICE_TABLE(platform, platform_device_ids);

static struct platform_driver platform_driver = {
	.driver = {
		.name = "ls3_cpuautoplug",
		.owner = THIS_MODULE,
	},
	.id_table = platform_device_ids,
};

static int __init cpuautoplug_init(void)
{
	int ret, delay;

	/* Register platform stuff */
	proc_create("cpuautoplug", 0, NULL, &autoplug_proc_fops);
	ret = platform_driver_register(&platform_driver);
	if (ret)
		return ret;

	pr_info("cpuautoplug: Loongson-3A CPU autoplug driver.\n");

	ap_info.dec_reqs = 0; 
	ap_info.sampling_rate = 625;  /* 622 ms */
#ifndef MODULE
	delay = msecs_to_jiffies(ap_info.sampling_rate * 20);
#else
	delay = msecs_to_jiffies(ap_info.sampling_rate * 8);
#endif
	kautoplugd_wq = alloc_workqueue("kautoplug", WQ_NON_REENTRANT, 1);
	if (!kautoplugd_wq) {
		printk(KERN_ERR "Creation of kautoplugd failed\n");
		return -EFAULT;
	}
	INIT_DELAYED_WORK_DEFERRABLE(&ap_info.work, do_autoplug_timer);
	queue_delayed_work_on(0, kautoplugd_wq, &ap_info.work, delay);

	return ret;
}

static void __exit cpuautoplug_exit(void)
{
	cancel_delayed_work_sync(&ap_info.work);
	destroy_workqueue(kautoplugd_wq);
	platform_driver_unregister(&platform_driver);
}

late_initcall(cpuautoplug_init);
module_exit(cpuautoplug_exit);

MODULE_AUTHOR("Huacai Chen <chenhc@lemote.com>");
MODULE_DESCRIPTION("cpuautoplug driver for Loongson3A");
MODULE_LICENSE("GPL");
