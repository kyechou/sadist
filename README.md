# Homework 1-2-1: CPU, Mem, Disk I/O Workload

2017 Fall NCTU Cloud Computing Systems and Applications, lectured by Prof. Chen,
Tien-Fu.

## Description

This workload generating tool is written in C, with ncurses and pthread
libraries. Before compiling, please make sure that `glibc` and `ncurses`
packages has been installed on your system. To compile the source, run `make` in
the command line. You can then launch the tool by `make run`, or by executing
the `./workload` command.

The workload generating tool takes seven optional arguments. The `-h` option can
be used to show the following message:

```
Usage: ./workload [-h] [-r max_read] [-w max_write] [-t max_total]
                  [-c cpu_workload] [-m mem_workload] [-d disk_IO_workload]

Options:
        -h      show this message
        -r      the max reading rate of the disk
        -w      the max writing rate of the disk
        -t      the max total IO rate of the disk
        -c      the percentage of additional workload on CPU (0-100)
        -m      the percentage of additional workload on memory (0-100)
        -d      the percentage of additional workload on disk (0-100)
```

If one or more of the options, `-r`, `-w`, or `-t`, are given, the monitor will
show the percentage of the respective disk utilization on the screen, and the
max writing rate, or the total IO rate, of the disk will be used to calculate
the amount of I/O workload the tool generates.

The `-c`, `-m`, `-d` options specify the percentage of the additional workload
this tool generates on the system, which can also be adjusted through the
interactive user interface after starting the program.

## Implementation

This is a multi-thread program, and the following is the thread map of this
program.

```
┌──────┐                                  ┌────────────┐       ┌─────────────────────┐
│ main ├─┬──────────────────────────────┬─┤ stress_cpu ├─────┬─┤ threaded_stress_cpu │
└──────┘ │                              │ └────────────┘     │ └─────────────────────┘
         │ ┌─────────┐                  │ ┌────────────┐     │       .
         └─┤ monitor │                  ├─┤ stress_mem │     │       .
           └─┬───────┘                  │ └────────────┘     │       .
             │ ┌───────────────────┐    │ ┌───────────────┐  │       .
             ├─┤ threaded_read_cpu │    └─┤ stress_diskio │  │
             │ └───────────────────┘      └───────────────┘  │ ┌─────────────────────┐
             │ ┌──────────────────────┐                      └─┤ threaded_stress_cpu │
             └─┤ threaded_read_diskio │                        └─────────────────────┘
               └──────────────────────┘
```

The `threaded_read_cpu` and the `threaded_read_diskio` threads are created on
each call, while all the other threads are persistent until the program exits.

The `main` thread is responsible for the user input and reacts correspondingly.
The `monitor` thread reads the statistics and refresh the screen display for a
certain amount of time. The `stress_*` threads account for generating the
respective workloads specified by the user. The `threaded_stress_cpu` threads,
of which the amount is equal to the number of CPUs, are workers controlled by
the master thread `stress_cpu`.

### CPU Workload Generation

The master thread `stress_cpu` first calculates the time to work and to
rest/sleep by multiplying the CPU_HOG_GRAN with the percentage of the workload,
and then sends a `SIGUSR1` signal to each of the worker threads
`threaded_stress_cpu` after they have worked for a period of time equal to the
working time. On receiving the signal, the worker threads then sleep for a
period of time that equals to (1 - percentage of workload) * CPU_HOG_GRAN.

Therefore, the worker threads work for the workload percentage of the time of
CPU_HOG_GRAN, and rest/sleep for the remaining time of the CPU_HOG_GRAN.

```
     workload(%) * CPU_HOG_GRAN           (1 - workload(%)) * CPU_HOG_GRAN
├───────────────────────────────────┼───────────────────────────────────────────┤
```

The CPU_HOG_GRAN macro is defined in the header `cpu.h`.

### Memory Workload Generation

The implementation of generating memory workload is much simpler. The
`stress_mem` thread simply dynamically allocates an area of memory and write
a byte on every page (every 4-KB memory area).

### Disk I/O Workload Generation

The `stress_diskio` thread first calculates the stressing rate by multiplying
the workload percentage with the maximum disk I/O rate, and then derives the
number of bytes it is going to write by multiplying the stress rate with
DISK_HOG_GRAN, which is defined in the header `diskio.h`. Hence, it writes the
number of bytes to a file on the disk, and sleeps through the remaining time,
i.e., DISK_HOG_GRAN - time used for writing.

