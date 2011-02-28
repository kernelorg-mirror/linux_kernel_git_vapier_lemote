/* 
 * 775 EC communication
 * added by huangw@lemote.com
 * */

#include <asm/types.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <wpce775x.h>

static int wpce775_send_command(u8 cmd);
static int wpce775_send_data(u8 data);

static int wpce775_command(u8 cmd, u8 data)
{
	int ret = 0;

	ret = wpce775_send_command(cmd);
	if (ret < 0)
		return -1;

	ret = wpce775_send_data(data);

	return ret;
}

static int wpce775_send_command(u8 cmd)
{
        int timeout = EC_SEND_TIMEOUT;

        while ((inb(EC_STS_PORT) & EC_IBF) && --timeout) {
                udelay(10);
        }

        if (!timeout) {
                printk(KERN_ERR "WPCE775: Timeout while sending command 0x%02x to EC!\n", cmd);
		return -1;
        }

        outb(cmd, EC_CMD_PORT);

        return 0;
}

static int wpce775_send_data(u8 data)
{
        int timeout = EC_SEND_TIMEOUT;

        while ((inb(EC_STS_PORT) & EC_IBF) && --timeout) {
                udelay(10);
        }

        if (!timeout) {
                printk(KERN_ERR "WPCE775: Timeout while sending data 0x%02x to EC!\n", data);
		return -1;
        }

        outb(data, EC_DAT_PORT);

        return 0;

}
void wpce775_reboot(void)
{
	u8 cmd = CMD_RESET;
	u8 data = BIT_RESET_ON;

	wpce775_command(cmd, data);
}

void wpce775_poweroff(void)
{
	u8 cmd = CMD_RESET;
	u8 data = BIT_PWROFF_ON;

	wpce775_command(cmd, data);
}
