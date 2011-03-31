/*
 * EC (Embedded Controller) WPCE775L device driver header for Linux
 *
 * Copyright (C) 2011 Lemote Inc.
 * Author : Wang Rui <wangr@lemote.com>
 * Author : Huangddwei <huangw@lemote.com>
 * Date   : 2011-02-21
 *
 * EC relative header file. All the EC registers should be defined here.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at you option) and later version.
 */

#ifndef __EC_WPCE775L_H__
#define __EC_WPCE775L_H__

#define EC_VERSION		"1.03"

/* 
 * The following registers are determined by the EC index configureation.
 * 1. fill the PORT_INDEX as EC register.
 * 2. fill the PORT_DATA as EC register write data or get the data from it.
 */
/* base address for io access for Loongson3A+rs780e Notebook platform */
#define EC_BASE_ADDR_PORT	(0xb8000000)
#define SIO_INDEX_PORT		0x2E
#define SIO_DATA_PORT		0x2F

/* 
 * EC delay time 500us for register and status access
 * Unit : us
 */
#define EC_REG_DELAY		30000
#define EC_CMD_TIMEOUT		0x1000
#define EC_SEND_TIMEOUT		0x7ff
#define EC_RECV_TIMEOUT		0xffff

/*
 * EC access port for with Host communication.
 */
#define EC_CMD_PORT			0x66
#define EC_STS_PORT			0x66
#define EC_DAT_PORT			0x62

/* 
 * ACPI legacy commands.
 */
#define CMD_READ_EC			0x80	/* Read EC command. */
#define CMD_WRITE_EC		0x81	/* Write EC command. */
#define CMD_GET_EVENT_NUM	0x84	/* Query EC command, for get SCI event number. */

/*
 * ACPI OEM commands.
 */
#define REG_BACKLIGHT_CTRL	0x49	/* LCD backlight control: on/off */
enum
{
	BIT_BACKLIGHT_OFF = 0,
	BIT_BACKLIGHT_ON
};

#define CMD_RESET			0x4E	/* Reset and poweroff the machine auto-clear: rd/wr */
enum
{
	BIT_RESET_OFF = 0,
	BIT_RESET_ON,
	BIT_PWROFF_ON
};

#define CMD_EC_VERSION		0x4F	/* EC Version OEM command: 36 Bytes */

/*
 * Used ACPI legacy command 80h to do active.
 */
/* Read temperature & fan index for ACPI 80h command. */
#define INDEX_TEMPERATURE_VALUE		0x1B	/* Current CPU temperature value, Read and Write(81h command). */
#define INDEX_FAN_MAXSPEED_LEVEL	0x5B	/* Fan speed maxinum levels supported. Defaut is 6. */
#define INDEX_FAN_SPEED_LEVEL		0x5C	/* FAn speed level. [0,5])*/
#define INDEX_FAN_CTRLMOD			0x5D	/* Fan control mode, 0 = by EC, 1 = by Host.*/
#define BIT_FAN_CTRLMOD_HOST		0x01
#define BIT_FAN_CTRLMOD_EC			0x00
#define INDEX_FAN_STSCTRL			0x5E	/* Fan status/control, 0 = stop, 1 = run. */
enum
{
	BIT_FAN_STSCTRL_OFF = 0,
	BIT_FAN_STSCTRL_ON
};
#define INDEX_FAN_ERRSTS			0x5F	/* Fan error status, 0 = no error, 1 = has error. */
enum
{
	BIT_FAN_ERRSTS_NO = 0,
	BIT_FAN_ERRSTS_HAS
};
#define INDEX_FAN_SPEED_HIGH		0x09	/* Fan speed high byte. */
#define INDEX_FAN_SPEED_LOW			0x08	/* Fan speed low byte.*/

/* Read battery index for ACPI 80h command */
#define FLAG_BAT_CELL_2S1P			0x02
#define FLAG_BAT_CELL_2S20			0x04
#define FLAG_BAT_VENDOR_DYON		0x01	/* DeYon Electronics Co., Shenzhen */
#define FLAG_BAT_VENDOR_NONE		0x02

/*
 * The reported battery voltage measured on the BAT pin.
 * Voltage is expressed in mV with an LSB resolution of 1 mV.
 * Reported voltage cannot exceed 5000 mV. The host
 * system has read-only access to this register pair.
 * Voltage is updated every 2.56 seconds.
 */

#define INDEX_BATTERY_VOL_LOW		0x90	/* Battery Voltage Low byte. */
#define INDEX_BATTERY_VOL_HIGH		0x91	/* Battery Voltage High byte. */
#define INDEX_BATTERY_CAPACITY		0x92	/* Battery Capacity byte. */

/* 
 * The reported battery die temperature.
 * The temperature is expressed in units of 0.25 seconds and is updated every 2.56 seconds.
 * The equation to calculate reported pack temperature is:
 * Temperature = 0.25 * (256 * TEMPH + TEMPL)
 * The host sytem has read-only access to this register pair.
 */

#define INDEX_BATTERY_TEMP_LOW		0x93	/* Battery temperature low byte. */
#define INDEX_BATTERY_TEMP_HIGH		0x94	/* Battery temperature high byte. */
#define INDEX_BATTERY_FLAG			0x95	/* Battery flags byte. */
#define BIT_BATTERY_CURRENT_PN      7       /* Battery current sign is positive or negative */
#define BIT_BATTERY_CURRENT_PIN		0x07	/* Battery current sign is positive or negative.*/

/*
 * The Average Current value is reported with a resolution of 3.57 uV per count.
 * Use the following equation to convert the value to mA,
 * where Rs is the sense resistor value in milliohms, here Rs is 0.02 ohm:
 * Average Current = (256 * AIH + AIL) * 3.57 / Rs
 * The current reported is an average over the last 5.12 seconds.
 * The host system has read-only access to this register pair.
 */

#define INDEX_BATTERY_AI_LOW			0x96	/* Battery Current Low byte. */
#define INDEX_BATTERY_AI_HIGH			0x97	/* Battery Current High byte. */

#define	INDEX_DISPLAY_BRIGHTNESS		0x5A	/* 10 stages (0~A) LCD backlight brightness adjust */
enum
{
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_0	= 0,	/* This level is backlight turn off. */
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_1,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_2,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_3,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_4,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_5,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_6,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_7,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_8,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_9,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_10

};

#define MASK(x)	(1 << x)

#define INDEX_POWER_STATUS		0xA2	/* Read current power status. */
enum
{
	BIT_POWER_BATVL = 1,	/* Battery in very low status. */
	BIT_POWER_BATL,			/* Battery in low status. */
	BIT_POWER_BATFCHG,		/* Battery in fully charging status. */
	BIT_POWER_BATCHG,		/* Battery in charging status. */
	BIT_POWER_BATPRES = 6,		/* Battery present. */
	BIT_POWER_ACPRES		/* AC present. */
};

#define	INDEX_DEVICE_STATUS		0xA3	/* Read Current Device Status */
enum
{
	BIT_DEVICE_TP = 0,	/* TouchPad status: 0 = close, 1 = open */
	BIT_DEVICE_WLAN,	/* WLAN status: 0 = close, 1 = open */
	BIT_DEVICE_CAM,		/* Camera status: 0 = close, 1 = open */
	BIT_DEVICE_MUTE,	/* Mute status: 0 = close, 1 = open */
	BIT_DEVICE_LID,		/* LID status: 0 = close, 1 = open */
	BIT_DEVICE_BKLIGHT	/* BackLight status: 0 = close, 1 = open */
};

#define	INDEX_SHUTDOWN_ID		0xA4	/* Read Shutdown ID */
enum
{
	BIT_SHUTDNID_S45 = 0,	/* in S4 or S5 */
	BIT_SHUTDNID_BATDEAD,	/* Battery Dead */
	BIT_SHUTDNID_OVERHEAT,	/* Over Heat */
	BIT_SHUTDNID_SYSCMD,	/* System command */
	BIT_SHUTDNID_LPRESSPWN	/* Long press power button */
};

#define	INDEX_SYSTEM_CFG		0xA5		/* Read System config */
#define BIT_SYSCFG_TPSWITCH		(1 << 0)	/* TouchPad switch */
#define BIT_SYSCFG_WLANPRES		(1 << 1)	/* WLAN present */
#define BIT_SYSCFG_CAMERAPRES	(1 << 3)	/* Camera Present */
#define BIT_SYSCFG_VOLCTRLEC	(1 << 4)	/* Volume control by EC */
#define BIT_SYSCFG_AUTOBRIGHT	(1 << 7)	/* Auto brightness */

#define	INDEX_VOLUME_LEVEL		0xA6		/* Read Volume Level command */
#define	VOLUME_MAX_LEVEL		0x0A		/* Volume level max is 15 */
enum
{
	FLAG_VOLUME_LEVEL_0 = 0,
	FLAG_VOLUME_LEVEL_1,
	FLAG_VOLUME_LEVEL_2,
	FLAG_VOLUME_LEVEL_3,
	FLAG_VOLUME_LEVEL_4,
	FLAG_VOLUME_LEVEL_5,
	FLAG_VOLUME_LEVEL_6,
	FLAG_VOLUME_LEVEL_7,
	FLAG_VOLUME_LEVEL_8,
	FLAG_VOLUME_LEVEL_9,
	FLAG_VOLUME_LEVEL_10
};

/* EC_SC input */
#define EC_SMI_EVT		(1 << 6)	/* SMI event padding */
#define EC_SCI_EVT		(1 << 5)	/* SCI event padding */
#define EC_BURST		(1 << 4)	/* Controller is in burst mode */
#define EC_CMD			(1 << 3)	/* Byte in data register is command */

#define EC_IBF			(1 << 1)	/* Input buffer full (data ready for ec) */
#define EC_OBF			(1 << 0)	/* Output buffer full (data ready for host) */

/* SCI Event Number from EC */
enum
{
	/* Huangw modified for ls3anb, 2011-03-04 */
	SCI_EVENT_NUM_WLAN = 0x21,		/* Wlan is on or off, Fn+F1 */
	SCI_EVENT_NUM_3G,				/* Fn+F9 for 3G switch */
	SCI_EVENT_NUM_LID,				/* press the lid or not */
	SCI_EVENT_NUM_DISPLAY_TOGGLE,	/* Fn+F8 for display switch */
	SCI_EVENT_NUM_SLEEP,			/* Fn+ESC for entering sleep mode */
	SCI_EVENT_NUM_BRIGHTNESS_UP,	/* LCD backlight brightness adjust, Fn+F3 */
	SCI_EVENT_NUM_BRIGHTNESS_DN,	/* LCD backlight brightness adjust, Fn+F2 */
	SCI_EVENT_NUM_CAMERA,			/* Camera is on or off, Fn+F10, no use */
	SCI_EVENT_NUM_TP,				/* TP is on or off, Fn+F11, no use */
	SCI_EVENT_NUM_AUDIO_MUTE,		/* Mute is on or off, Fn+F4 */
	SCI_EVENT_NUM_BLACK_SCREEN,		/* Black screen is on or off, Fn+F7 */
	SCI_EVENT_NUM_VOLUME_UP,		/* Volume adjust, Fn+F6 */
	SCI_EVENT_NUM_VOLUME_DN,		/* Volume adjust, Fn+F5 */
	SCI_EVENT_NUM_OVERTEMP,			/* Over-temperature happened */
	SCI_EVENT_NUM_AC,				/* ac & battery relative issue */
	SCI_EVENT_NUM_BAT,				/* ac & battery relative issue */
	SCI_EVENT_NUM_BATL,				/* ac & battery relative issue */
	SCI_EVENT_NUM_BATVL				/* ac & battery relative issue, 0x32 */
};

#define SCI_EVENT_NUM_START		SCI_EVENT_NUM_WLAN
#define SCI_EVENT_NUM_END		SCI_EVENT_NUM_BATVL

extern unsigned char app_access_ec_flag;

typedef int (*sci_handler)(int status);

/* The general ec index-io port read action */
extern unsigned char ec_read(unsigned char index);
extern unsigned char ec_read_all(unsigned char command, unsigned char index);
extern unsigned char ec_read_noindex(unsigned char command);

/* The general ec index-io port write action */
extern int ec_write(unsigned char index, unsigned char data);
extern int ec_write_all(unsigned char command, unsigned char index, unsigned char data);
extern int ec_write_noindex(unsigned char command, unsigned char data);

/* Query sequence of 62/66 port access routine. */
extern int ec_query_seq(unsigned char command);
extern int ec_get_event_num(void);

extern void clean_ec_event_status(void);

#endif /* __EC_WPCE775L_H__ */
