#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#define _GNU_SOURCE
#include <pthread.h>
#undef _GNU_SOURCE
#include "monitor.h"

#define INTERVAL 2000	/* update every 2 sec */
#define CPU_INT  1000000	/* wait 1 sec to read cpu usage */

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
	pre_draw ();

	while (1) {
		draw ();
		refresh ();
		c = getch ();
		if (c == 'q' || c == '') {
			break;
		} else if (c == KEY_RESIZE) {	/* handling SIGWINCH signal would be more compatible */
			clear ();
			pre_draw ();
		}
	}

	/* end curses mode */
	endwin();

	return 0;
}

void pre_draw (void)
{
	/* show CPU usage before the blocking calculation */
	mvprintw (1, 2, "CPU: %5.1lf%%", cpu_usage);
}

void draw (void)
{
	pthread_t	cpu_thread;

	if (pthread_create (&cpu_thread, NULL, (void *(*)(void *)) &draw_cpu, NULL) != 0)
		error ("failed to create thread for calculating cpu usage");
	draw_mem ();
}

void draw_cpu (void)
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

	mvprintw (1, 2, "CPU: %5.1lf%%", cpu_usage);
}

void draw_mem (void)
{
	FILE	*fin;

	if ((fin = fopen ("/proc/meminfo", "r")) == NULL)
		error ("failed to open /proc/meminfo");
	fscanf (fin,
		"%*[^0-9]%ld"	/* MemTotal       */
		"%*[^0-9]%ld"	/* MemFree        */
		"%*[^0-9]%*ld"	/* MemAvailable   */
		"%*[^0-9]%ld"	/* Buffers        */
		"%*[^0-9]%ld"	/* Cached         */
		"%*[^0-9]%*ld"	/* SwapCached     */
		"%*[^0-9]%*ld"	/* Active         */
		"%*[^0-9]%*ld"	/* Inactive       */
		"%*[^0-9]%*ld"	/* Active(anon)   */
		"%*[^0-9]%*ld"	/* Inactive(anon) */
		"%*[^0-9]%*ld"	/* Active(file)   */
		"%*[^0-9]%*ld"	/* Inactive(file) */
		"%*[^0-9]%*ld"	/* Unevictable    */
		"%*[^0-9]%*ld"	/* Mlocked        */
		"%*[^0-9]%*ld"	/* SwapTotal      */
		"%*[^0-9]%*ld"	/* SwapFree       */
		"%*[^0-9]%*ld"	/* Dirty          */
		"%*[^0-9]%*ld"	/* Writeback      */
		"%*[^0-9]%*ld"	/* AnonPages      */
		"%*[^0-9]%*ld"	/* Mapped         */
		"%*[^0-9]%ld"	/* Shmem          */
		"%*[^0-9]%*ld"	/* Slab           */
		"%*[^0-9]%ld"	/* SReclaimable   */
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

	mvprintw (3, 2, "Memory: %5.1lf%% (%ld kB / %ld kB)", mem_usage, memused, memtotal);
}

void error (const char *s)
{
	endwin();
	if (s)
		fprintf (stderr, "[Error] %s\n", s);
	exit (!!s);
}

/* vim: set ts=8 sw=8 noet: */
