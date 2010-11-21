#ifndef _ASM_MACH_MMZONE_H
#define _ASM_MACH_MMZONE_H

#define NODE_ADDRSPACE_SHIFT 44
//#define NODE_ADDRSPACE_SHIFT 28

#define g3node_getbasepfn(node)   ((node)<<(NODE_ADDRSPACE_SHIFT - PAGE_SHIFT))
#define g3node_getfirstfree(node) ((node)<<(NODE_ADDRSPACE_SHIFT - PAGE_SHIFT))

#define PER_NODE_ADDRSPACE_SIZE    4096UL

//#define pa_to_nid(addr)		((addr)>>NODE_ADDRSPACE_SHIFT)
#define pa_to_nid(addr)		(((addr)>>NODE_ADDRSPACE_SHIFT) & 0x3)

#define LEVELS_PER_SLICE        128

struct slice_data {
	unsigned long irq_enable_mask[2];
	int level_to_irq[LEVELS_PER_SLICE];
};

struct hub_data {
	cpumask_t	h_cpus;
	unsigned long slice_map;
	unsigned long irq_alloc_mask[2];
	struct slice_data slice[2];
};

struct node_data {
	struct pglist_data pglist;
	struct hub_data hub;
	cpumask_t cpumask;
};

extern struct node_data *__node_data[];

#define NODE_DATA(n)		(&__node_data[(n)]->pglist)
#define hub_data(n)		(&__node_data[(n)]->hub)

#endif /* _ASM_MACH_MMZONE_H */
