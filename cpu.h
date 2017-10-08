#ifndef __CPU_H
#define __CPU_H

#define CPU_INT 1000000		/* wait for 1 sec to read cpu usage */

extern double cpu_usage;	/* (working time) / (working time + idle time) * 100 */

void read_cpu (void);
void clean_cpu (void);

#endif

/* vim: set ts=8 sw=8 noet: */
