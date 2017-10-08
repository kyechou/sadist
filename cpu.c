#include <stdio.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <pthread.h>
#undef _GNU_SOURCE
#include "monitor.h"

double cpu_usage;	/* (working time) / (working time + idle time) * 100 */
static pthread_t cpu_thread;
static FILE *fin;

static void threaded_read_cpu (void)
{
	double	s[4], t[4];

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
}

void read_cpu (void)
{
	if (pthread_create (&cpu_thread, NULL, (void *(*)(void *)) &threaded_read_cpu, NULL) != 0)
		error ("failed to create thread for calculating cpu usage");
}

void clean_cpu (void)
{
	pthread_join (cpu_thread, NULL);
	fclose (fin);
}

/* vim: set ts=8 sw=8 noet: */
