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
#include <linux/init.h>
#include <linux/pci.h>
#include <asm/mips-boards/bonito64.h>

extern void prom_printf(char *fmt, ...);


#define NIC_INTERRUPT_LINE      0x06
#define OHCI_INTERRUPT_LINE     0x06
#define EHCI_INTERRUPT_LINE     0x06
#define SATA_INTERRUPT_LINE     0x04
#define INFIBAND_INTERRUPT_LINE 0x06
#define PCIE_NIC_INTERRUPT_LINE 0x06

#define NIC_PCI_IRQ             0x09  //INTE#
#define OHCI_PCI_IRQ            0x02  //INTB#
#define EHCI_PCI_IRQ            0x02  //INTC#
#define SATA_PCI_IRQ            0x0c  //INTH#
#define INFIBAND_PCI_IRQ        0x03  //INTD#
#define PCIE_NIC_PCI_IRQ        0x01  //INTB#

/**
 * ROUTE_IRQ_TO_LINE - route the irq to the line
 * @irq:
 * @line
 */
#define ROUTE_IRQ_TO_LINE(irq, line) \
do { \
	outb(irq, 0xc00); \
	outb(line, 0xc01); \
} while(0);


#define ROUTE_DEV_TO_IRQ(dev, irq) (void) pci_write_config_byte(dev, PCI_INTERRUPT_PIN, irq);

/**
 * ROUTE_DEV_TO_LINE - route the dev's interrupt to the line via the irq
 * @dev:
 * @irq:
 * @line
 */
#define ROUTE_DEV_TO_LINE(dev, irq1, line) \
do { \
	dev->irq = line; \
	(void) pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq); \
	ROUTE_IRQ_TO_LINE(irq1, line) \
} while(0);


#define SET_TRIGGER_MODE(level, line) outw(inw(0x4d0) | (level << line), 0x4d0); 

#define SET_DEV_INTERRUPT(dev, irq, line, level) \
do { \
	ROUTE_DEV_TO_LINE(dev, irq, line) \
	SET_TRIGGER_MODE(level, line) \
} while(0);

int __init pcibios_map_irq( struct pci_dev *dev, u8 slot, u8 pin)
{
        unsigned short t16;

		/* IRQ fixup for on-board peripherals */
		if ((dev->vendor == 0x10ec)  
			 && ((dev->device == 0x8169) || (dev->device == 0x8139))) { //8139 nic
			/* set interrupt vector */
			SET_DEV_INTERRUPT(dev, NIC_PCI_IRQ, NIC_INTERRUPT_LINE, 1)
   			printk("nic fix: rtl8139 interrupt routing 0x4d0=%x\n", inw(0x4d0));
			return dev->irq;
		}
#if 1
		else if ((dev->vendor == 0x1002)  
                         && ((dev->device == 0x4397)||(dev->device == 0x4398) 
			 || (dev->device == 0x4399))) { //ohci usb 
			/* set interrupt vector */
			SET_DEV_INTERRUPT(dev, OHCI_PCI_IRQ, OHCI_INTERRUPT_LINE, 1)
                        printk("usb fix: usb ohci interrupt routing 0x4d0=%x\n", inw(0x4d0));
                        return dev->irq;
                }
		else if ((dev->vendor == 0x1002)  
                         && (dev->device == 0x4396)) { //ehci usb 
                        /* set interrupt vector */
			SET_DEV_INTERRUPT(dev, EHCI_PCI_IRQ, EHCI_INTERRUPT_LINE, 1)
                        printk("usb fix: usb ehci interrupt routing 0x4d0=%x\n", inw(0x4d0));
                        return dev->irq;
                }
		else if ((dev->vendor == 0x1002)  
                         && (dev->device == 0x4390)) { //sata ahci
                        /* set interrupt vector */
			SET_DEV_INTERRUPT(dev, SATA_PCI_IRQ, SATA_INTERRUPT_LINE, 1)
			/* sata ahci use Pci_INTH# */
                        printk("sata fix: sata ahci interrupt routing, 0x4d0=%x\n", inw(0x4d0));
                        return dev->irq;
                }
                else if ((dev->vendor == 0x15b3)&& 
			((dev->device == 0x6340)||(dev->device == 0x634a)||(dev->device == 0x6278)||
			(dev->device == 0x6354)||(dev->device == 0x6732)||(dev->device == 0x673c))) 
		{ //infiband fixup
			/*REG 0x3d in NB_780e is RW and it's default value:0x00*/
                        /* set interrupt vector */
			SET_DEV_INTERRUPT(dev, INFIBAND_PCI_IRQ, INFIBAND_INTERRUPT_LINE, 1)
                        (void) pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
                        printk("fixup infiband hca  interrupt routing 0x4d0=%x\n", inw(0x4d0));
                        return dev->irq;

                }
#if 0
                else if ((dev->vendor == 0x14e4)
                         && (dev->device == 0x163a)) { //NB :NIC irq fixup
                        dev->irq = 7;
                        /* set interrupt vector */
                        (void) pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);

                        /* nic PciA# connect to  Pci_intB# */
			outb(0x01, 0xc00);
                	outb(0x07, 0xc01);
			outw(inw(0x4d0) | (1 << 7), 0x4d0);

                        printk("fixup NB NIC interrupt routing 0x4d0=%x\n",inw(0x4d0));
                        return dev->irq;
                }
#endif
		else if ((dev->vendor == 0x14e4)
                         && (dev->device == 0x1677)) { //broadcom tg3 ethernet
                        /* tg3 nic share interrupt hard line with usb ohcis  intA conneted to PCI_intB#*/
                        /* set interrupt vector */
			SET_DEV_INTERRUPT(dev, PCIE_NIC_PCI_IRQ, PCIE_NIC_INTERRUPT_LINE, 1)
                        printk("fixup PCIE SLOT NIC interrupt routing 0x4d0=%x\n",inw(0x4d0));
                        return dev->irq;

		}
		else if ((dev->vendor == 0x8086)
                         && (dev->device == 0x10d3)) { //Intel 82574 PCIE ethernet
			SET_DEV_INTERRUPT(dev, PCIE_NIC_PCI_IRQ, PCIE_NIC_INTERRUPT_LINE, 1)
                        printk("82574: fixup PCIE SLOT NIC interrupt routing 0x4d0=%x\n",inw(0x4d0));
                        return dev->irq;

		}
#endif
		else
		return 0;
}

/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}


/*
* smbus is the system control center in sb700
*/
static void __init godson3a_smbus_fixup(struct pci_dev *pdev)
{
        unsigned short t16;
	unsigned char t8;
	prom_printf("\n-----------------godson3a_smbus_fixup---------------\n");

        /*1. usb interrupt map smbus reg:0XBE  map usbint1map usbint3map(ohci use) to PCI_INTC#
	map usbint2map usbint4map(ehci use) to PCI_INTC# */
	(void) pci_write_config_word(pdev, 0xbe, ((2<<0)|(2 << 3)|(2 << 8)|(2 << 11)) );
	pci_read_config_word(pdev, 0xbe, &t16);
	prom_printf(" set smbus reg (0xbe) :%x (usb intr map)\n", t16);

	/*2. sata interrupt map smbus reg:0Xaf map sataintmap to PCI_INTH#*/
	//pci_read_config_byte(pdev, 0xaf, &t8);
	//(void) pci_write_config_byte(pdev, 0xaf, ( (t8 & ~0x1c) | (0x0<<2)) );
	(void) pci_write_config_byte(pdev, 0xaf,  0x1c );
	pci_read_config_byte(pdev, 0xaf, &t8);
	prom_printf(" set smbus reg (0xaf) :%x (sata intr map)\n", t8);


}
DECLARE_PCI_FIXUP_EARLY(0x1002, 0x4385, godson3a_smbus_fixup);

static void __init godson3a_ide_fixup(struct pci_dev *pdev)
{
    prom_printf("\n-----------------godson3a_ide_fixup---------------\n");
	/*set IDE ultra DMA enable as master and slalve device*/
    (void) pci_write_config_byte(pdev, 0x54, 0xf);
   /*set ultral DAM mode 0~6  we use 6 as high speed !*/
  (void) pci_write_config_word(pdev, 0x56, (0x6 << 0)|(0x6 << 4)|(0x6 << 8)|(0x6 << 12));
}

DECLARE_PCI_FIXUP_FINAL(0x1002, 0x439C, godson3a_ide_fixup);


/* fixup sb700 sata controller configure.
*  in file "drivers/pci/quirks.c" function quirk_amd_ide_mode do the same job
*/
static void __init godson3a_sata_fixup(struct pci_dev *pdev)
{
     unsigned char t8;
     unsigned int t32;
    prom_printf("\n-----------------godson3a_sata_fixup---------------\n");

   /*1. enable the subcalss code register for setting sata controller mode*/
    pci_read_config_byte(pdev, 0x40, &t8);
    (void) pci_write_config_byte(pdev, 0x40, (t8 | 0x01) );

   /*2.  set sata controller act as AHCI mode 
   *	 sata controller support IDE mode, AHCI mode, Raid mode*/
   (void) pci_write_config_byte(pdev, 0x09, 0x01);
   (void) pci_write_config_byte(pdev, 0x0a, 0x06);

   /*3. disable the subcalss code register*/
    pci_read_config_byte(pdev, 0x40, &t8);
    (void) pci_write_config_byte(pdev, 0x40, t8 & (~0x01));
	
   prom_printf("-----------------tset sata------------------\n");
	pci_read_config_dword(pdev, 0x40, &t32);
	prom_printf("sata pci_config 0x40 (%x)\n", t32);

}

DECLARE_PCI_FIXUP_FINAL(0x1002, 0x4390, godson3a_sata_fixup);




