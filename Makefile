PREFIX     ?= /usr
INSTALLDIR := $(DESTDIR)$(PREFIX)

CC	:= gcc

PKGS := allegro-5.0 allegro_image-5.0 jansson
INCS := $(shell pkg-config --cflags $(PKGS))
LIBS := $(shell pkg-config --libs $(PKGS)) -lchipmunk -lm

CFLAGS   := -Wall -Wextra -std=gnu99 -I. $(INCS) $(CFLAGS) $(DEBUG)
CPPFLAGS := $(CPPFLAGS)
DEBUG    := -g -DDEBUG
LDFLAGS  := $(LIBS) $(LDFLAGS)

SRCS  := $(wildcard *.c)
HEADS := $(wildcard *.h)
OBJS  := $(SRCS:.c=.o)


all: discontent-snails

debug: CFLAGS += $(DEBUG)
debug: all

.c.o:
	@echo $(CC) -c $< -o $@
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

discontent-snails: $(OBJS)
	@echo $(CC) -o $@ $(OBJS)
	@$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -rf discontent-snails $(OBJS)
