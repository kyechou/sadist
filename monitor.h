#ifndef __MONITOR_H
#define __MONITOR_H

int	NCPU;

long	memtotal;	/* MemTotal */
long	memfree;	/* MemFree */
long	membuffers;	/* Buffers */
long	memcached;	/* Cached */
long	memshmem;	/* Shmem */
long	memsreclaim;	/* SReclaimable */
long	memused;	/* memtotal - memfree - membuffers - memcached - memsreclaim + memshmem */
double	mem_usage;	/* memused / memtotal * 100 */
double	cpu_usage;	/* (working time) / (working time + idle time) * 100 */
double	io_usage;	/* disk IO utilization */
double	load[3];	/* CPU load */

void	pre_draw (void);	/* preparations before drawing */
void	draw (void);		/* construct the screen output */
void	draw_cpu (void);	/* draw the CPU utilization */
void	draw_mem (void);	/* draw the memory utilization */
void	error (const char *);	/* report error and quit */

#endif
