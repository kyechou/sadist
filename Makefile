# Makefile for Cloud Computing lab 1-2
# author: kyechou

CFLAGS  += -O2 -lpthread -pthread $(shell pkg-config --cflags ncurses)
LDFLAGS += -Wl,--gc-sections -lpthread -pthread $(shell pkg-config --libs ncurses)
OBJS = sadist.o \
       cpu.o \
       mem.o \
       diskio.o

all: tags sadist

sadist: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	@./sadist -t 105481.40

clean: tags
	-@rm -rf $(OBJS) sadist

tags:
	-@ctags -R

.PHONY: all run clean tags
