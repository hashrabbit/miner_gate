CXX ?= arm-none-linux-gnueabi-g++
INSTALL ?= install
PKG_CONFIG ?= pkg-config

VPATH = .:../i2ctools

CFLAGS_base := -g -I. \
	-lpthread \
	-rdynamic \
	-DDC2DC_CHECK_ON_ERROR \
	-DMINERGATE

LDFLAGS_base := -fpermissive

CFLAGS_libsystemd = $(shell $(PKG_CONFIG) --cflags libsystemd-daemon)
LDFLAGS_libsystemd = $(shell $(PKG_CONFIG) --libs libsystemd-daemon)

CFLAGS := \
	$(CFLAGS_base) \
	$(CFLAGS_libsystemd) \
	$(CFLAGS)

LDFLAGS := \
	$(LDFLAGS_base) \
	$(LDFLAGS_libsystemd) \
	$(LDFLAGS)

TARGETS = miner_gate_sp2x miner_gate_sp3x
SOURCES = $(wildcard *.c) mainvpd_lib.cpp

all: $(TARGETS)

miner_gate_sp2x: CFLAGS += -DSP2x
$(TARGETS): $(SOURCES)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(EXTRA_FLAGS) -o $@ $^

clean:
	$(RM) -f $(TARGETS)

install: $(TARGETS)
	$(INSTALL) -D -m 755 $^ $(DESTDIR)/usr/bin/

.PHONY: all clean install
