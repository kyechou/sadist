#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#define _GNU_SOURCE
#include <pthread.h>
#undef _GNU_SOURCE
#include "monitor.h"

#define INTERVAL   1500		/* update every 1.5 sec */
#define CPU_INT    1000000	/* wait 1 sec to read cpu usage */
#define DISKIO_INT 1000000	/* wait 1 sec to read disk usage */
#define DISKSTATS_BUF 1024	/* buffer size when reading /proc/diskstats */

int	NCPU;

unsigned long	memtotal;	/* MemTotal */
unsigned long	memfree;	/* MemFree */
unsigned long	membuffers;	/* Buffers */
unsigned long	memcached;	/* Cached */
unsigned long	memshmem;	/* Shmem */
unsigned long	memsreclaim;	/* SReclaimable */
unsigned long	memused;	/* memtotal - memfree - membuffers - memcached - memsreclaim + memshmem */
double		mem_usage;	/* memused / memtotal * 100 */
double		cpu_usage;	/* (working time) / (working time + idle time) * 100 */
struct disks_t	disks = { NULL, NULL };

int main (void)
{
	int	c;

	NCPU = sysconf(_SC_NPROCESSORS_ONLN);

	/* start curses mode */
	initscr ();
	curs_set (0);		/* make the cursor invisible */
	raw ();			/* terminal raw mode */
	keypad (stdscr, TRUE);	/* enable keypad */
	noecho ();		/* no echo */
	timeout (INTERVAL);	/* input blocking for INTERVAL milliseconds */

	while (1) {
		draw_cpu ();
		draw_mem ();
		draw_diskio ();
		refresh ();

		c = getch ();
		if (c == 'q' || c == '')
			break;
		else if (c == KEY_RESIZE)	/* handling SIGWINCH signal would be more compatible */
			clear ();
	}

	/* end curses mode */
	endwin ();
	free_disks ();

	return 0;
}

void draw_cpu (void)
{
	static pthread_t	thread;

	if (pthread_create (&thread, NULL, (void *(*)(void *)) &read_cpu, NULL) != 0)
		error ("failed to create thread for calculating cpu usage");

	mvprintw (1, 2, "CPU: %5.1lf%%", cpu_usage);
}

void read_cpu (void)
{
	double	s[4], t[4];
	FILE	*fin;

	if ((fin = fopen ("/proc/stat", "r")) == NULL)
		error ("failed to open /proc/stat");
	fscanf (fin, "%*s %lf %lf %lf %lf", &s[0], &s[1], &s[2], &s[3]);
	fclose (fin);
	usleep (CPU_INT);
	if ((fin = fopen ("/proc/stat", "r")) == NULL)
		error ("failed to open /proc/stat");
	fscanf (fin, "%*s %lf %lf %lf %lf", &t[0], &t[1], &t[2], &t[3]);
	fclose (fin);
	cpu_usage = ((t[0] + t[1] + t[2]) - (s[0] + s[1] + s[2])) /
		((t[0] + t[1] + t[2] + t[3]) - (s[0] + s[1] + s[2] + s[3])) *
		100.0;
}

void draw_mem (void)
{
	read_mem ();
	mvprintw (3, 2, "Memory: %5.1lf%% (%ld kB / %ld kB)", mem_usage, memused, memtotal);
}

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

void draw_diskio (void)
{
	static pthread_t	thread;
	struct disk_t		*d;
	int			i;

	if (pthread_create (&thread, NULL, (void *(*)(void *)) &read_diskio, NULL) != 0)
		error ("failed to create thread for calculating diskio usage");

	mvaddstr (5, 2, "Disk I/O:");
	for (d = disks.head, i = 0; d != NULL; d = d->next, i += 4) {
		mvprintw (7 + i, 6, "- %s", d->name);
		mvprintw (8 + i, 10,  "read:  %.1lf kB/sec", d->rkbps);
		mvprintw (9 + i, 10,  "write: %.1lf kB/sec", d->wkbps);
		mvprintw (10 + i, 10, "total: %.1lf kB/sec", d->totalkbps);
	}
}

void read_diskio (void)
{
	struct disk_t *d;

	read_diskstats (0);
	usleep (DISKIO_INT);
	read_diskstats (1);

	for (d = disks.head; d != NULL; d = d->next) {
		d->rkbps = (double)(d->rsect[1] - d->rsect[0]) / 2.0
			* 1000000.0 / (double)(DISKIO_INT);
		d->wkbps = (double)(d->wsect[1] - d->wsect[0]) / 2.0
			* 1000000.0 / (double)(DISKIO_INT);
		d->totalkbps = (double)(d->rsect[1] - d->rsect[0] + d->wsect[1] - d->wsect[0]) / 2.0
			* 1000000.0 / (double)(DISKIO_INT);
	}
}

void read_diskstats (int sample)
{
	FILE		*fin;
	char		entry[DISKSTATS_BUF];
	struct disk_t	disk;

	if ((fin = fopen ("/proc/diskstats", "r")) == NULL)
		error ("failed to open /proc/diskstats");
	while (fgets (entry, DISKSTATS_BUF, fin) != NULL) {
		sscanf (entry, "%d %d %s %*lu %*lu %lu %*lu %*lu %*lu %lu",
			&(disk.major),
			&(disk.minor),
			disk.name,
			&(disk.rsect[sample]),
			&(disk.wsect[sample]));
		if (disk.major != 8 || (disk.minor % 16 != 0))
			continue;
		if (update_disk (&disk, sample) < 0 && sample == 0)
			append_disk (&disk);
	}
	fclose (fin);
}

int update_disk (const struct disk_t *disk, int sample)
{
	struct disk_t *d;
	for (d = disks.head; d != NULL; d = d->next) {
		if (d->major != disk->major || d->minor != disk->minor)
			continue;
		d->rsect[sample] = disk->rsect[sample];
		d->wsect[sample] = disk->wsect[sample];
		return 0;
	}
	return -1;
}

void append_disk (const struct disk_t *disk)
{
	struct disk_t	*new_disk;

	if ((new_disk = malloc (sizeof (struct disk_t))) == NULL)
		error ("failed to allocate memory for new disk structure");
	(*new_disk) = (*disk);

	new_disk->previous = disks.tail;
	if (disks.tail)
		disks.tail->next = new_disk;
	else
		disks.head = new_disk;
	new_disk->next = NULL;
	disks.tail = new_disk;
}

void free_disks (void)
{
	struct disk_t *d = disks.head, *n;
	while (d != NULL) {
		n = d->next;
		free (d);
		d = n;
	}
}

void error (const char *s)
{
	endwin();
	free_disks ();
	if (s)
		fprintf (stderr, "[Error] %s\n", s);
	exit (!!s);
}

/* vim: set ts=8 sw=8 noet: */
