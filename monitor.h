#ifndef __MONITOR_H
#define __MONITOR_H

#define DISKNAME_SIZE 32

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

void	error (const char *);	/* report error and quit */
void	display (void);		/* display the screen with refreshing rate */

void	draw_cpu (void);	/* draw the CPU utilization */
void	read_cpu (void);	/* blocking; running with pthread */

void	draw_mem (void);	/* draw the memory utilization */
void	read_mem (void);

void	draw_diskio (void);	/* draw the disk IO utilization */
void	read_diskio (void);	/* blocking; running with pthread */
void	read_diskstats (int);
int	update_disk (const struct disk_t *, int);
void	append_disk (const struct disk_t *);
void	free_disks (void);

#endif
