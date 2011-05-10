#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <asm/bootinfo.h>

#define TEMP_SENSOR_ADDR	0x4c
//#define TEMP_DEBUG

struct i2c_client *client = NULL;

static struct workqueue_struct *notify_workqueue;
static void notify_temp(struct work_struct *work);
static DECLARE_DELAYED_WORK(notify_work, notify_temp);
extern int ec_write_noindex(u8, u8);

static void notify_temp(struct work_struct *work)
{
	u8 boardtemp;

	boardtemp = i2c_smbus_read_byte_data(client, 0);

#ifdef TEMP_DEBUG
	printk(KERN_ERR "notify_temp: get temp %d\n", boardtemp);
#endif
        ec_write_noindex(0x4d, boardtemp);

        queue_delayed_work(notify_workqueue, &notify_work, HZ);
}

static __init int notify_temp_init(void)
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_board_info info;
	int i = 0, found = 0;

	if (mips_machtype != MACH_LEMOTE_3A_A1004)
		return 0;

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
	printk(KERN_INFO "match adater %s\n", adapter->name);
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
