#ifndef __MONITOR_H
#define __MONITOR_H

void	pre_draw (void);	/* preparations before drawing */
void	draw (void);		/* construct the screen output */
void	draw_cpu (void);	/* draw the CPU utilization */
void	draw_mem (void);	/* draw the memory utilization */
void	draw_diskio (void);	/* draw the disk IO utilization */
void	error (const char *);	/* report error and quit */

#endif
