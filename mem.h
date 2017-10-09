#ifndef __MEM_H
#define __MEM_H

extern unsigned long	memtotal;	/* MemTotal */
extern unsigned long	memused;	/* memtotal - memfree - membuffers - memcached - memsreclaim + memshmem */
extern double		mem_usage;	/* memused / memtotal * 100 */

void read_mem (void);
void stress_mem (void);

#endif

/* vim: set ts=8 sw=8 noet: */
