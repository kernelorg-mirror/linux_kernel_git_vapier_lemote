#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <asm/bootinfo.h>

#define TEMP_SENSOR_ADDR	0x4c
//#define TEMP_DEBUG

#ifdef CONFIG_64BIT
#define TEMPRATURE_SENSOR_REG 0xffffffffbfe0019c
#else
#define TEMPRATURE_SENSOR_REG 0xbfe0019c
#endif

struct i2c_client *client = NULL;
static struct workqueue_struct *notify_workqueue;
static void notify_temp(struct work_struct *work);
static DECLARE_DELAYED_WORK(notify_work, notify_temp);

extern int ec_write_noindex(u8, u8);
extern u8 pm_ioread(u8 reg);
extern void pm_iowrite(u8 reg, u8 val);
extern u8 pm2_ioread(u8 reg);
extern void pm2_iowrite(u8 reg, u8 val);

static int fan_controlled; // fan speed is not controlled by default.

static void enable_fan_control(void)
{
        unsigned char temp8;
	
	//fan 0
	temp8 = pm_ioread(0x60);
	pm_iowrite(0x60, temp8 | 0x40); // configure gpio3 as fan0out

	temp8 = pm2_ioread(0x0);
	temp8 &= ~3;
	temp8 |= 0x1; // enable software control
	pm2_iowrite(0x0, temp8);

	temp8 = pm2_ioread(0x1);
	temp8 &= ~3; // disable automode
	temp8 |= 0x4; // active high
	pm2_iowrite(0x1, temp8);

	pm2_iowrite(0x2, 4); // set freq to 19.82KHz

	//fan 1
	temp8 = pm_ioread(0x60);
	pm_iowrite(0x60, temp8 | 0x4); // configure gpio48 as fan1out

	temp8 = pm2_ioread(0x0);
	temp8 &= ~0xc;
	temp8 |= 0x4; // enable software control
	pm2_iowrite(0x0, temp8);

	temp8 = pm2_ioread(0xe);
	temp8 &= ~3; // disable automode
	temp8 |= 0x4; // active high
	pm2_iowrite(0xe, temp8);

	pm2_iowrite(0xf, 4); // set freq to 19.82KHz

	fan_controlled = 1;
}

void adjust_fan0_speed(unsigned char fan_level)
{
	pm2_iowrite(0x3, fan_level);
}

void adjust_fan1_speed(unsigned char fan_level)
{
	pm2_iowrite(0x10, fan_level);
}

void fan_adjust(u16 cputemp, u8 nbtemp)
{
	if (cputemp > 128) {
		printk(KERN_ERR "CPU IS HOT!!! %d\n", cputemp);
	}

	if (!fan_controlled)
		enable_fan_control();	

	adjust_fan0_speed(0x80);
	adjust_fan1_speed(0x80);
}

static void notify_temp(struct work_struct *work)
{
	u8 boardtemp, nbtemp;
	u16 cputemp;

	boardtemp = i2c_smbus_read_byte_data(client, 0);
	nbtemp = i2c_smbus_read_byte_data(client, 1);
	cputemp = (*(u16 *)(TEMPRATURE_SENSOR_REG) & 0xff00) >> 8;

#ifdef TEMP_DEBUG
	printk(KERN_ERR "notify_temp:\n	boardtemp %d nbtemp %d cputemp %d\n", 
						boardtemp, nbtemp, cputemp);
#endif

	switch(mips_machtype) {
	case MACH_LEMOTE_3A_A1004:
        	ec_write_noindex(0x4d, boardtemp);
		break;
	case MACH_LEMOTE_3A_A1101:
	case MACH_LEMOTE_2GQ_A1205:
		fan_adjust(nbtemp, cputemp);
		return; // now keep fan speed constant
	default:
		break;
	}

        queue_delayed_work(notify_workqueue, &notify_work, HZ);
}

static __init int notify_temp_init(void)
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_board_info info;
	int i = 0, found = 0;

        memset(&info, 0, sizeof(struct i2c_board_info));
        adapter = i2c_get_adapter(i++);

        while(adapter) {
                if (strncmp(adapter->name, "SMBus PIIX4", 11) == 0) {
                        found = 1;
                        break;
                }

                adapter = i2c_get_adapter(i++);
        }

        if (!found)
		goto fail;

#ifdef TEMP_DEBUG
	printk(KERN_INFO "match adapter %s\n", adapter->name);
#endif
	info.addr = TEMP_SENSOR_ADDR;
	info.platform_data = "temp sensor";

	client = i2c_new_device(adapter, &info);
        if (client == NULL) {
                printk(KERN_ERR "notify_temp: failed to attach EM1412 sensor\n");
                goto fail;
        }
#ifdef TEMP_DEBUG
        printk(KERN_ERR "notify_temp: success to attach EM1412 sensor\n");
#endif
        notify_workqueue = create_singlethread_workqueue("Temprature Notify");
        queue_delayed_work(notify_workqueue, &notify_work, HZ);

fail:
	return 0;
}

static __exit void notify_temp_cleanup(void)
{
	if (client)
		i2c_unregister_device(client);

        cancel_delayed_work(&notify_work);
        destroy_workqueue(notify_workqueue);

}
module_init(notify_temp_init);
module_exit(notify_temp_cleanup);

MODULE_LICENSE("GPL");
