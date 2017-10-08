#ifndef __DISKIO_H
#define __DISKIO_H

#define DISKIO_INT	1000000	/* wait 1 sec to read disk usage */
#define DISKSTATS_BUF	1024	/* buffer size when reading /proc/diskstats */
#define DISKNAME_SIZE	32	/* maximum length of the disk name */

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

void	read_diskio (void);
void	clean_diskio (void);

#endif

/* vim: set ts=8 sw=8 noet: */
