# Makefile for Cloud Computing lab 1-2
# author: kyechou

CFLAGS  += -O2 -lncurses -lpthread -pthread
LDFLAGS += -Wl,--gc-sections -lncurses -lpthread -pthread
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
