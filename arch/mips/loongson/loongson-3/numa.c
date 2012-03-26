#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/module.h>
#include <linux/nodemask.h>
#include <linux/swap.h>
#include <linux/bootmem.h>
#include <linux/pfn.h>
#include <linux/highmem.h>
#include <asm/page.h>
#include <asm/pgalloc.h>
#include <asm/sections.h>

#include <linux/bootmem.h>
#include <linux/init.h>
#include <linux/irq.h>

#include <asm/bootinfo.h>
#include <asm/mc146818-time.h>
#include <asm/time.h>
#include <asm/wbflush.h>

#define DIABLE_NODE1
#undef DIABLE_NODE1

extern void prom_printf(char *fmt, ...);

static struct node_data prealloc__node_data[MAX_NUMNODES] ;
struct node_data *__node_data[MAX_NUMNODES];
EXPORT_SYMBOL(__node_data);

unsigned char __node_distances[MAX_NUMNODES][MAX_NUMNODES];

#ifdef CONFIG_SMP
static unsigned int cpu_num = CONFIG_NR_CPUS;
#else
static unsigned int cpu_num = 1;
#endif

static unsigned long per_node_addrspace_size;

extern unsigned long memsize, highmemsize;

static void enable_lpa(void)
{
	unsigned long value;

	value = __read_32bit_c0_register($16, 3);
	value |= 0x00000080;
	__write_32bit_c0_register($16, 3, value);
	value = __read_32bit_c0_register($16, 3);
	prom_printf("CP0_Config3: CP0 16.3 (0x%lx)\n", value);

	value = __read_32bit_c0_register($5, 1);
	value |= 0x20000000;
	__write_32bit_c0_register($5, 1, value);
	value = __read_32bit_c0_register($5, 1);
	prom_printf("CP0_PageGrain: CP0 5.1 (0x%lx)\n", value);
}

static void cpu_node_probe(void)
{
	int i;

	nodes_clear(node_online_map);
	for (i = 0; i < MAX_NUMNODES; i++) {
		node_set_online(num_online_nodes());
		//highest += 4; //4 cores per node
	}

	printk("NUMA: Discovered %d cpus on %d nodes\n", cpu_num, num_online_nodes());
	prom_printf("NUMA: Discovered %d cpus on %d nodes\n", cpu_num, num_online_nodes());
}

static void __init szmem(void)
{
	unsigned long node_psize;	/* Hack to detect problem configs */
	unsigned long start_pfn, end_pfn;
	unsigned int node;

	num_physpages = 0;

	per_node_addrspace_size = (highmemsize + 256UL) * 2;
	prom_printf("NUMA: per_node_addrspace_size = %d MB\n", per_node_addrspace_size);
	for_each_online_node(node) {
		//node_psize = (PER_NODE_ADDRSPACE_SIZE << 20) >> PAGE_SHIFT;
		node_psize = (per_node_addrspace_size << 20) >> PAGE_SHIFT;
		if(node == 0) 
			node_psize -= ((8 << 20) >> PAGE_SHIFT); //cww
#ifdef DIABLE_NODE1
		if(node == 1) node_psize = 0; //cww
#endif
		num_physpages += node_psize;
		switch(node)
		{
			case 0:
				start_pfn = 0x0;
				break;
			case 1:
				start_pfn = (0x100000000000UL >> PAGE_SHIFT);
				break;
			case 2:
				start_pfn = (0x60000000 >> PAGE_SHIFT);
				break;
			case 3:
				start_pfn = (0x70000000 >> PAGE_SHIFT);
				break;
			default:
				break;
		}
		end_pfn  = start_pfn + node_psize;
#ifdef DIABLE_NODE1
		if(node==1) end_pfn = start_pfn; //cww
#endif
		prom_printf("NUMA: Node%d range(0x%lx-0x%lx) size(0x%lx) total(0x%lx)\n", 
			node, start_pfn << PAGE_SHIFT, end_pfn << PAGE_SHIFT, 
			node_psize << PAGE_SHIFT, num_physpages << PAGE_SHIFT);
		add_active_range(node, start_pfn, end_pfn);
	}
}

static void __init node_mem_init(unsigned int node)
{
	unsigned long freepfn; //= firstpfn;
	unsigned long bootmap_size;
	unsigned long start_pfn, end_pfn;
	
	switch(node)
	{
		case 0:
			start_pfn = 0x0;
			break;
		case 1:
			start_pfn = (0x100000000000UL >> PAGE_SHIFT);
			break;
		case 2:
			start_pfn = (0x60000000 >> PAGE_SHIFT);
			break;
		case 3:
			start_pfn = (0x70000000 >> PAGE_SHIFT);
			break;
		default:
			break;
	}

        freepfn = start_pfn;
	if(node == 0)
		freepfn = PFN_UP(__pa_symbol(&_end)); //((16<<20)>>PAGE_SHIFT);
	get_pfn_range_for_nid(node, &start_pfn, &end_pfn);

	__node_data[node] = prealloc__node_data + node;

	NODE_DATA(node)->bdata = &bootmem_node_data[node];
	NODE_DATA(node)->node_start_pfn = start_pfn;
	NODE_DATA(node)->node_spanned_pages = end_pfn - start_pfn;

	// ???? TBD cpus_clear(hub_data(node)->h_cpus);

	//freepfn += PFN_UP(sizeof(struct pglist_data) + sizeof(struct hub_data));

  	bootmap_size = init_bootmem_node(NODE_DATA(node), freepfn,
					start_pfn, end_pfn);
	free_bootmem_with_active_regions(node, end_pfn);
	reserve_bootmem_node(NODE_DATA(node), start_pfn << PAGE_SHIFT,
		((freepfn - start_pfn) << PAGE_SHIFT) + bootmap_size,
		BOOTMEM_DEFAULT);
#ifdef DIABLE_NODE1
	if(node == 0) //cww
#endif
	reserve_bootmem_node(NODE_DATA(node), \
		((unsigned long)node << 44) | 0x10000000, \
		per_node_addrspace_size << (20 - 1) , BOOTMEM_DEFAULT); //add by cww
	if((node == 0) && (memsize == 128)) /* Reserve for UMA */
		reserve_bootmem_node(NODE_DATA(node), 0x08000000, 0x08000000, BOOTMEM_DEFAULT); 
	sparse_memory_present_with_active_regions(node);
}

static __init void prom_meminit(void)
{
	unsigned int node, cpu;
        
	cpu_node_probe();
	szmem();

	for (node = 0; node < MAX_NUMNODES; node++) {
		if (node_online(node)) {
			node_mem_init(node);
			cpus_clear(__node_data[(node)]->cpumask);///gx GX TODO
			continue;
		}
	}

	for (cpu = 0; cpu < cpu_num; cpu++) {
		node = cpu / 4;
		if(node >= num_online_nodes())
			node = 0; //cww ??
			//continue;
		prom_printf("NUMA: set cpumask cpu %d on node %d\n", cpu, node);
		cpu_set(cpu, __node_data[(node)]->cpumask);///gx GX TODO
	}
}

void __init paging_init(void)
{
	unsigned long zones_size[MAX_NR_ZONES] = {0, };
	unsigned node;

	pagetable_init();

	for_each_online_node(node) {
		unsigned long  start_pfn, end_pfn;

		get_pfn_range_for_nid(node, &start_pfn, &end_pfn);

		if (end_pfn > max_low_pfn)
			max_low_pfn = end_pfn;
	}
#ifdef CONFIG_ZONE_DMA32
	zones_size[ZONE_DMA32] = MAX_DMA32_PFN;
#endif
	zones_size[ZONE_NORMAL] = max_low_pfn;
	free_area_init_nodes(zones_size);
}

extern unsigned long setup_zero_pages(void);

void __init mem_init(void)
{
	unsigned long codesize, datasize, initsize, tmp;
	unsigned node;

	high_memory = (void *) __va(num_physpages << PAGE_SHIFT);

	for_each_online_node(node) {
		/*
		 * This will free up the bootmem, ie, slot 0 memory.
		 */
		totalram_pages += free_all_bootmem_node(NODE_DATA(node));
	}

	totalram_pages -= setup_zero_pages();	/* This comes from node 0 */

	codesize =  (unsigned long) &_etext - (unsigned long) &_text;
	datasize =  (unsigned long) &_edata - (unsigned long) &_etext;
	initsize =  (unsigned long) &__init_end - (unsigned long) &__init_begin;

	tmp = nr_free_pages();
	printk(KERN_INFO "Memory: %luk/%luk available (%ldk kernel code, "
	       "%ldk reserved, %ldk data, %ldk init, %ldk highmem)\n",
	       tmp << (PAGE_SHIFT-10),
	       num_physpages << (PAGE_SHIFT-10),
	       codesize >> 10,
	       (num_physpages - tmp) << (PAGE_SHIFT-10),
	       datasize >> 10,
	       initsize >> 10,
	       (unsigned long) (totalhigh_pages << (PAGE_SHIFT-10)));
}


int pcibus_to_node(struct pci_bus *bus)
{
        return 0;
}
EXPORT_SYMBOL(pcibus_to_node);

void __init prom_init_numa_memory(void)
{
	enable_lpa();
	prom_meminit();
}
EXPORT_SYMBOL(prom_init_numa_memory);
 
