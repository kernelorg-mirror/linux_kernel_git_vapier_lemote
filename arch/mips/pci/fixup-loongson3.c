/*
 * fixup-3a780e.c
 *
 * Copyright (C) 2004 ICT CAS
 * Author: Li xiaoyu, ICT CAS
 *   lixy@ict.ac.cn
 *
 * Copyright (C) 2007 Lemote, Inc. & Institute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 * 
 * Copyright (C) 2010 Dawning 
 * Author: Yongcheng Li, Dawning 
 *    liych@dawning.com.cn
 *
 * Copyright (C) 2010 Dawning 
 * Author: Lv minqiang, Dawning, Inc 
 *    lvmq@dawning.com.cn
 *  Changed for : 
 *		1.addust coding 
 *		2. add sata fixup
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

#include <linux/pci.h>

/*
 * SB700 PIC has 10 interrupt input pins, not increasing by number,
 * no 6,7,8 exist.
 */
enum pic_int {
	PIC_INTA  = 0,
	PIC_INTB  = 1,
	PIC_INTC  = 2,
	PIC_INTD  = 3,
	PIC_SCI   = 4,
	PIC_SMBUS = 5,
	PIC_INTE  = 9,
	PIC_INTF  = 10,
	PIC_INTG  = 11,
	PIC_INTH  = 12
};

/*
 * SB700 PIC has 16 interrupt output pins
 */
enum pic_irq {
	PIC_IRQ0 = 0,
	PIC_IRQ1,
	PIC_IRQ2,
	PIC_IRQ3,
	PIC_IRQ4,
	PIC_IRQ5,
	PIC_IRQ6,
	PIC_IRQ7,
	PIC_IRQ8,
	PIC_IRQ9,
	PIC_IRQ10,
	PIC_IRQ11,
	PIC_IRQ12,
	PIC_IRQ13,
	PIC_IRQ14,
	PIC_IRQ15
};

struct pic_routing {
	enum pic_int from;
	enum pic_irq to;
};

struct pic_routing pic_routing_array[] = {
	{PIC_INTA, PIC_IRQ3},	/* 0 */ 
	{PIC_INTB, PIC_IRQ3},	/* 1 */ 
	{PIC_INTC, PIC_IRQ6},	/* 2 */ 
	{PIC_INTD, PIC_IRQ5},	/* 3 */ 
	{PIC_SCI,  PIC_IRQ7},	/* 4 */ 
	{-1,	0},		/* 5 SMBUS not used*/ 
	{-1,	0},		/* 6 */ 
	{-1,	0},		/* 7 */ 
	{-1,	0},		/* 8 */ 
	{PIC_INTE, PIC_IRQ5},	/* 9 */ 
	{PIC_INTF, PIC_IRQ5},	/* 10 */
	{PIC_INTG, PIC_IRQ5},	/* 11 */
	{PIC_INTH, PIC_IRQ4}	/* 12 */
};

enum dev_list {
	/* RS780 devices list*/
	INTERNAL_GFX = 0,
	GPPSB0,
	GPPSB1,
	GPPSB2,
	X16_SLOT,
	/* SB7xx devices list*/
	HDA,
	IDE,
	SATA,
	USB1_OHCI,	// device 12h
	USB1_EHCI,	// device 12h
	USB2_OHCI,	// device 13h
	USB2_EHCI,	// device 13h
	USB3_OHCI,	// device 14h
	DEV_LIST_END
};

enum reg_int {
	REG_INTA = 0,
	REG_INTB = 1,
	REG_INTC = 2,
	REG_INTD = 3,
	REG_INTE = 4,
	REG_INTF = 5,
	REG_INTG = 6,
	REG_INTH = 7
};

struct dev_interrupt_descriptor {
	enum pic_int pic_int_number;
	enum reg_int reg_int_number;
};

struct dev_interrupt_descriptor dev_interrupt_array[DEV_LIST_END] = {

	/* INTERNAL_GFX */	{ PIC_INTC, REG_INTC}, // fixed
	/* GPPSB0    */		{ PIC_INTA, REG_INTA}, // fixed
	/* GPPSB1    */		{ PIC_INTB, REG_INTB}, // fixed
	/* GPPSB2    */		{ PIC_INTC, REG_INTC}, // fixed
	/* X16_SLOT  */		{ PIC_INTC, REG_INTC}, // fixed
	/* HDA       */		{ PIC_INTE, REG_INTE},
	/* IDE       */		{ PIC_INTA, REG_INTA}, // fixed
	/* SATA      */		{ PIC_INTH, REG_INTH},
	/* USB1_OHCI */		{ PIC_INTC, REG_INTC},
	/* USB1_EHCI */		{ PIC_INTC, REG_INTC},
	/* USB2_OHCI */		{ PIC_INTC, REG_INTC},
	/* USB2_EHCI */		{ PIC_INTC, REG_INTC},
	/* USB3_OHCI */		{ PIC_INTC, REG_INTC}
};

static int get_device_irq(enum dev_list dev)
{
	int i = 0, irq = 0;

	while (i < ARRAY_SIZE(pic_routing_array)) {
		if (pic_routing_array[i].from == dev_interrupt_array[dev].pic_int_number) {
			irq = pic_routing_array[i].to;
			break;
		}

		i++;
	}

	return irq;
}

static void print_fixup_info(struct pci_dev * pdev)
{
	printk(KERN_INFO "Fixup: bus%d dev_%xh %x:%x irq=%d\n",
			pdev->bus->number, PCI_SLOT(pdev->devfn),
		  	pdev->vendor, pdev->device, pdev->irq);
}

int __init pcibios_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	switch (dev->bus->number) {
	case 0:	
	case 1:			// Integrated devices in RS780 & SB7xx
		return 0;
	case 2:
		dev->irq = get_device_irq(GPPSB0);
		pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
		break;
	case 4:
		dev->irq = get_device_irq(GPPSB2);
		pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
		break;
	default: // unexcepted device
		dev->irq = 0; 
		printk(KERN_ERR "++++++++++++ UnExcepted Deivce! ++++++++++\n");
	}

	print_fixup_info(dev);
	return dev->irq;
}

/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}

/* 
 * SB7xx PIC can map any INTx to IRQn by programming as following:
 *
 * INTx --> 0xC00
 * IRQn --> 0xC01
 */
static inline void int2irq_map(struct pic_routing routing)
{
	outb(routing.from, 0xC00);
	outb(routing.to, 0xC01);
}

static void pic_routing_config(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pic_routing_array); i++) {
		if (pic_routing_array[i].from != -1)
			int2irq_map(pic_routing_array[i]);
	}
}

static void setting_device_irq(struct pci_dev *pdev, enum dev_list dev)
{
	pdev->irq = get_device_irq(dev);
	pci_write_config_byte(pdev, PCI_INTERRUPT_LINE, pdev->irq);

	print_fixup_info(pdev);
}

static void __devinit hda_initialize(struct pci_dev *pdev)
{
	u8 reg8;

	/* Map the HDA interrupt to INTE */
	pci_read_config_byte(pdev, 0x63, &reg8);
	reg8 &= 0xf8;
	reg8 |= dev_interrupt_array[HDA].reg_int_number;
	pci_write_config_byte(pdev, 0x63, reg8);

	/* Set GPIO42, GPIO43, GPIO44, GPIO46 as HD function */
	pci_write_config_word(pdev, 0xf8, 0x0);
	pci_write_config_word(pdev, 0xfc, 0x2<<0);
}

static void __devinit sata_initialize(struct pci_dev *pdev)
{
	u8 reg8;

	/* sata interrupt map smbus reg:0Xaf map sataintmap to PCI_INTH#*/
	pci_read_config_byte(pdev, 0xaf, &reg8);
	reg8 &= ~(0x7 << 2);
	reg8 |= (dev_interrupt_array[SATA].reg_int_number << 2);
	pci_write_config_byte(pdev, 0xaf, reg8);

	/* Set SATA and PATA Controller to combined mode
	 * Port0-Port3 is SATA mode, Port4-Port5 is IDE mode
	 */
	pci_read_config_byte(pdev, 0xad, &reg8);
	reg8 |= 0x1<<3;
	reg8 &= ~(0x1<<4);
	pci_write_config_byte(pdev, 0xad, reg8);
}

static void __devinit usb_initialize(struct pci_dev *pdev)
{
	u8 reg8;
	u16 reg16;

	// set USB1 & USB2 interrupt
	reg16 = (dev_interrupt_array[USB1_OHCI].reg_int_number << 0) |
		(dev_interrupt_array[USB1_EHCI].reg_int_number << 3) |
		(dev_interrupt_array[USB2_OHCI].reg_int_number << 8) |
		(dev_interrupt_array[USB2_EHCI].reg_int_number << 11);
	pci_write_config_word(pdev, 0xbe, reg16);
	
	// set USB3 interrupt
	pci_read_config_byte(pdev, 0x63, &reg8);
	reg8 &= ~(0x7 << 4);
	reg8 |= (dev_interrupt_array[USB3_OHCI].reg_int_number << 4);
	pci_write_config_byte(pdev, 0x63, reg8);
}

/*
 * smbus is the system control center in sb700
 */
static void __devinit godson3a_smbus_fixup(struct pci_dev *pdev)
{
	usb_initialize(pdev);

	sata_initialize(pdev);

	hda_initialize(pdev);

	pic_routing_config();
	
	// level or edge interrupt
	outw(inw(0x4D0)|(1<<7)|(1<<6)|(1<<5)|(1<<4)|(1<<3), 0x4D0);
}

/* fixup sb700 sata controller configure.
 *  in file "drivers/pci/quirks.c" function quirk_amd_ide_mode do the same job
 */
static void __devinit godson3a_sata_fixup(struct pci_dev *pdev)
{
	unsigned char t8;

	/*1. enable the subcalss code register for setting sata controller mode*/
	pci_read_config_byte(pdev, 0x40, &t8);
	pci_write_config_byte(pdev, 0x40, (t8 | 0x01));

	/*2. set sata controller act as AHCI mode
	 *	 sata controller support IDE mode, AHCI mode, Raid mode*/
	pci_write_config_byte(pdev, 0x09, 0x01);
	pci_write_config_byte(pdev, 0x0a, 0x06);

	/*3. disable the subcalss code register*/
	pci_read_config_byte(pdev, 0x40, &t8);
	pci_write_config_byte(pdev, 0x40, t8 & (~0x01));

	setting_device_irq(pdev, SATA);
}

static void __devinit godson3a_ide_fixup(struct pci_dev *pdev)
{
        /*set IDE ultra DMA enable as master and slalve device*/
	pci_write_config_byte(pdev, 0x54, 0xf);

	/*set ultral DAM mode 0~6  we use 6 as high speed !*/
	pci_write_config_word(pdev, 0x56, (0x6 << 0)|(0x6 << 4)|(0x6 << 8)|(0x6 << 12));
}

static void __devinit godson3a_ohci1_fixup(struct pci_dev *pdev)
{
	if (PCI_SLOT(pdev->devfn) == 0x12)
		setting_device_irq(pdev, USB1_OHCI);
	else
		setting_device_irq(pdev, USB2_OHCI);
}

static void __devinit godson3a_ohci2_fixup(struct pci_dev *pdev)
{
	setting_device_irq(pdev, USB3_OHCI);
}

static void __devinit godson3a_ehci_fixup(struct pci_dev *pdev)
{
	if (PCI_SLOT(pdev->devfn) == 0x12)
		setting_device_irq(pdev, USB1_EHCI);
	else
		setting_device_irq(pdev, USB2_EHCI);
}

static void __devinit godson3a_lpc_fixup(struct pci_dev *pdev)
{
	unsigned char t;

	pci_read_config_byte(pdev, 0x46, &t);
	printk("Fixup: lpc: 0x46 value is 0x%x\n",t);
	pci_write_config_byte(pdev, 0x46, t|(0x3 << 6));
	pci_read_config_byte(pdev, 0x46, &t);

	pci_read_config_byte(pdev, 0x47, &t);
	printk("Fixup: lpc: 0x47 value is 0x%x\n",t);
	pci_write_config_byte(pdev, 0x47, t|0xff);
	pci_read_config_byte(pdev, 0x47, &t);

	pci_read_config_byte(pdev, 0x48, &t);
	printk("Fixup: lpc: 0x48 value is 0x%x\n",t);
	pci_write_config_byte(pdev, 0x48, t|0xff);
	pci_read_config_byte(pdev, 0x48, &t);
}

static void __devinit godson3a_hda_fixup(struct pci_dev *pdev)
{
	setting_device_irq(pdev, HDA);
}

static void __devinit godson3a_graphic_fixup(struct pci_dev *pdev)
{
	setting_device_irq(pdev, INTERNAL_GFX);
}

DECLARE_PCI_FIXUP_EARLY(0x1002, 0x4385, godson3a_smbus_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x4390, godson3a_sata_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x439c, godson3a_ide_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x4397, godson3a_ohci1_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x4398, godson3a_ohci1_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x4399, godson3a_ohci2_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x4396, godson3a_ehci_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x439d, godson3a_lpc_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x4383, godson3a_hda_fixup);
DECLARE_PCI_FIXUP_FINAL(0x1002, 0x9615, godson3a_graphic_fixup);

DECLARE_PCI_FIXUP_RESUME_EARLY(0x1002, 0x4385, godson3a_smbus_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x4390, godson3a_sata_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x439c, godson3a_ide_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x4397, godson3a_ohci1_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x4398, godson3a_ohci1_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x4399, godson3a_ohci2_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x4396, godson3a_ehci_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x439d, godson3a_lpc_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x4383, godson3a_hda_fixup);
DECLARE_PCI_FIXUP_RESUME(0x1002, 0x9615, godson3a_graphic_fixup);
