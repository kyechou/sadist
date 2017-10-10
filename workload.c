#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#undef _GNU_SOURCE
#include "workload.h"

static const char usage[] =
"Usage: ./workload [-h] [-r max_read] [-w max_write] [-t max_total]\n"
"                  [-c cpu_workload] [-m mem_workload] [-d disk_IO_workload]\n\n"
"Options:\n"
"        -h      show this message\n"
"        -r      the max reading rate of the disk\n"
"        -w      the max writing rate of the disk\n"
"        -t      the max total IO rate of the disk\n"
"        -c      the percentage of additional workload on CPU (0-100)\n"
"        -m      the percentage of additional workload on memory (0-100)\n"
"        -d      the percentage of additional workload on disk (0-100)\n\n";

/* UI input mode; constants defined in workload.h */
static int mode;
/* additional workload target */
double	workload[NR_MODE];
/* maxima of disk IO rate */
double	maxrkbps;
double	maxwkbps;
double	maxtotalkbps;
/* workload threads */
static pthread_t threads[NR_MODE];
/* monitor thread */
static pthread_t monitor_thread;

static inline void mon_init (void);
static inline void wl_init (void);
static inline void wl_fin (void);
static inline void mon_fin (void);
static inline int parse_args (int, char **);
static void monitor (void);
static void draw (void);
static void winch_handler (int);
static void input (double);

int main (int argc, char **argv)
{
	int	c;

	if (parse_args (argc, argv))
		return 1;

	/* initialize monitor */
	mon_init ();
	/* initialize workload */
	wl_init ();

	while ((c = getch ()) != ERR) {
		c = tolower (c);
		switch (c) {
		case 'q':
		case '':
			error (NULL);
		case 'j':
			input (-1);
			break;
		case 'k':
			input (1);
			break;
		case 'h':
			input (-10);
			break;
		case 'l':
			input (10);
			break;
		case '0':
			input (0);
			break;
		case 'c':
			if (mode == M_NORMAL)
				mode = M_CPU;
			break;
		case 'm':
			if (mode == M_NORMAL)
				mode = M_MEM;
			break;
		case 'd':
			if (mode == M_NORMAL)
				mode = M_DISKIO;
			break;
		case '':
			if (mode != M_NORMAL)
				mode = M_NORMAL;
			break;
		default:
			continue;
		}
		draw ();
	}

	/* finalize workload */
	wl_fin ();
	/* finalize monitor */
	mon_fin ();

	return 0;
}

static void input (double delta)
{
	if (mode == M_NORMAL)
		return;
	if (delta == 0) {
		workload[mode] = 0;
		return;
	}
	if (workload[mode] + delta < 0 || workload[mode] + delta > 100)
		return;
	workload[mode] += delta;
}

static void draw (void)
{
	struct disk_t	*d;
	int		r = 1, max_r, max_c;

	clear ();
	getmaxyx (stdscr, max_r, max_c);

	mvaddstr (r, 1, "------------------------------------------------------------------------");
	r += 2;
	mvprintw (r, 2, "CPU: %5.1lf%%", cpu_usage);
	r += 2;
	mvprintw (r, 2, "Memory: %5.1lf%% (%ld kB / %ld kB)", mem_usage, memused, memtotal);
	r += 2;
	mvaddstr (r, 2, "Disk I/O:");
	r += 2;
	for (d = disks.head; d != NULL; d = d->next, r += 4) {
		mvprintw (r, 6, "- %s", d->name);
		mvprintw (r + 1, 10,  "read:  %.1lf kB/sec", d->rkbps);
		if (maxrkbps)
			printw (" (%.1lf%%)", d->rkbps / maxrkbps * 100.0);
		mvprintw (r + 2, 10,  "write: %.1lf kB/sec", d->wkbps);
		if (maxwkbps)
			printw (" (%.1lf%%)", d->wkbps / maxwkbps * 100.0);
		mvprintw (r + 3, 10, "total: %.1lf kB/sec", d->totalkbps);
		if (maxtotalkbps)
			printw (" (%.1lf%%)", d->totalkbps / maxtotalkbps * 100.0);
	}
	r += 2;
	mvaddstr (r, 1, "------------------------------------------------------------------------");
	r += 2;
	mvprintw (r, 2, "CPU workload:      +%.1lf%%", workload[M_CPU]);
	r += 2;
	mvprintw (r, 2, "Memory workload:   +%.1lf%%", workload[M_MEM]);
	printw (" (%.1lf MB; %.1lf KB; %.1lf bytes)",
		memsize / 1024.0 / 1024.0,
		memsize / 1024.0,
		memsize);
	r += 2;
	mvprintw (r, 2, "Disk I/O workload: +%.1lf%%", workload[M_DISKIO]);
	printw (" (%.1lf MB/s; %.1lf KB/s; %.1lf bytes/s)",
		stress_rate / 1024.0, stress_rate, stress_rate * 1024.0);
	r += 2;
	mvaddstr (r, 1, "------------------------------------------------------------------------");

	mvaddstr (max_r - 4, 1, "------------------------------------------------------------------------");
	mvaddstr (max_r - 3, 2, "q: Quit");
	switch (mode) {
	case M_NORMAL:
		addstr ("   c: CPU   m: Memory   d: Disk");
		break;
	case M_CPU:
		addstr ("   Esc: Back   h: -10%   j: -1%   k: +1%   l: +10%   0: 0%");
		break;
	case M_MEM:
		addstr ("   Esc: Back   h: -10%   j: -1%   k: +1%   l: +10%   0: 0%");
		break;
	case M_DISKIO:
		addstr ("   Esc: Back   h: -10%   j: -1%   k: +1%   l: +10%   0: 0%");
		break;
	}
	mvaddstr (max_r - 2, 1, "------------------------------------------------------------------------");

	refresh ();
}

static void monitor (void)
{
	while (1) {
		read_cpu ();
		read_mem ();
		read_diskio ();
		draw ();
		usleep (INTERVAL);
	}
}

static inline int parse_args (int argc, char **argv)
{
	int c;

	while ((c = getopt (argc, argv, "hr:w:t:c:m:d:")) != -1) {
		switch (c) {
		case 'r':
			maxrkbps = atof (optarg);
			break;
		case 'w':
			maxwkbps = atof (optarg);
			break;
		case 't':
			maxtotalkbps = atof (optarg);
			break;
		case 'c':
			workload[M_CPU] = atof (optarg);
			break;
		case 'm':
			workload[M_MEM] = atof (optarg);
			break;
		case 'd':
			workload[M_DISKIO] = atof (optarg);
			break;
		case 'h':
		case '?':
		default:
			fputs (usage, stderr);
			return 1;
		}
	}

	if (maxrkbps < 0 || maxwkbps < 0 || maxtotalkbps < 0
		|| workload[M_CPU] < 0 || workload[M_CPU] > 100
		|| workload[M_MEM] < 0 || workload[M_MEM] > 100
		|| workload[M_DISKIO] < 0 || workload[M_DISKIO] > 100) {
		fputs ("[Error] invalid argument\n", stderr);
		fputs (usage, stderr);
		return 1;
	}

	return 0;
}

static void winch_handler (int sig)
{
	draw ();
}

void error (const char *s)
{
	wl_fin ();
	mon_fin ();
	if (s)
		fprintf (stderr, "[Error] %s\n", s);
	exit (!!s);
}

static inline void mon_init (void)
{
	mode = M_NORMAL;

	/* set ESCDELAY to make it respond immediately */
	if (putenv ("ESCDELAY=10") != 0)
		error ("failed to set environment variable ESCDELAY");

	/* start curses mode */
	initscr ();
	curs_set (0);		/* make the cursor invisible */
	raw ();			/* terminal raw mode */
	keypad (stdscr, TRUE);	/* enable keypad */
	noecho ();		/* no echoing */

	signal (SIGWINCH, winch_handler);

	/* create a thread for monitoring */
	if (pthread_create (&monitor_thread, NULL, (void *(*)(void *)) &monitor, NULL) != 0)
		error ("failed to create thread for monitoring");
	pthread_detach (monitor_thread);
}

static inline void wl_init (void)
{
	if (pthread_create (&threads[M_CPU], NULL, (void *(*)(void *)) &stress_cpu, NULL) != 0)
		error ("failed to create thread for cpu stress");
	pthread_detach (threads[M_CPU]);

	if (pthread_create (&threads[M_MEM], NULL, (void *(*)(void *)) &stress_mem, NULL) != 0)
		error ("failed to create thread for mem stress");
	pthread_detach (threads[M_MEM]);

	if (pthread_create (&threads[M_DISKIO], NULL, (void *(*)(void *)) &stress_diskio, NULL) != 0)
		error ("failed to create thread for diskio stress");
	pthread_detach (threads[M_DISKIO]);
}

static inline void wl_fin (void)
{
	for (int i = 0; i < NR_MODE; ++i) {
		pthread_cancel (threads[i]);
		pthread_join (threads[i], NULL);
	}
}

static inline void mon_fin (void)
{
	/* stop the monitor thread */
	pthread_cancel (monitor_thread);
	readcpu_fin ();
	readdiskio_fin ();
	pthread_join (monitor_thread, NULL);

	/* end curses mode */
	endwin ();
}

/* vim: set ts=8 sw=8 noet: */
