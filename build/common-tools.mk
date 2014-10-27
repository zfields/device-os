
RM = rm -f
RMDIR = rm -f -r
MKDIR = mkdir -p

CC = $(GCC_PREFIX)gcc
CPP = $(GCC_PREFIX)g++
AR = $(GCC_PREFIX)ar
OBJCOPY = $(GCC_PREFIX)objcopy
OBJDUMP = $(GCC_PREFIX)objdump
SIZE = $(GCC_PREFIX)size
NM = $(GCC_PREFIX)nm
AWK = awk
DFU = dfu-util
CURL = curl

# ensure there are no multiply-defined symbols
CFLAGS += -fno-common
CPPFLAGS += -std=gnu++11

