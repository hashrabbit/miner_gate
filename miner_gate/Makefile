# Makefile to build monte_pi_sprng program
# --- macros
CC=arm-none-linux-gnueabi-g++
#MUST OPTIMISE -O3 FOR 1.4!
CFLAGS= -O0 -g -rdynamic -fpermissive -lpthread -static-libstdc++ -static-libgcc -DDC2DC_CHECK_ON_ERROR -DMINERGATE
LIBS = -L./ -lpthread
SRCS = $(wildcard *.c)
INCLUDES = -I./
FILE_DEPS = $(wildcard *.[hc])
TARGET = miner_gate_arm

#ifeq ($(shell whoami),zvisha)
#	CFLAGS += -D ZVISHA
#endif


.PHONY: all clean depend

# --- targets
all: $(TARGET)

$(TARGET): $(FILE_DEPS) Makefile
	$(CC) -o $(TARGET) $(SRCS) $(CFLAGS) $(INCLUDES)

# --- remove binary and executable files
clean:
	rm -f $(TARGET)

depend: $(SRCS)
	makedepend $(INCLUDES) --$(CFLAGS) --$(SRCS) $^
