#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#define _GNU_SOURCE
#include <pthread.h>
#undef _GNU_SOURCE
#include <signal.h>
#include "workload.h"

int		NCPU;
pthread_t	display_thread;
double		maxrkbps, maxwkbps, maxtotalkbps;

static const char usage[] =
"Usage: ./workload [-h] [-r max_read] [-w max_write] [-t max_total]\n\n"
"Options:\n"
"        -h      show this message\n"
"        -r      specify the max reading rate of the disk\n"
"        -w      specify the max writing rate of the disk\n"
"        -t      specify the total max IO rate of the disk\n\n";

static inline void mon_init (void);
static inline void mon_fin (void);
static void display (void);	/* display the screen with refreshing rate */
static void draw (void);
static void winch_handler (int);

int main (int argc, char **argv)
{
	int	c;

	/* parse options */
	while ((c = getopt (argc, argv, "hr:w:t:")) != -1) {
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
		case 'h':
		case '?':
			fputs (usage, stderr);
			return 0;
		default:
			return -1;
		}
	}

	/* initialize monitor */
	mon_init ();

	while ((c = getch ()) != ERR) {
		if (c == 'q' || c == '')
			break;
	}

	/* finalize monitor */
	mon_fin ();

	return 0;
}

static void draw (void)
{
	struct disk_t	*d;
	int		r = 1, max_r, max_c;

	clear ();
	getmaxyx (stdscr, max_r, max_c);

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

	mvaddstr (max_r - 2, 2, "q: Quit");

	refresh ();
}

static void display (void)
{
	while (1) {
		read_cpu ();
		read_mem ();
		read_diskio ();
		draw ();
		usleep (INTERVAL);
	}
}

static void winch_handler (int sig)
{
	draw ();
}

void error (const char *s)
{
	mon_fin ();
	if (s)
		fprintf (stderr, "[Error] %s\n", s);
	exit (!!s);
}

static inline void mon_init (void)
{
	NCPU = sysconf(_SC_NPROCESSORS_ONLN);

	/* start curses mode */
	initscr ();
	curs_set (0);		/* make the cursor invisible */
	raw ();			/* terminal raw mode */
	keypad (stdscr, TRUE);	/* enable keypad */
	noecho ();		/* no echo */

	signal (SIGWINCH, winch_handler);

	/* create a thread displaying the screen */
	if (pthread_create (&display_thread, NULL, (void *(*)(void *)) &display, NULL) != 0)
		error ("failed to create thread for display");
}

static inline void mon_fin (void)
{
	/* end curses mode */
	endwin ();
	clean_cpu ();
	clean_diskio ();
}

/* vim: set ts=8 sw=8 noet: */
