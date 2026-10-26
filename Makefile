# Build df.exe for 16-bit MS-DOS with Open Watcom.
#   make WATCOM=~/opt/watcom
#   make test        host unit tests, then DOS tests under DOSBox-X
#
# Optionally, build dfsc.exe with Smaller C instead (see SMALLERC.md).
#   make smallerc
#   make test-smallerc

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

# Smaller C, with an 8086 build of its library in $(SMLRC_TC)
SMLRCC   ?= smlrcc
SMLRC_TC ?= $(CURDIR)/smallerc/toolchain
SCFLAGS   = -doss -Wall -asm $(CURDIR)/smallerc/nasm8086 \
            -SI $(SMLRC_TC)/include -SI $(CURDIR)/smallerc -I src \
            -SL $(SMLRC_TC)/lib '-DVERSION="$(VERSION)"'
# smallerc/format.c replaces src/format.c; smallerc/i86.c adds int86
SCSRCS    = src/main.c src/df.c src/options.c smallerc/format.c \
            src/drives.c smallerc/i86.c
SCHDRS    = $(HDRS) smallerc/i86.h

# the same unit tests, against smallerc/format.c
SCUNIT = build/test_format_sc build/test_df_sc

smallerc: dfsc.exe

dfsc.exe: $(SCSRCS) $(SCHDRS) VERSION $(SMLRC_TC)/lib/lcds.a
	$(SMLRCC) $(SCFLAGS) -o $@ $(SCSRCS)

$(SMLRC_TC)/lib/lcds.a: smallerc/toolchain.sh smallerc/nasm8086 \
                        smallerc/8086.inc smallerc/lcds-8086.patch
	SMLRCC=$(SMLRCC) sh smallerc/toolchain.sh $(SMLRC_TC)

build/test_format_sc: tests/test_format.c tests/unit.h smallerc/format.c \
		src/format.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -DDFSIZE_WORDS -Isrc -o $@ \
		tests/test_format.c smallerc/format.c

build/test_df_sc: tests/test_df.c tests/unit.h src/df.c src/df.h \
		src/options.c src/options.h smallerc/format.c src/format.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -DDFSIZE_WORDS -Isrc -o $@ tests/test_df.c \
		src/df.c src/options.c smallerc/format.c

unit-smallerc: $(SCUNIT)
	@for t in $(SCUNIT); do echo "$$t"; ./$$t || exit 1; done

dostest-smallerc: dfsc.exe
	sh tests/run.sh $(CURDIR)/dfsc.exe

test-smallerc: unit-smallerc dostest-smallerc

# tests/rows.c from Watcom and Smaller C against the host; needs both
build/rows: tests/rows.c src/format.c src/format.h
	@mkdir -p build
	$(HOSTCC) $(HOSTCFLAGS) -Isrc -o $@ tests/rows.c src/format.c

build/rowswc.exe: tests/rows.c src/format.c src/format.h
	@mkdir -p build
	$(CC) -q -bt=dos -ms -0 -os -s -wx -we -zq -Isrc -fe=$@ \
		tests/rows.c src/format.c

build/rowssc.exe: tests/rows.c smallerc/format.c src/format.h \
		$(SMLRC_TC)/lib/lcds.a
	@mkdir -p build
	$(SMLRCC) $(SCFLAGS) -o $@ tests/rows.c smallerc/format.c

test-rows: build/rows build/rowswc.exe build/rowssc.exe
	sh tests/rows.sh build/rows build/rowswc.exe build/rowssc.exe

clean:
	rm -rf *.o *.obj *.err df.exe dfsc.exe build

.PHONY: unit dostest test smallerc unit-smallerc dostest-smallerc \
	test-smallerc test-rows clean
