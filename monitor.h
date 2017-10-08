#ifndef __MONITOR_H
#define __MONITOR_H

#include "cpu.h"
#include "mem.h"
#include "diskio.h"

#define INTERVAL 1200000	/* refresh screen every 1.2 sec */

void error (const char *);	/* report error and quit */

#endif

/* vim: set ts=8 sw=8 noet: */
