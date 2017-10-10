#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#undef _GNU_SOURCE
#include "workload.h"

struct disks_t	disks = { NULL, NULL };
static pthread_t	readdiskio_thread;
static FILE		*fin;

static void	threaded_read_diskio (void);	/* blocking; running with pthread */
static void	read_diskstats (int);
static int	update_disk (const struct disk_t *, int);
static void	append_disk (const struct disk_t *);
static void	free_disks (void);

void read_diskio (void)
{
	if (pthread_create (&readdiskio_thread, NULL, (void *(*)(void *)) &threaded_read_diskio, NULL) != 0)
		error ("failed to create thread for calculating diskio usage");
	pthread_detach (readdiskio_thread);
}

void readdiskio_fin (void)
{
	pthread_cancel (readdiskio_thread);
	pthread_join (readdiskio_thread, NULL);
	fclose (fin);
	free_disks ();
	/* temporarily ignore new_disk */
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

static int fd;
static void *buffer;
static char filename[] = "tmp.XXXXXX";

static void stressdiskio_fin (void)
{
	close (fd);
	free (buffer);
	if (access (filename, F_OK) != -1)
		unlink (filename);
}

static double diff_in_useconds (const struct timespec *b, const struct timespec *e)
{
	struct timespec diff;

	if (e->tv_nsec - b->tv_nsec < 0) {
		diff.tv_sec  = e->tv_sec  - b->tv_sec - 1;
		diff.tv_nsec = e->tv_nsec - b->tv_nsec + 1000000000;
	} else {
		diff.tv_sec  = e->tv_sec  - b->tv_sec;
		diff.tv_nsec = e->tv_nsec - b->tv_nsec;
	}

	return (diff.tv_sec * 1000000.0 + diff.tv_nsec / 1000.0);
}

double stress_rate;	/* kB/sec */

void stress_diskio (void)
{
	struct timespec start, end;
	double remaining;	/* usec */
	size_t nr_bytes;

	fd = -1;
	buffer = NULL;

	/* push a cleanup function */
	pthread_cleanup_push ((void (*)(void *)) &stressdiskio_fin, NULL);

	while (1) {
		/* calculate the rate and the number of bytes to write */
		if (maxwkbps)
			stress_rate = workload[M_DISKIO] / 100 * maxwkbps;
		else if (maxtotalkbps)
			stress_rate = workload[M_DISKIO] / 100 * maxtotalkbps;
		else
			stress_rate = 0;
		nr_bytes = stress_rate * DISK_HOG_GRAN / 1000000.0 * 1024;

		/* time start */
		clock_gettime (CLOCK_REALTIME, &start);

		/* write 'nr_bytes' bytes to disk */
		if (nr_bytes) {
			/* create file at PWD */
			for (int i = 4; i < 10; ++i)
				filename[i] = 'X';
			if ((fd = mkostemp (filename, O_SYNC)) < 0)
				error ("failed to open file for disk I/O");
			/* allocate memory buffer */
			if ((buffer = realloc (buffer, nr_bytes)) == NULL)
				error ("failed to allocate memory buffer for disk I/O");
			/* write to the file */
			if (write (fd, buffer, nr_bytes) < 0)
				error ("failed to write to the file");
			/* close and remove the file */
			close (fd);
			unlink (filename);
		}

		/* time end */
		clock_gettime (CLOCK_REALTIME, &end);

		/* sleep through the remaining time */
		if ((remaining = DISK_HOG_GRAN - diff_in_useconds (&start, &end)) > 0)
			usleep (remaining);
	}

	/* pop a cleanup function */
	pthread_cleanup_pop (0);
}

/* vim: set ts=8 sw=8 noet: */
