#include <stdio.h>
#include "workload.h"

unsigned long	memtotal;	/* MemTotal */
unsigned long	memused;	/* memtotal - memfree - membuffers - memcached - memsreclaim + memshmem */
double		mem_usage;	/* memused / memtotal * 100 */

static unsigned long	memfree;	/* MemFree */
static unsigned long	membuffers;	/* Buffers */
static unsigned long	memcached;	/* Cached */
static unsigned long	memshmem;	/* Shmem */
static unsigned long	memsreclaim;	/* SReclaimable */

void read_mem (void)
{
	FILE	*fin;

	if ((fin = fopen ("/proc/meminfo", "r")) == NULL)
		error ("failed to open /proc/meminfo");
	fscanf (fin,
		"%*[^0-9]%lu"	/* MemTotal       */
		"%*[^0-9]%lu"	/* MemFree        */
		"%*[^0-9]%*lu"	/* MemAvailable   */
		"%*[^0-9]%lu"	/* Buffers        */
		"%*[^0-9]%lu"	/* Cached         */
		"%*[^0-9]%*lu"	/* SwapCached     */
		"%*[^0-9]%*lu"	/* Active         */
		"%*[^0-9]%*lu"	/* Inactive       */
		"%*[^0-9]%*lu"	/* Active(anon)   */
		"%*[^0-9]%*lu"	/* Inactive(anon) */
		"%*[^0-9]%*lu"	/* Active(file)   */
		"%*[^0-9]%*lu"	/* Inactive(file) */
		"%*[^0-9]%*lu"	/* Unevictable    */
		"%*[^0-9]%*lu"	/* Mlocked        */
		"%*[^0-9]%*lu"	/* SwapTotal      */
		"%*[^0-9]%*lu"	/* SwapFree       */
		"%*[^0-9]%*lu"	/* Dirty          */
		"%*[^0-9]%*lu"	/* Writeback      */
		"%*[^0-9]%*lu"	/* AnonPages      */
		"%*[^0-9]%*lu"	/* Mapped         */
		"%*[^0-9]%lu"	/* Shmem          */
		"%*[^0-9]%*lu"	/* Slab           */
		"%*[^0-9]%lu"	/* SReclaimable   */
		,
		&memtotal,
		&memfree,
		&membuffers,
		&memcached,
		&memshmem,
		&memsreclaim);
	fclose (fin);
	memused = memtotal - memfree - membuffers - memcached - memsreclaim + memshmem;
	mem_usage = (double)memused / (double)memtotal * 100.0;
}

/* vim: set ts=8 sw=8 noet: */
