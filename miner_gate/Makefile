CXX ?= arm-none-linux-gnueabi-g++
INSTALL ?= install
PKG_CONFIG ?= pkg-config

VPATH = .:../i2ctools

CFLAGS_base := -g -I. \
	$(shell $(PKG_CONFIG) --cflags libsystemd) \
	-lpthread \
	-rdynamic \
	-DDC2DC_CHECK_ON_ERROR

LDFLAGS_base := \
	$(shell $(PKG_CONFIG) --libs libsystemd) \
	-fpermissive

CFLAGS := \
	$(CFLAGS_base) \
	$(CFLAGS)

LDFLAGS := \
	$(LDFLAGS_base) \
	$(LDFLAGS)

CFLAGS_miner_gate := \
	$(CFLAGS) \
	-DMINERGATE

LDFLAGS_miner_gate := \
	$(LDFLAGS)

TARGETS_miner_gate = miner_gate_sp2x miner_gate_sp3x
SOURCES_miner_gate = $(wildcard *.c) mainvpd_lib.cpp

CFLAGS_i2ctools := \
	$(CFLAGS)

LDFLAGS_i2ctools := \
	$(LDFLAGS)

TARGETS_i2ctools = ac2dcvpd
SOURCES_i2ctools = ac2dcvpd.c \
	ac2dc.c \
	i2c.c \
	spond_debug.c \
	squid.c

TARGETS = \
	$(TARGETS_miner_gate) \
	$(TARGETS_i2ctools)

all: $(TARGETS)

miner_gate_sp2x: CFLAGS_miner_gate += -DSP2x
$(TARGETS_miner_gate): $(SOURCES_miner_gate)
	$(CXX) $(CFLAGS_miner_gate) $(LDFLAGS_miner_gate) $(EXTRA_FLAGS) -o $@ $^

$(TARGETS_i2ctools): $(SOURCES_i2ctools)
	$(CXX) $(CFLAGS_i2ctools) $(LDFLAGS_i2ctools) $(EXTRA_FLAGS) -o $@ $^

clean:
	$(RM) -f $(TARGETS)

install: $(TARGETS)
	$(INSTALL) -D -m 755 $^ $(DESTDIR)/usr/bin/

.PHONY: all clean install
