# Build df.exe for 16-bit MS-DOS with Open Watcom.
#   make WATCOM=~/opt/watcom
#   make test        host unit tests, then DOS tests under DOSBox-X

WATCOM ?= $(HOME)/opt/watcom
export WATCOM
export INCLUDE := $(WATCOM)/h
export PATH := $(WATCOM)/binl64:$(WATCOM)/binl:$(PATH)

VERSION := $(shell cat VERSION)

CC     = wcl
CFLAGS = -q -bt=dos -ms -0 -os -s -wx -we -zq '-DVERSION="$(VERSION)"'

# C99 for unsigned long long, which the 64-bit capacity arithmetic needs
HOSTCC     ?= cc
HOSTCFLAGS ?= -std=c99 -pedantic -Wall -Wextra -Werror -O2

SRCS = src/main.c src/df.c src/options.c src/format.c src/drives.c
HDRS = src/df.h src/options.h src/format.h src/drives.h

UNIT = build/test_format build/test_options build/test_df

df.exe: $(SRCS) $(HDRS) VERSION
	$(CC) $(CFLAGS) -fe=$@ $(SRCS)

build/test_format: tests/test_format.c tests/unit.h src/format.c src/format.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -Isrc -o $@ tests/test_format.c src/format.c

build/test_options: tests/test_options.c tests/unit.h src/options.c src/options.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -Isrc -o $@ tests/test_options.c src/options.c

build/test_df: tests/test_df.c tests/unit.h src/df.c src/df.h \
		src/options.c src/options.h src/format.c src/format.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -Isrc -o $@ tests/test_df.c src/df.c \
		src/options.c src/format.c

unit: $(UNIT)
	@for t in $(UNIT); do echo "$$t"; ./$$t || exit 1; done

dostest: df.exe
	sh tests/run.sh $(CURDIR)/df.exe

test: unit dostest

clean:
	rm -rf *.o *.obj *.err df.exe build

.PHONY: unit dostest test clean
