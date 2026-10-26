# Build df.exe for 16-bit MS-DOS with Open Watcom and libc16.
#   make             fetch and build the pinned libc16 into build/libc16,
#                    then build df.exe
#   make LIBC16_ROOT=../libc16
#                    use a libc16 checkout built with its make
#   make test        host unit tests, then DOS tests under DOSBox-X
#
# df.exe is compiled and linked against libc16 only (its ABI is in
# libc16's docs/ABI.md), with the Open Watcom toolchain libc16 was built
# with, named in $(LIBC16_ROOT)/build/tools.env.

LIBC16_ROOT ?= build/libc16
LIBC16      := $(LIBC16_ROOT)/build/LIBC16.LIB
WATCOM_ENV   = . $(LIBC16_ROOT)/build/tools.env && PATH=$$WATCOM/binl64:$$PATH

VERSION := $(shell cat VERSION)

CC     = wcc
CFLAGS = -q -zq -zl -zld -x -ms -0 -os -s -bt=dos -wx -we \
         -I$(LIBC16_ROOT)/include '-DVERSION="$(VERSION)"'

# C99 for unsigned long long, which the 64-bit capacity arithmetic needs
HOSTCC     ?= cc
HOSTCFLAGS ?= -std=c99 -pedantic -Wall -Wextra -Werror -O2

SRCS = src/main.c src/df.c src/options.c src/format.c src/drives.c src/out.c
HDRS = src/df.h src/options.h src/format.h src/drives.h src/out.h

OBJS = $(patsubst src/%.c,build/dos/%.obj,$(SRCS))

UNIT = build/test_format build/test_options build/test_df

df.exe: $(OBJS) $(LIBC16)
	$(WATCOM_ENV) && wlink option quiet, nodefaultlibs, map=build/df.map \
	  format dos name $@ $(foreach o,$(OBJS),file $(o)) library $(LIBC16)

build/dos/%.obj: src/%.c $(HDRS) VERSION $(LIBC16)
	@mkdir -p build/dos
	$(WATCOM_ENV) && $(CC) $(CFLAGS) -fo=$@ $<

# The pinned libc16 (scripts/fetch-libc16). A LIBC16_ROOT given on the
# command line must already be built.
ifeq ($(LIBC16_ROOT),build/libc16)
$(LIBC16): scripts/fetch-libc16
	scripts/fetch-libc16 $(LIBC16_ROOT)
endif

build/test_format: tests/test_format.c tests/unit.h src/format.c src/format.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -Isrc -o $@ tests/test_format.c src/format.c

build/test_options: tests/test_options.c tests/unit.h src/options.c src/options.h \
		src/out.c src/out.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -Isrc -o $@ tests/test_options.c src/options.c \
		src/out.c

build/test_df: tests/test_df.c tests/unit.h src/df.c src/df.h \
		src/options.c src/options.h src/format.c src/format.h \
		src/out.c src/out.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -Isrc -o $@ tests/test_df.c src/df.c \
		src/options.c src/format.c src/out.c

unit: $(UNIT)
	@for t in $(UNIT); do echo "$$t"; ./$$t || exit 1; done

dostest: df.exe
	sh tests/run.sh $(CURDIR)/df.exe

test: unit dostest

# Keeps build/libc16, which takes a while to fetch.
clean:
	rm -rf *.o *.obj *.err df.exe \
	  $(filter-out build/libc16,$(wildcard build/*))

.PHONY: unit dostest test clean
