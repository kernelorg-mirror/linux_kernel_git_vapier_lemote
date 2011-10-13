/* 
 * Driver for Loongson3A Laptop
 *
 * Copyright (C) 2011 Lemote Inc.
 * Author : Huangw Wei <huangw@lemote.com>
 *        : Wang Rui <wangr@lemote.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/interrupt.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/pm.h>
#include <linux/power_supply.h>
#include <linux/thermal.h>
#include <linux/workqueue.h>
#include <linux/video_output.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <asm/bootinfo.h>

#include <ec_wpce775l.h>

/* Copy from Linux 2.6.38 */
#define KEY_TOUCHPAD_TOGGLE	0x212
#define KEY_MODEM	248

/* Backlight */
#define MAX_BRIGHTNESS	9

/* Power supply */
#define BIT_BAT_POWER_ACIN		(1 << 0)
enum
{
	APM_AC_OFFLINE =	0,
	APM_AC_ONLINE,
	APM_AC_BACKUP,
	APM_AC_UNKNOWN =	0xff
};
enum
{
	APM_BAT_STATUS_HIGH =		0,
	APM_BAT_STATUS_LOW,
	APM_BAT_STATUS_CRITICAL,
	APM_BAT_STATUS_CHARGING,
	APM_BAT_STATUS_NOT_PRESENT,
	APM_BAT_STATUS_UNKNOWN =	0xff
};
/* Power info cached timeout */
#define POWER_INFO_CACHED_TIMEOUT	100	/* jiffies */

/* SCI device */
#define EC_SCI_DEV		"sci"	/* < 10 bytes. */
#define SCI_IRQ_NUM		0x07
#define GPIO_SIZE		256

const char *version = EC_VERSION;

/* Power information structure */
struct ls3anb_power_info
{
	/* AC insert or not */
	unsigned int ac_in;
	/* Battery insert or not */
	unsigned int bat_in;
	unsigned int health;

	/* Se use capacity for caculating the life and time */
	//unsigned int current_capacity;

	/* Battery designed capacity */
	unsigned int design_capacity;
	/* Battery designed voltage */
	unsigned int design_voltage;
	/* Battery capacity after full charged */
	unsigned int full_charged_capacity;
	/* Battery Manufacture Date */
	unsigned char manufacture_date[11];
	/* Battery Serial number */
	unsigned char serial_number[8];
	/* Battery Manufacturer Name, max 11 + 1(length) bytes */
	unsigned char manufacturer_name[12];
	/* Battery Device Name, max 7 + 1(length) bytes */
	unsigned char device_name[8];
	/* Battery Technology */
	unsigned int technology;
	/* Battery cell count */
	unsigned char cell_count_string[4];
	unsigned char cell_count;

	/* Battery dynamic charge/discharge voltage */
	unsigned int voltage_now;
	/* Battery dynamic charge/discharge average current */
	int current_now;
	int current_sign;
	int current_average;
	/* Battery current remaining capacity */
	unsigned int remain_capacity;
	/* Battery current remaining capacity percent */
	unsigned int remain_capacity_percent;
	/* Battery current temperature */
	unsigned int temperature;
	/* Battery current remaining time (AverageTimeToEmpty) */
	unsigned int remain_time;
	/* Battery current full charging time (averageTimeToFull) */
	unsigned int fullchg_time;
	/* Battery Status */
	unsigned int charge_status;
};

/* SCI device structure */
struct sci_device
{
	/* The sci number get from ec */
	unsigned char number;
	/* Sci count */
	unsigned char parameter;
	/* Irq relative */
	unsigned char irq;
	unsigned char irq_data;
	/* Device name */
	unsigned char name[10];
};
/* SCI device event structure */
struct sci_event
{
	int index;
	sci_handler handler;
};


/* Platform driver init handler */
static int __init ls3anb_init(void);
/* Platform driver exit handler */
static void __exit ls3anb_exit(void);
/* Platform device suspend handler */
static int ls3anb_suspend(struct platform_device * pdev, pm_message_t state);
/* Platform device resume handler */
static int ls3anb_resume(struct platform_device * pdev);
static ssize_t ls3anb_get_version(struct device_driver * driver, char * buf);

/* Camera control misc device open handler */
static int ls3anb_cam_misc_open(struct inode * inode, struct file * filp);
/* Camera control misc device release handler */
static int ls3anb_cam_misc_release(struct inode * inode, struct file * filp);
/* Camera control misc device read handler */
ssize_t ls3anb_cam_misc_read(struct file * filp,
			char __user * buffer, size_t size, loff_t * offset);
/* Camera control misc device write handler */
static ssize_t ls3anb_cam_misc_write(struct file * filp,
			const char __user * buffer, size_t size, loff_t * offset);

/* Backlight device set brightness handler */
static int ls3anb_set_brightness(struct backlight_device * pdev);
/* Backlight device get brightness handler */
static int ls3anb_get_brightness(struct backlight_device * pdev);

/* Hwmon device get fan pwm by manual */
static ssize_t ls3anb_get_fan_pwm_enable(struct device * dev,
			struct device_attribute * attr, char * buf);
/* Hwmon device set fan pwm by manual */
static ssize_t ls3anb_set_fan_pwm_enable(struct device * dev,
			struct device_attribute * attr, const char * buf,
			size_t count);
/* Hwmon device get pwm level */
static ssize_t ls3anb_get_fan_pwm(struct device * dev,
			struct device_attribute * attr, char * buf);
/* Hwmon device set pwm level */
static ssize_t ls3anb_set_fan_pwm(struct device * dev,
			struct device_attribute * attr, const char * buf,
			size_t count);
/* Hwmon device get fan rpm */
static ssize_t ls3anb_get_fan_rpm(struct device * dev,
			struct device_attribute * attr, char * buf);
/* Hwmon device get cpu temperature */
static ssize_t ls3anb_get_cpu_temp(struct device * dev,
			struct device_attribute * attr, char * buf);
/* Hwmon device get name */
static ssize_t ls3anb_get_hwmon_name(struct device * dev,
			struct device_attribute * attr, char * buf);

/* >>>Power management operation */
/* Update power_info->voltage value */
static void ls3anb_power_info_voltage_update(void);
/* Update power_info->current_now value */
static void ls3anb_power_info_current_now_update(void);
/* Update power_info->current_avg value */
static void ls3anb_power_info_current_avg_update(void);
/* Update power_info->temperature value */
static void ls3anb_power_info_temperature_update(void);
/* Update power_info->remain_capacity value */
static void ls3anb_power_info_capacity_now_update(void);
/* Update power_info->curr_cap value */
static void ls3anb_power_info_capacity_percent_update(void);
/* Update power_info->remain_time value */
static void ls3anb_power_info_remain_time_update(void);
/* Update power_info->fullchg_time value */
static void ls3anb_power_info_fullcharge_time_update(void);
/* Clear battery static information. */
static void ls3anb_power_info_battery_static_clear(void);
/* Get battery static information. */
static void ls3anb_power_info_battery_static_update(void);
/* Update power_status value */
static void ls3anb_power_info_power_status_update(void);
static void ls3anb_bat_get_string(unsigned char index, unsigned char *bat_string);
/* Power supply Battery get property handler */
static int ls3anb_bat_get_property(struct power_supply * pws,
			enum power_supply_property psp, union power_supply_propval * val);
/* Power supply AC get property handler */
static int ls3anb_ac_get_property(struct power_supply * pws,
			enum power_supply_property psp, union power_supply_propval * val);
/* <<<End power management operation */

/* SCI device pci driver init handler */
static int sci_pci_driver_init(void);
/* SCI device pci driver exit handler */
static void sci_pci_driver_exit(void);
/* SCI device pci driver init */
static int sci_pci_init(void);
/* SCI event routine handler */
static irqreturn_t ls3anb_sci_int_routine(int irq, void * dev_id);
/* SCI event handler */
void ls3anb_sci_event_handler(int event);
/* SCI device over temperature event handler */
static int ls3anb_over_temp_handler(int status);
/* SCI device Throttling the CPU event handler */
static int ls3anb_throttling_CPU_handler(int status);
/* SCI device AC event handler */
static int ls3anb_ac_handler(int status);
/* SCI device Battery event handler */
static int ls3anb_bat_handler(int status);
/* SCI device Battery low event handler */
static int ls3anb_bat_low_handler(int status);
/* SCI device Battery very low event handler */
static int ls3anb_bat_very_low_handler(int status);
/* SCI device LID event handler */
static int ls3anb_lid_handler(int status);

/* Hotkey device init handler */
static int ls3anb_hotkey_init(void);
/* Hotkey device exit handler */
static void ls3anb_hotkey_exit(void);
extern int ec_query_get_event_num(void);

/* Platform device ids table object */
static struct platform_device_id platform_device_ids[] = 
{
	{
		.name = "ls3a-laptop",
	},
	{}
};
MODULE_DEVICE_TABLE(platform, platform_device_ids);
/* Platform driver object */
static struct platform_driver platform_driver = 
{
	.driver = 
	{
		.name = "ls3a-laptop",
		.owner = THIS_MODULE,
	},
	.id_table = platform_device_ids,
#ifdef CONFIG_PM
	.suspend = ls3anb_suspend,
	.resume  = ls3anb_resume,
#endif /* CONFIG_PM */
};
static DRIVER_ATTR(version, S_IRUGO, ls3anb_get_version, NULL);

/* Camera control misc device object file operations */
static const struct file_operations ls3anb_cam_misc_fops =
{
	.open = ls3anb_cam_misc_open,
	.release = ls3anb_cam_misc_release,
	.read = ls3anb_cam_misc_read,
	.write = ls3anb_cam_misc_write
};
/* Camera control misc device object */
static struct miscdevice ls3anb_cam_misc_dev =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = "webcam",
	.fops = &ls3anb_cam_misc_fops
};

/* Backlight device object */
static struct backlight_device * ls3anb_backlight_dev = NULL;
/* Backlight device operations table object */
static struct backlight_ops ls3anb_backlight_ops =
{
	.get_brightness = ls3anb_get_brightness,
	.update_status =  ls3anb_set_brightness,
};

/* Hwmon device object */
static struct device * ls3anb_hwmon_dev = NULL;
/* Sensors */
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO,
			ls3anb_get_fan_rpm, NULL, 0);
static SENSOR_DEVICE_ATTR(pwm1, S_IRUGO | S_IWUSR,
			ls3anb_get_fan_pwm, ls3anb_set_fan_pwm, 0);
static SENSOR_DEVICE_ATTR(pwm1_enable, S_IRUGO | S_IWUSR,
			ls3anb_get_fan_pwm_enable, ls3anb_set_fan_pwm_enable, 0);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO,
			ls3anb_get_cpu_temp, NULL, 0);
static SENSOR_DEVICE_ATTR(name, S_IRUGO,
			ls3anb_get_hwmon_name, NULL, 0);
/* Hwmon attributes table */
static struct attribute * ls3anb_hwmon_attributes[] =
{
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_pwm1_enable.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_name.dev_attr.attr,
	NULL
};
/* Hwmon device attribute group */
static struct attribute_group ls3anb_hwmon_attribute_group =
{
	.attrs = ls3anb_hwmon_attributes,
};

/* Power info object */
static struct ls3anb_power_info * power_info = NULL;
/* Power supply Battery property object */
static enum power_supply_property ls3anb_bat_props[] =
{
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL, /* in uAh */
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY, /* in percents! */
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	POWER_SUPPLY_PROP_TIME_TO_FULL_AVG,
	/* Properties of type `const char *' */
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
};

/* Power supply Battery device object */
static struct power_supply ls3anb_bat =
{
	.name = "ls3anb-bat",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = ls3anb_bat_props,
	.num_properties = ARRAY_SIZE(ls3anb_bat_props),
	.get_property = ls3anb_bat_get_property,
};
/* Power supply AC property object */
static enum power_supply_property ls3anb_ac_props[] =
{
	POWER_SUPPLY_PROP_ONLINE,
};
/* Power supply AC device object */
static struct power_supply ls3anb_ac =
{
	.name = "ls3anb-ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = ls3anb_ac_props,
	.num_properties = ARRAY_SIZE(ls3anb_ac_props),
	.get_property = ls3anb_ac_get_property,
};

/* SCI device object */
static struct sci_device * ls3anb_sci_device = NULL;

/* SCI device event handler table */
static const struct sci_event se[] =
{
	[SCI_EVENT_NUM_LID] =				{INDEX_DEVICE_STATUS, ls3anb_lid_handler},
	[SCI_EVENT_NUM_SLEEP] =				{0, NULL},
	[SCI_EVENT_NUM_WLAN] =				{0, NULL},
	[SCI_EVENT_NUM_BRIGHTNESS_DN] =		{0, NULL},
	[SCI_EVENT_NUM_BRIGHTNESS_UP] =		{0, NULL},
	[SCI_EVENT_NUM_AUDIO_MUTE] =		{0, NULL},
	[SCI_EVENT_NUM_VOLUME_DN] =			{0, NULL},
	[SCI_EVENT_NUM_VOLUME_UP] =			{0, NULL},
	[SCI_EVENT_NUM_BLACK_SCREEN] =		{0, NULL},
	[SCI_EVENT_NUM_DISPLAY_TOGGLE] =	{0, NULL},
	[SCI_EVENT_NUM_3G] =				{0, NULL},
	[SCI_EVENT_NUM_SIM] =				{0, NULL},
	[SCI_EVENT_NUM_CAMERA] =			{0, NULL},
	[SCI_EVENT_NUM_TP] =				{0, NULL},
	[SCI_EVENT_NUM_OVERTEMP] =			{0, ls3anb_over_temp_handler},
	[SCI_EVENT_NUM_AC] =				{0, ls3anb_ac_handler},
	[SCI_EVENT_NUM_BAT] =				{INDEX_POWER_STATUS, ls3anb_bat_handler},
	[SCI_EVENT_NUM_BATL] =				{0, ls3anb_bat_low_handler},
	[SCI_EVENT_NUM_BATVL] =				{0, ls3anb_bat_very_low_handler},
	[SCI_EVENT_NUM_THROT] =				{0, ls3anb_throttling_CPU_handler},
};
/* Hotkey device object */
static struct input_dev * ls3anb_hotkey_dev = NULL;
/* Hotkey keymap object */
static const struct key_entry ls3anb_keymap[] = 
{
	{KE_SW,  SCI_EVENT_NUM_LID, { SW_LID } },
	{KE_KEY, SCI_EVENT_NUM_SLEEP, { KEY_SLEEP } }, /* Fn + ESC */
	{KE_KEY, SCI_EVENT_NUM_BRIGHTNESS_DN, { KEY_BRIGHTNESSDOWN } }, /* Fn + F2 */
	{KE_KEY, SCI_EVENT_NUM_BRIGHTNESS_UP, { KEY_BRIGHTNESSUP } }, /* Fn + F3 */
	{KE_KEY, SCI_EVENT_NUM_AUDIO_MUTE, { KEY_MUTE } }, /* Fn + F4 */
	{KE_KEY, SCI_EVENT_NUM_VOLUME_DN, { KEY_VOLUMEDOWN } }, /* Fn + F5 */
	{KE_KEY, SCI_EVENT_NUM_VOLUME_UP, { KEY_VOLUMEUP } }, /* Fn + F6 */
	{KE_KEY, SCI_EVENT_NUM_BLACK_SCREEN, { KEY_DISPLAYTOGGLE } }, /* Fn + F7 */
	{KE_KEY, SCI_EVENT_NUM_DISPLAY_TOGGLE, { KEY_SWITCHVIDEOMODE } }, /* Fn + F8 */
	{KE_KEY, SCI_EVENT_NUM_3G, { KEY_MODEM } }, /* Fn + F9 */
	{KE_KEY, SCI_EVENT_NUM_CAMERA, { KEY_CAMERA } }, /* Fn + F10 */
	{KE_KEY, SCI_EVENT_NUM_TP, { KEY_TOUCHPAD_TOGGLE } }, /* Fn + F11 */
	{KE_END, 0 }
};


/* Platform driver init handler */
static int __init ls3anb_init(void)
{
	int ret;

	if (mips_machtype != MACH_LEMOTE_3A_A1004)
		return -1;

	printk(KERN_INFO "LS3ANB Driver : Load Platform Specific Driver V%s.\n", version);

	/* Register platform stuff START */
	ret = platform_driver_register(&platform_driver);
	if(ret)
	{
		printk(KERN_ERR "LS3ANB Driver : Fail to register ls3anb laptop platform driver.\n");
		return ret;
	}
	ret = driver_create_file(&platform_driver.driver, &driver_attr_version);
	/* Register platform stuff END */
	
	/* Register backlight START */
	ls3anb_backlight_dev = backlight_device_register("lemote",
				NULL, NULL, &ls3anb_backlight_ops, NULL);
	if(IS_ERR(ls3anb_backlight_dev))
	{
		ret = PTR_ERR(ls3anb_backlight_dev);
		goto fail_backlight_device_register;
	}
	ls3anb_backlight_dev->props.max_brightness = ec_read(INDEX_DISPLAY_MAXBRIGHTNESS_LEVEL);
	ls3anb_backlight_dev->props.brightness = ec_read(INDEX_DISPLAY_BRIGHTNESS);
	backlight_update_status(ls3anb_backlight_dev);
	/* Register backlight END */

	/* Register power supply START */
	power_info = kzalloc(sizeof(struct ls3anb_power_info), GFP_KERNEL);
	if(!power_info)
	{
		printk(KERN_ERR "LS3ANB Driver: Alloc memory for power_info failed!\n");
		ret = -ENOMEM;
		goto fail_power_info_alloc;
	}

	ls3anb_power_info_power_status_update();
	if(power_info->bat_in)
 	{
		/* Get battery static information. */
		ls3anb_power_info_battery_static_update();
	}
	else
	{
		printk(KERN_ERR "LS3ANB Driver: The battery does not exist!!\n");
	}
	ret = power_supply_register(NULL, &ls3anb_bat);
	if(ret)
	{
		ret = -ENOMEM;
		goto fail_bat_power_supply_register;
	}

	ret = power_supply_register(NULL, &ls3anb_ac);
	if(ret)
	{
		ret = -ENOMEM;
		goto fail_ac_power_supply_register;
	}
	/* Register power supply END */

	/* Register sensors START */
	ls3anb_hwmon_dev = hwmon_device_register(NULL);
	if(IS_ERR(ls3anb_hwmon_dev))
	{
		ret = -ENOMEM;
		goto fail_hwmon_device_register;
	}
	ret = sysfs_create_group(&ls3anb_hwmon_dev->kobj,
				&ls3anb_hwmon_attribute_group);
	if(ret)
	{
		ret = -ENOMEM;
		goto fail_sysfs_create_group_hwmon;
	}
	/* Register sensors END */

	/* Hotkey device START */
	ret = ls3anb_hotkey_init();
	if(ret)
	{
		printk(KERN_ERR "LS3ANB Driver : Fail to register hotkey device.\n");
		goto fail_hotkey_init;
	}
	/* Hotkey device END */

	/* SCI PCI Driver Init START  */
	ret = sci_pci_driver_init();
	if(ret)
	{
		printk(KERN_ERR "LS3ANB Driver : Fail to register sci pci driver.\n");
		goto fail_sci_pci_driver_init;
	}
	/* SCI PCI Driver Init END */

	/* Camera control misc Device START */
	ret = misc_register(&ls3anb_cam_misc_dev);
	if(ret)
	{
		printk(KERN_ERR "LS3ANB Driver : Fail to register camera control misc device.\n");
		goto fail_misc_register;
	}
	/* Camera control misc Device END */

	/* Request control for backlight device START */
	ec_write(INDEX_BACKLIGHT_CTRLMODE, BACKLIGHT_CTRL_BYHOST);
	/* Request control for backlight device END */

	return 0;

fail_misc_register:
	sci_pci_driver_exit();
fail_sci_pci_driver_init:
	ls3anb_hotkey_exit();
fail_hotkey_init:
	sysfs_remove_group(&ls3anb_hwmon_dev->kobj,
				&ls3anb_hwmon_attribute_group);
fail_sysfs_create_group_hwmon:
	hwmon_device_unregister(ls3anb_hwmon_dev);
fail_hwmon_device_register:
	power_supply_unregister(&ls3anb_ac);
fail_ac_power_supply_register:
	power_supply_unregister(&ls3anb_bat);
fail_bat_power_supply_register:
	kfree(power_info);
fail_power_info_alloc:
	backlight_device_unregister(ls3anb_backlight_dev);
fail_backlight_device_register:
	platform_driver_unregister(&platform_driver);

	return ret;
}

/* Platform driver exit handler */
static void __exit ls3anb_exit(void)
{
	free_irq(ls3anb_sci_device->irq, ls3anb_sci_device);

	/* Return control for backlight device START */
	ec_write(INDEX_BACKLIGHT_CTRLMODE, BACKLIGHT_CTRL_BYEC);
	/* Return control for backlight device END */

	/* Camera control misc device */
	misc_deregister(&ls3anb_cam_misc_dev);

	/* Hotkey & SCI device */
	sci_pci_driver_exit();
	ls3anb_hotkey_exit();

	/* Power supply */
	power_supply_unregister(&ls3anb_ac);
	power_supply_unregister(&ls3anb_bat);
	kfree(power_info);

	/* Sensors */
	sysfs_remove_group(&ls3anb_hwmon_dev->kobj,
				&ls3anb_hwmon_attribute_group);
	hwmon_device_unregister(ls3anb_hwmon_dev);

	/* Backlight */
	backlight_device_unregister(ls3anb_backlight_dev);

	/* Platform device & driver */
	platform_driver_unregister(&platform_driver);

	printk(KERN_INFO "LS3ANB Driver : Unload Platform Specific Driver.\n");
}

#ifdef CONFIG_PM
/* Platform device suspend handler */
static int ls3anb_suspend(struct platform_device * pdev, pm_message_t state)
{
	struct pci_dev *dev;

	dev = pci_get_device(PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SBX00_SMBUS, NULL);
	pci_disable_device(dev);

	return 0;
}

/* Platform device resume handler */
static int ls3anb_resume(struct platform_device * pdev)
{
	struct pci_dev *dev;

	dev = pci_get_device(PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SBX00_SMBUS, NULL);
	pci_enable_device(dev);

	return 0;
}
#else
static int ls3anb_suspend(struct platform_device * pdev, pm_message_t state)
{
	return 0;
}

static int ls3anb_resume(struct platform_device * pdev)
{
	return 0;
}
#endif /* CONFIG_PM */

static ssize_t ls3anb_get_version(struct device_driver * driver, char * buf)
{
	return sprintf(buf, "%s\n", version);
}
 
/* Camera control misc device open handler */
static int ls3anb_cam_misc_open(struct inode * inode, struct file * filp)
{
	return 0;
}

/* Camera control misc device release handler */
static int ls3anb_cam_misc_release(struct inode * inode, struct file * filp)
{
	return 0;
}

/* Camera control misc device read handler */
ssize_t ls3anb_cam_misc_read(struct file * filp,
			char __user * buffer, size_t size, loff_t * offset)
{
	int ret = 0;

	if(0 != *offset)
	  return 0;

	ret = ec_read(INDEX_CAM_STSCTRL);
	ret = sprintf(buffer, "%d\n", ret);
	*offset = ret;
	
	return ret;
}

/* Camera control misc device write handler */
static ssize_t ls3anb_cam_misc_write(struct file * filp,
			const char __user * buffer, size_t size, loff_t * offset)
{
	if(0 >= size)
	  return -EINVAL;

	if('0' == buffer[0])
	  ec_write(INDEX_CAM_STSCTRL, CAM_STSCTRL_OFF);
	else
	  ec_write(INDEX_CAM_STSCTRL, CAM_STSCTRL_ON);

	return size;
}

/* Backlight device set brightness handler */
static int ls3anb_set_brightness(struct backlight_device * pdev)
{
	unsigned int level = 0;

	level = ((FB_BLANK_UNBLANK==pdev->props.fb_blank) &&
				(FB_BLANK_UNBLANK==pdev->props.power)) ?
					pdev->props.brightness : 0;

	if(MAX_BRIGHTNESS < level)
	{
		level = MAX_BRIGHTNESS;
	}
	else if(level < 0)
	{
		level = 0;
	}

	ec_write(INDEX_DISPLAY_BRIGHTNESS, level);

	return 0;
}

/* Backlight device get brightness handler */
static int ls3anb_get_brightness(struct backlight_device * pdev)
{
	/* Read level from ec */
	return ec_read(INDEX_DISPLAY_BRIGHTNESS);
}

/* Hwmon device get fan pwm by manual */
static ssize_t ls3anb_get_fan_pwm_enable(struct device * dev,
			struct device_attribute * attr, char * buf)
{
	return sprintf(buf, "%d\n", ec_read(INDEX_FAN_CTRLMOD));
}

/* Hwmon device set fan pwm by manual */
static ssize_t ls3anb_set_fan_pwm_enable(struct device * dev,
			struct device_attribute * attr, const char * buf,
			size_t count)
{
	int value = 0;

	if(!count)
	{
		return 0;
	}
	if(1 != sscanf(buf, "%i", &value))
	{
		return -EINVAL;
	}

	if(value)
	{
		ec_write(INDEX_FAN_CTRLMOD,FAN_CTRL_BYHOST);
	}
	else
	{
		ec_write(INDEX_FAN_CTRLMOD,FAN_CTRL_BYEC);
	}

	return count;
}

/* Hwmon device get pwm level */
static ssize_t ls3anb_get_fan_pwm(struct device * dev,
			struct device_attribute * attr, char * buf)
{
	return sprintf(buf, "%d\n", ec_read(INDEX_FAN_SPEED_LEVEL));
}

/* Hwmon device set pwm level */
static ssize_t ls3anb_set_fan_pwm(struct device * dev,
			struct device_attribute * attr, const char * buf,
			size_t count)
{
	int value = 0;
	int status = 0;

	if(!count)
	{
		return 0;
	}
	if(1 != sscanf(buf, "%i", &value))
	{
		return -EINVAL;
	}

	status = ec_read(INDEX_FAN_CTRLMOD);
	if(FAN_CTRL_BYEC == status)
	{
		ec_write(INDEX_FAN_CTRLMOD, FAN_CTRL_BYHOST);
	}
	ec_write(INDEX_FAN_SPEED_LEVEL, value);

	return count;
}

/* Hwmon device get fan rpm */
static ssize_t ls3anb_get_fan_rpm(struct device * dev,
			struct device_attribute * attr, char * buf)
{
	return sprintf(buf, "%d\n", ((ec_read(INDEX_FAN_SPEED_HIGH) << 8) |
				ec_read(INDEX_FAN_SPEED_LOW)));
}

/* Hwmon device get cpu temperature */
static ssize_t ls3anb_get_cpu_temp(struct device * dev,
			struct device_attribute * attr, char * buf)
{
	int value = 0;

	value = ec_read(INDEX_TEMPERATURE_VALUE);

	return sprintf(buf, "%d\n", value);
}

/* Hwmon device get name */
static ssize_t ls3anb_get_hwmon_name(struct device * dev,
			struct device_attribute * attr, char * buf)
{
	return sprintf(buf, "ls3a-laptop\n");
}

/* Update power_info->voltage value */
static void ls3anb_power_info_voltage_update(void)
{
	short voltage_now = 0;
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	ls3anb_power_info_power_status_update();
	
	voltage_now = (ec_read(INDEX_BATTERY_VOL_HIGH) << 8) | ec_read(INDEX_BATTERY_VOL_LOW);

	power_info->voltage_now = (power_info->bat_in) ? voltage_now : 0;
}

/* Update power_info->current_now value */
static void ls3anb_power_info_current_now_update(void)
{
	short current_now = 0;
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	ls3anb_power_info_power_status_update();
	
	current_now = (ec_read(INDEX_BATTERY_CURRENT_HIGH) << 8) | ec_read(INDEX_BATTERY_CURRENT_LOW);

	power_info->current_now = (power_info->bat_in) ? current_now : 0;
}


/* Update power_info->current_avg value */
static void ls3anb_power_info_current_avg_update(void)
{
	short current_avg = 0;
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	ls3anb_power_info_power_status_update();
	
	current_avg = (ec_read(INDEX_BATTERY_AC_HIGH) << 8) | ec_read(INDEX_BATTERY_AC_LOW);

	power_info->current_average = (power_info->bat_in) ? current_avg : 0;
}

/* Update power_info->temperature value */
static void ls3anb_power_info_temperature_update(void)
{
	short temperature = 0;
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	ls3anb_power_info_power_status_update();

	temperature = (ec_read(INDEX_BATTERY_TEMP_HIGH) << 8) | ec_read(INDEX_BATTERY_TEMP_LOW);

	power_info->temperature = (power_info->bat_in) ?
				(temperature / 10 - 273) : 0;
}

/* Update power_info->remain_capacity value */
static void ls3anb_power_info_capacity_now_update(void)
{
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	power_info->remain_capacity = (ec_read(INDEX_BATTERY_RC_HIGH) << 8) | ec_read(INDEX_BATTERY_RC_LOW);
}


/* Update power_info->curr_cap value */
static void ls3anb_power_info_capacity_percent_update(void)
{
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	power_info->remain_capacity_percent = ec_read(INDEX_BATTERY_CAPACITY);
}

/* Update power_info->remain_time value */
static void ls3anb_power_info_remain_time_update(void)
{
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	power_info->remain_time = (ec_read(INDEX_BATTERY_ATTE_HIGH) << 8) | ec_read(INDEX_BATTERY_ATTE_LOW);
}

/* Update power_info->fullchg_time value */
static void ls3anb_power_info_fullcharge_time_update(void)
{
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	power_info->fullchg_time = (ec_read(INDEX_BATTERY_ATTF_HIGH) << 8) | ec_read(INDEX_BATTERY_ATTF_LOW);
}

/* Clear battery static information. */
static void ls3anb_power_info_battery_static_clear(void)
{
	strcpy(power_info->manufacturer_name, "Unknown");
	strcpy(power_info->device_name, "Unknown");
	power_info->technology = POWER_SUPPLY_TECHNOLOGY_UNKNOWN; 
	strcpy(power_info->serial_number, "Unknown");
	strcpy(power_info->manufacture_date, "Unknown");
	power_info->cell_count = 0;
	power_info->design_capacity = 0;
	power_info->design_voltage = 0;
	power_info->full_charged_capacity = 0;
}

/* Get battery static information. */
static void ls3anb_power_info_battery_static_update(void)
{
	unsigned int manufacture_date, bat_serial_number;
	char device_chemistry[5];

	manufacture_date = (ec_read(INDEX_BATTERY_MFD_HIGH) << 8) | ec_read(INDEX_BATTERY_MFD_LOW);
	sprintf(power_info->manufacture_date, "%d-%d-%d", (manufacture_date >> 9) + 1980,
            (manufacture_date & 0x01E0) >> 5, manufacture_date & 0x001F);
	ls3anb_bat_get_string(INDEX_BATTERY_MFN_LENG, power_info->manufacturer_name);
	ls3anb_bat_get_string(INDEX_BATTERY_DEVNAME_LENG, power_info->device_name);
	ls3anb_bat_get_string(INDEX_BATTERY_DEVCHEM_LENG, device_chemistry);
	if((device_chemistry[2] == 'o') || (device_chemistry[2] == 'O'))
	{
		power_info->technology = POWER_SUPPLY_TECHNOLOGY_LION; 
	}
	else if(((device_chemistry[1] = 'h') && (device_chemistry[2] == 'm')) ||
			((device_chemistry[1] = 'H') && (device_chemistry[2] == 'M')))
	{
		power_info->technology = POWER_SUPPLY_TECHNOLOGY_NiMH; 
	}
	else if((device_chemistry[2] == 'p') || (device_chemistry[2] == 'P'))
	{
		power_info->technology = POWER_SUPPLY_TECHNOLOGY_LIPO; 
	}
	else if((device_chemistry[2] == 'f') || (device_chemistry[2] == 'F'))
	{
		power_info->technology = POWER_SUPPLY_TECHNOLOGY_LiFe; 
	}
	else if((device_chemistry[2] == 'c') || (device_chemistry[2] == 'C'))
	{
		power_info->technology = POWER_SUPPLY_TECHNOLOGY_NiCd; 
	}
	else if(((device_chemistry[1] = 'n') && (device_chemistry[2] == 'm')) ||
			((device_chemistry[1] = 'N') && (device_chemistry[2] == 'M')))
	{
		power_info->technology = POWER_SUPPLY_TECHNOLOGY_LiMn; 
	}
	else
	{
		power_info->technology = POWER_SUPPLY_TECHNOLOGY_UNKNOWN; 
	}

	bat_serial_number = (ec_read(INDEX_BATTERY_SN_HIGH) << 8) | ec_read(INDEX_BATTERY_SN_LOW);
	snprintf(power_info->serial_number, 8, "%x", bat_serial_number);

	ls3anb_bat_get_string(INDEX_BATTERY_CELLCNT_START, power_info->cell_count_string);
	power_info->cell_count = (!strncmp(power_info->cell_count_string, FLAG_BAT_CELL_3S1P, 4)) ? 3 : 0;

	power_info->design_capacity = (ec_read(INDEX_BATTERY_DC_HIGH) << 8) | ec_read(INDEX_BATTERY_DC_LOW);
	power_info->design_voltage = (ec_read(INDEX_BATTERY_DV_HIGH) << 8) | ec_read(INDEX_BATTERY_DV_LOW);
	power_info->full_charged_capacity = (ec_read(INDEX_BATTERY_FCC_HIGH) << 8) | ec_read(INDEX_BATTERY_FCC_LOW);
	printk(KERN_INFO "LS3ANB Battery Information:\nManufacturerName: %s, DeviceName: %s, DeviceChemistry: %s\n",
			power_info->manufacturer_name, power_info->device_name, device_chemistry);
	printk(KERN_INFO "SerialNumber: %s, ManufactureDate: %s, CellNumber: %d\n",
			power_info->serial_number, power_info->manufacture_date, power_info->cell_count);
	printk(KERN_INFO "DesignCapacity: %dmAh, DesignVoltage: %dmV, FullChargeCapacity: %dmAh\n",
			power_info->design_capacity, power_info->design_voltage, power_info->full_charged_capacity);
}

/* Update power_status value */
static void ls3anb_power_info_power_status_update(void)
{
	unsigned int power_status = 0;
	static unsigned long last_jiffies = 0;

	if(POWER_INFO_CACHED_TIMEOUT > (jiffies - last_jiffies))
	{
		return;
	}
	last_jiffies = jiffies;

	power_status = ec_read(INDEX_POWER_STATUS);

	power_info->ac_in = (power_status & MASK(BIT_POWER_ACPRES)) ?
					APM_AC_ONLINE : APM_AC_OFFLINE;

	power_info->bat_in = (power_status & MASK(BIT_POWER_BATPRES)) ? 1 : 0;

	power_info->health = (power_info->bat_in) ?	POWER_SUPPLY_HEALTH_GOOD :
							POWER_SUPPLY_HEALTH_UNKNOWN;
	if(power_status & (MASK(BIT_POWER_BATL) | MASK(BIT_POWER_BATVL)))
	{
		power_info->health = POWER_SUPPLY_HEALTH_DEAD;
	}

	if(!power_info->bat_in)
	{
		power_info->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
	}
	else
	{
		if(power_status & MASK(BIT_POWER_BATFCHG))
		{
			power_info->charge_status = POWER_SUPPLY_STATUS_FULL;
		}
		else if(power_status & MASK(BIT_POWER_BATCHG))
		{
			power_info->charge_status = POWER_SUPPLY_STATUS_CHARGING;
		}
		else if(power_status & MASK(BIT_POWER_TERMINATE))
		{
			power_info->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		}
		else
		{
			power_info->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
		}
	}
}

/* Get battery static information string */
static void ls3anb_bat_get_string(unsigned char index, unsigned char *bat_string)
{
	unsigned char length, i;

	if(index == INDEX_BATTERY_CELLCNT_START)
	{
		length = BATTERY_CELLCNT_LENG;
		index--;
	}
	else
	{
		length = ec_read(index);
	}
	for(i = 0; i < length; i++)
	{
		*bat_string++ = ec_read(++index);
	}
	*bat_string = '\0';
}

/* Power supply Battery get property handler */
static int ls3anb_bat_get_property(struct power_supply * pws,
			enum power_supply_property psp, union power_supply_propval * val)
{
	switch(psp)
	{
		/* Get battery static information. */
		case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
			val->intval = power_info->design_voltage * 1000; /* mV -> uV */
			break;
		case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
			val->intval = power_info->design_capacity * 1000; /* mAh -> uAh */
			break;
		case POWER_SUPPLY_PROP_CHARGE_FULL:
			val->intval = power_info->full_charged_capacity * 1000;/* mAh -> uAh */
			break;
		case POWER_SUPPLY_PROP_MODEL_NAME:
			val->strval = power_info->device_name;
			break;
		case POWER_SUPPLY_PROP_MANUFACTURER:
			val->strval = power_info->manufacturer_name;
			break;
		case POWER_SUPPLY_PROP_SERIAL_NUMBER:
			val->strval = power_info->serial_number;
			break;
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = power_info->technology;
			break;

			/* Get battery dynamic information. */
		case POWER_SUPPLY_PROP_STATUS:
			ls3anb_power_info_power_status_update();
			val->intval = power_info->charge_status;
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			ls3anb_power_info_power_status_update();
			val->intval = power_info->bat_in; 
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			ls3anb_power_info_power_status_update();
			val->intval = power_info->health;
			break;
		case POWER_SUPPLY_PROP_CURRENT_NOW:
			ls3anb_power_info_current_now_update();
			val->intval = power_info->current_now * 1000; /* mA -> uA */
			break;
		case POWER_SUPPLY_PROP_CURRENT_AVG:
			ls3anb_power_info_current_avg_update();
			val->intval = power_info->current_average * 1000; /* mA -> uA */
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			ls3anb_power_info_voltage_update();
			val->intval =  power_info->voltage_now * 1000; /* mV -> uV */
			break;
		case POWER_SUPPLY_PROP_CHARGE_NOW:
			ls3anb_power_info_capacity_now_update();
			val->intval = power_info->remain_capacity * 1000; /* mAh -> uAh */
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			ls3anb_power_info_capacity_percent_update();
			val->intval = power_info->remain_capacity_percent;	/* Percentage */
			break;	
		case POWER_SUPPLY_PROP_TEMP:
			ls3anb_power_info_temperature_update();
			val->intval = power_info->temperature;	 /* Celcius */
			break;
		case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG: 
			ls3anb_power_info_remain_time_update();
			if(power_info->remain_time == 0xFFFF)
			{
				power_info->remain_time = 0;
			}
			val->intval = power_info->remain_time * 60;  /* seconds */
			break;
		case POWER_SUPPLY_PROP_TIME_TO_FULL_AVG: 
			ls3anb_power_info_fullcharge_time_update();
			if(power_info->fullchg_time == 0xFFFF)
			{
				power_info->fullchg_time = 0;
			}
			val->intval = power_info->fullchg_time * 60;  /* seconds */
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

/* Power supply AC get property handler */
static int ls3anb_ac_get_property(struct power_supply * pws,
			enum power_supply_property psp, union power_supply_propval * val)
{
	switch(psp)
	{
		case POWER_SUPPLY_PROP_ONLINE:
			ls3anb_power_info_power_status_update();
			val->intval = power_info->ac_in;
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

/* SCI device pci driver init handler */
static int sci_pci_driver_init(void)
{
	int ret;

	ret = sci_pci_init();
	if(ret)
	{
		printk(KERN_ERR "LS3ANB Drvier : Register pci driver error.\n");

		return ret;
	}

	printk(KERN_INFO "LS3ANB Driver : SCI event handler on WPCE775L Embedded Controller init.\n");

	return ret;
}

/* SCI device pci driver exit handler */
static void sci_pci_driver_exit(void)
{
	printk(KERN_INFO "LS3ANB Driver : SCI event handler on WPCE775L Embedded Controll exit.\n");
}

/* SCI device pci driver init */
static int sci_pci_init(void)
{
	int ret = -EIO;
	struct pci_dev *pdev;

	pdev = pci_get_device(PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SBX00_SMBUS, NULL);

	/* Create the sci device */
	ls3anb_sci_device = kmalloc(sizeof(struct sci_device), GFP_KERNEL);
	if(NULL == ls3anb_sci_device)
	{
		printk(KERN_ERR "LS3ANB Drvier : Malloc memory for sci_device failed!\n");

		return -ENOMEM;
	}
	
	/* Fill sci device */
	ls3anb_sci_device->irq = SCI_IRQ_NUM;
	ls3anb_sci_device->irq_data = 0x00;
	ls3anb_sci_device->number = 0x00;
	ls3anb_sci_device->parameter = 0x00;
	strcpy(ls3anb_sci_device->name, EC_SCI_DEV);

	/* Enable pci device and get the GPIO resources. */
	ret = pci_enable_device(pdev);
	if(ret)
	{
		printk(KERN_ERR "LS3ANB Driver : Enable pci device failed!\n");
		ret = -ENODEV;
		goto out_pdev;
	}

	/* Clear sci status: GPM9Status field in bit14
	 * of EVENT_STATUS register for SB710, write to
	 * 1 clear. */
    clean_ec_event_status();

	/* Alloc the interrupt for sci not pci */
	ret = request_irq(ls3anb_sci_device->irq, ls3anb_sci_int_routine,
				IRQF_SHARED, ls3anb_sci_device->name, ls3anb_sci_device);
	if(ret)
	{
		printk(KERN_ERR "LS3ANB Driver : Request irq %d failed!\n", ls3anb_sci_device->irq);
		ret = -EFAULT;
		goto out_irq;
	}

	ret = 0;
	printk(KERN_DEBUG "LS3ANB Driver : PCI Init successful!\n");
	goto out;

out_irq:
	pci_disable_device(pdev);
out_pdev:
	kfree(ls3anb_sci_device);
out:
	return ret;
}

/* SCI event routine handler */
static irqreturn_t ls3anb_sci_int_routine(int irq, void * dev_id)
{
	int event;

	//printk(KERN_CRIT "LS3ANB Driver : Entry sci_int_routine...\n");
	if(ls3anb_sci_device->irq != irq)
	{
		return IRQ_NONE;
	}

	event = ec_query_get_event_num();
	//printk(KERN_CRIT "LS3ANB Driver : Entry sci_int_routine(): event = 0x%x\n", event);
	if((SCI_EVENT_NUM_START > event) || (SCI_EVENT_NUM_END < event))
	{
        goto exit_event_action;
	}

	/* Do event action */
	ls3anb_sci_event_handler(event);

	/* Clear sci status: GPM9Status field in bit14
	 * of EVENT_STATUS register for SB710, write to
	 * 1 clear. */
    clean_ec_event_status();

	return IRQ_HANDLED;

exit_event_action:
    clean_ec_event_status();
	return IRQ_NONE;
}
 
/* SCI device event handler */
void ls3anb_sci_event_handler(int event)
{
	int status = 0;
	struct key_entry * ke = NULL;
	struct sci_event * sep = NULL;

	sep = (struct sci_event*)&(se[event]);
	if(0 != sep->index)
	{
		status = ec_read(sep->index);
	}
	if(NULL != sep->handler)
	{
		status = sep->handler(status);
	}

	ke = sparse_keymap_entry_from_scancode(ls3anb_hotkey_dev, event);
	if(ke)
	{
		if(SW_LID == ke->keycode)
		{
			// report LID event.
			input_report_switch(ls3anb_hotkey_dev, SW_LID, status);
			input_sync(ls3anb_hotkey_dev);
		}
		else
		{
			sparse_keymap_report_entry(ls3anb_hotkey_dev, ke, 1, true);
		}
	}
}

/* SCI device over temperature event handler */
static int ls3anb_over_temp_handler(int status)
{
	// do something
	return 0;
}

/* SCI device Throttling the CPU event handler */
static int ls3anb_throttling_CPU_handler(int status)
{
	// do something
	return 0;
}

/* SCI device AC event handler */
static int ls3anb_ac_handler(int status)
{
	/* Report status changed */
	power_supply_changed(&ls3anb_ac);

	return 0;
}

/* SCI device Battery event handler */
static int ls3anb_bat_handler(int status)
{
	/* Battery insert/pull-out to handle battery static information. */
	if(status & MASK(BIT_POWER_BATPRES))
	{
		/* If battery is insert, get battery static information. */
		ls3anb_power_info_battery_static_update();
	}
	else
	{
		/* Else if battery is pull-out, clear battery static information. */
		ls3anb_power_info_battery_static_clear();
	}
	/* Report status changed */
	power_supply_changed(&ls3anb_bat);

	return 0;
}

/* SCI device Battery low event handler */
static int ls3anb_bat_low_handler(int status)
{
	/* Report status changed */
	power_supply_changed(&ls3anb_bat);

	return 0;
}

/* SCI device Battery very low event handler */
static int ls3anb_bat_very_low_handler(int status)
{
	/* Report status changed */
	power_supply_changed(&ls3anb_bat);

	return 0;
}

/* SCI device LID event handler */
static int ls3anb_lid_handler(int status)
{
	if(status & BIT(BIT_DEVICE_LID))
	{
		return 1;
	}

	return 0;
}

/* Hotkey device init handler */
static int ls3anb_hotkey_init(void)
{
	int ret;

	ls3anb_hotkey_dev = input_allocate_device();
	if(!ls3anb_hotkey_dev)
	{
		return -ENOMEM;
	}

	ls3anb_hotkey_dev->name = "Loongson3A Laptop Hotkeys";
	ls3anb_hotkey_dev->phys = "button/input0";
	ls3anb_hotkey_dev->id.bustype = BUS_HOST;
	ls3anb_hotkey_dev->dev.parent = NULL;

	ret = sparse_keymap_setup(ls3anb_hotkey_dev, ls3anb_keymap, NULL);
	if(ret)
	{
		printk(KERN_ERR "LS3ANB Driver : Fail to setup input device keymap\n");
		input_free_device(ls3anb_hotkey_dev);

		return ret;
	}

	ret = input_register_device(ls3anb_hotkey_dev);
	if(ret)
	{
		sparse_keymap_free(ls3anb_hotkey_dev);
		input_free_device(ls3anb_hotkey_dev);

		return ret;
	}
	return 0;
}

/* Hotkey device exit handler */
static void ls3anb_hotkey_exit(void)
{
	if(ls3anb_hotkey_dev)
	{
		sparse_keymap_free(ls3anb_hotkey_dev);
		input_unregister_device(ls3anb_hotkey_dev);
		ls3anb_hotkey_dev = NULL;
	}
}


module_init(ls3anb_init);
module_exit(ls3anb_exit);

MODULE_AUTHOR("Huangw Wei <huangw@lemote.com>; Wang rui <wangr@lemote.com>");
MODULE_DESCRIPTION("Loongson3A Laptop Driver");
MODULE_LICENSE("GPL");

