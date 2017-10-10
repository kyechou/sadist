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

