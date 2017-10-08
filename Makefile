# Makefile for Cloud Computing lab 1-2
# author: kyechou

CFLAGS  += -O2 -lncurses -lpthread
LDFLAGS += -Wl,--gc-sections -lncurses -lpthread
OBJS = monitor.o \
       cpu.o \
       mem.o \
       diskio.o

all: tags monitor

monitor: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	@./monitor

clean: tags
	-@rm -rf $(OBJS) monitor

tags:
	-@ctags -R

.PHONY: all run clean tags
