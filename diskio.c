#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#undef _GNU_SOURCE
#include "workload.h"

struct disks_t	disks = { NULL, NULL };
static pthread_t	diskio_thread;
static FILE		*fin;

static void	threaded_read_diskio (void);	/* blocking; running with pthread */
static void	read_diskstats (int);
static int	update_disk (const struct disk_t *, int);
static void	append_disk (const struct disk_t *);
static void	free_disks (void);

void read_diskio (void)
{
	if (pthread_create (&diskio_thread, NULL, (void *(*)(void *)) &threaded_read_diskio, NULL) != 0)
		error ("failed to create thread for calculating diskio usage");
}

void clean_diskio (void)
{
	pthread_join (diskio_thread, NULL);
	fclose (fin);
	free_disks ();
	/* temporarily ignore the new_disk */
}

void stress_diskio (void)
{
}

static void threaded_read_diskio (void)
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

static void read_diskstats (int sample)
{
	char		entry[DISKSTATS_BUF];
	struct disk_t	disk = { 0 };

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

static int update_disk (const struct disk_t *disk, int sample)
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

static void append_disk (const struct disk_t *disk)
{
	struct disk_t	*new_disk;

	if ((new_disk = malloc (sizeof (struct disk_t))) == NULL)
		error ("failed to allocate memory for new disk structure");
	(*new_disk) = (*disk);

	new_disk->previous = disks.tail;
	new_disk->next = NULL;
	if (disks.tail)
		disks.tail->next = new_disk;
	else
		disks.head = new_disk;
	disks.tail = new_disk;
}

static void free_disks (void)
{
	struct disk_t *d = disks.head, *n;
	while (d != NULL) {
		n = d->next;
		free (d);
		d = n;
	}
}

/* vim: set ts=8 sw=8 noet: */
