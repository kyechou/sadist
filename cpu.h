#ifndef __CPU_H
#define __CPU_H

#define CPU_INT		1000000	/* wait for 1 sec to read cpu usage */
#define CPU_HOG_GRAN	200000	/* cpu hog granularity: 200 ms */

extern double cpu_usage;	/* (working time) / (working time + idle time) * 100 */

void cpu_init (void);
void cpu_fin (void);
void read_cpu (void);
void stress_cpu (void);

#endif

/* vim: set ts=8 sw=8 noet: */
