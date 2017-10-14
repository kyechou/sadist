#ifndef __SADIST_H
#define __SADIST_H

#include "cpu.h"
#include "mem.h"
#include "diskio.h"

#define INTERVAL 1200000	/* refresh screen every 1.2 sec */

/* mode constants */
#define M_NORMAL	(-1)
#define M_CPU		0
#define M_MEM		1
#define M_DISKIO	2
#define NR_MODE		3

extern double workload[NR_MODE];
extern double maxrkbps;
extern double maxwkbps;
extern double maxtotalkbps;

void error (const char *);	/* report error and quit */

#endif

/* vim: set ts=8 sw=8 noet: */
