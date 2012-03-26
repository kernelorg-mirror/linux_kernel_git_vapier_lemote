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

static void print_fixup_info(struct pci_dev * pdev)
{
	printk(KERN_INFO "Fixup: bus%d dev%xh fun%xh %x:%x irq %d\n",
			pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn),
		  	pdev->vendor, pdev->device, pdev->irq);

}

int __init pcibios_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	print_fixup_info(dev);
	return dev->irq;
}

/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}
