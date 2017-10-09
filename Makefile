# Makefile for Cloud Computing lab 1-2
# author: kyechou

CFLAGS  += -O2 -lncurses -lpthread -pthread
LDFLAGS += -Wl,--gc-sections -lncurses -lpthread -pthread
OBJS = workload.o \
       cpu.o \
       mem.o \
       diskio.o

all: tags workload

workload: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	@./workload -t 105481.40

clean: tags
	-@rm -rf $(OBJS) workload

tags:
	-@ctags -R

.PHONY: all run clean tags
