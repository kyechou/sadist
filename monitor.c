#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#define _GNU_SOURCE
#include <pthread.h>
#undef _GNU_SOURCE
#include <signal.h>
#include "monitor.h"

int		NCPU;
pthread_t	display_thread;

static inline void init (void);
static inline void fin (void);
static void display (void);	/* display the screen with refreshing rate */
static void draw (void);
static void winch_handler (int);

static inline void init (void)
{
	NCPU = sysconf(_SC_NPROCESSORS_ONLN);

	/* start curses mode */
	initscr ();
	curs_set (0);		/* make the cursor invisible */
	raw ();			/* terminal raw mode */
	keypad (stdscr, TRUE);	/* enable keypad */
	noecho ();		/* no echo */

	signal (SIGWINCH, winch_handler);
}

static inline void fin (void)
{
	/* end curses mode */
	endwin ();
	clean_cpu ();
	clean_diskio ();
}

int main (void)
{
	int	c;

	init ();

	/* create a thread displaying the screen */
	if (pthread_create (&display_thread, NULL, (void *(*)(void *)) &display, NULL) != 0)
		error ("failed to create thread for display");

	while ((c = getch ()) != ERR) {
		if (c == 'q' || c == '')
			break;
	}

	fin ();

	return 0;
}

static void draw (void)
{
	struct disk_t	*d;
	int		r = 1;

	mvprintw (r, 2, "CPU: %5.1lf%%", cpu_usage); clrtoeol ();
	r += 2;
	mvprintw (r, 2, "Memory: %5.1lf%% (%ld kB / %ld kB)", mem_usage, memused, memtotal); clrtoeol ();
	r += 2;
	mvaddstr (r, 2, "Disk I/O:");
	r += 2;
	for (d = disks.head; d != NULL; d = d->next, r += 4) {
		mvprintw (r, 6, "- %s", d->name); clrtoeol ();
		mvprintw (r + 1, 10,  "read:  %.1lf kB/sec", d->rkbps); clrtoeol ();
		mvprintw (r + 2, 10,  "write: %.1lf kB/sec", d->wkbps); clrtoeol ();
		mvprintw (r + 3, 10, "total: %.1lf kB/sec", d->totalkbps); clrtoeol ();
	}

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
	clear ();
	draw ();
}

void error (const char *s)
{
	fin ();
	if (s)
		fprintf (stderr, "[Error] %s\n", s);
	exit (!!s);
}

/* vim: set ts=8 sw=8 noet: */
