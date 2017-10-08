# Homework 1-2-1: CPU, Mem, Disk I/O Monitoring

2017 Fall NCTU Cloud Computing Systems and Applications, lectured by Prof. Chen,
Tien-Fu.

## Description

The monitoring tool is written in C, with ncurses and pthread libraries. It
monitors the utilization of CPUs, main memory, and disks I/O. After compiling
the sources with `make`, you can launch the monitor with `make run` or simply
`./monitor`.

The monitor takes three options: `-r`, `-w`, and `-t`, which stands for the
maximum disk rate of reading, writing and total IOs, respectively. The `-h`
option can be used to show following help message:

```
Usage: ./monitor [-h] [-r max_read] [-w max_write] [-t max_total]

Options:
        -h      show this message
        -r      specify the max reading rate of the disk
        -w      specify the max writing rate of the disk
        -t      specify the total max IO rate of the disk
```

## Implementation

The source codes are pretty self-explanatory. Here is a brief introduction to
the calculations of the statistics.

### CPU Utilization

The CPU utilization is calculated from the file `/proc/stat` provided by the
kernel, i.e., the working time divided by the summation of the working time and
the idle time. The working time and the idle time is calculated from the
difference of two samples of reading `/proc/stat`. The interval between two
samplings is set to 1 second in the `cpu.h` header. Since the function that
samples the `/proc/stat` sleeps for 1 second on each call, which blocks the
whole process, so the function that samples the statistics and calculates the
CPU utilization is run in a POSIX thread.

### Memory Utilization

The calculation of memory utilization is much more complicated, but the
implementation is much simpler, since it does not need two samples of statistics.
The statistics are also read from the file `/proc/meminfo` provided by the
kernel. The amount of memory used is calculated as:

```
MemTotal - MemFree - Buffers - Cached - SReclaimable + Shmem
```

This is the formula that htop uses to compute the amount of memory used,
[stated by Hisham](https://stackoverflow.com/a/41251290/4558070), the author of
htop.

Therefore, the usage of memory is then (the amount of memory used) / (total
memory) * 100%.

### Disk IO Utilization

This is the trickiest part in my opinion. I build a linked list for disks, store
the statistics of each disk device in a node of the list, and calculate the
utilization from two samples of statistics, like the CPU utilization. The
interval of each sample is also 1 second. The function that calculates the disk
IO utilization is also run in a POSIX thread.

If one or more of the options (`-r`, `-w`, or `-t`) are given, the monitor will
show the percentage of the respective disk utilization on the screen.

My maximum disk IO rate (105481.40 kB/s) is calculated from the output of my
monitor, and averaged from the 20 highest IO rates with the command `sort -gr |
head -n20 | paste -sd+ - | sed -e 's/^/\(/' -e 's/$/\)\/20/' | bc -l`.

