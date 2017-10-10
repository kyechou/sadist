#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#undef _GNU_SOURCE
#include "workload.h"

double cpu_usage;	/* (working time) / (working time + idle time) * 100 */
static int NR_CPU;
static FILE *fin;
static pthread_t readcpu_thread;
static pthread_t *stresscpu_threads;

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
	if (pthread_create (&readcpu_thread, NULL, (void *(*)(void *)) &threaded_read_cpu, NULL) != 0)
		error ("failed to create thread for calculating cpu usage");
	pthread_detach (readcpu_thread);
}

void readcpu_fin (void)
{
	pthread_cancel (readcpu_thread);
	pthread_join (readcpu_thread, NULL);
	fclose (fin);
}

static void stresscpu_fin (void)
{
	for (int i = 0; i < NR_CPU; ++i) {
		pthread_cancel (stresscpu_threads[i]);
		pthread_join (stresscpu_threads[i], NULL);
	}
	free (stresscpu_threads);
}

static void threaded_stress_cpu (void)
{
	while (1);
}

static void rest (int sig)
{
	usleep ((100.0 - workload[M_CPU]) / 100 * CPU_HOG_GRAN);
}

void stress_cpu (void)
{
	int i;
	cpu_set_t cpuset;

	NR_CPU = sysconf (_SC_NPROCESSORS_ONLN);
	stresscpu_threads = malloc (sizeof (pthread_t) * NR_CPU);

	/* install the signal handler */
	signal (SIGUSR1, &rest);

	/* create threads and set affinity */
	for (i = 0; i < NR_CPU; ++i) {
		if (pthread_create (&stresscpu_threads[i], NULL, (void *(*)(void *)) &threaded_stress_cpu, NULL) != 0)
			error ("failed to create thread for cpu stress worker");
		pthread_detach (stresscpu_threads[i]);
		CPU_ZERO (&cpuset);
		CPU_SET (i, &cpuset);
		if (pthread_setaffinity_np (stresscpu_threads[i], sizeof (cpu_set_t), &cpuset) != 0)
			error ("failed to set affinity for cpu stress worker");
	}

	/* push a cleanup function */
	pthread_cleanup_push ((void (*)(void *)) &stresscpu_fin, NULL);

	/* control the stress timing */
	while (1) {
		/* send signals to threads */
		for (i = 0; i < NR_CPU; ++i)
			pthread_kill (stresscpu_threads[i], SIGUSR1);

		usleep (CPU_HOG_GRAN);
	}

	/* pop a cleanup function */
	pthread_cleanup_pop (0);
}

/* vim: set ts=8 sw=8 noet: */
