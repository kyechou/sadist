#ifndef __DISKIO_H
#define __DISKIO_H

#define DISKIO_INT	1000000	/* (usec) wait 1 sec to read disk usage */
#define DISKSTATS_BUF	1024	/* buffer size when reading /proc/diskstats */
#define DISKNAME_SIZE	32	/* maximum length of the disk name */
#define DISK_HOG_GRAN	1000000	/* (usec) disk hog granularity: 1 sec */

struct disk_t {
	int		major, minor;
	char		name[DISKNAME_SIZE];
	unsigned long	rsect[2], wsect[2];
	double		rkbps, wkbps, totalkbps;
	struct disk_t	*previous, *next;
};

struct disks_t {
	struct disk_t	*head, *tail;
};

extern struct disks_t	disks;
extern double stress_rate;	/* kB/sec */

void read_diskio (void);
void readdiskio_fin (void);
void stress_diskio (void);

#endif

/* vim: set ts=8 sw=8 noet: */
