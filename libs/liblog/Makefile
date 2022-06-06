
CONFIG := clang-debug
# CONFIG := gcc-debug
# CONFIG := profile
# CONFIG := release

CC = clang
CXX = clang
CXXFLAGS = -MD -Wall -Wextra -ggdb -fPIE
LDLIBS = -lstdc++

ifeq ($(CONFIG),clang-debug)
CXXFLAGS += -std=c++11 -O0
endif

ifeq ($(CONFIG),gcc-debug)
CC = gcc
CXX = gcc
CXXFLAGS += -std=gnu++0x -O0
endif

ifeq ($(CONFIG),profile)
CC = gcc
CXX = gcc
CXXFLAGS += -std=gnu++0x -Os -DNDEBUG
endif

ifeq ($(CONFIG),release)
CC = gcc
CXX = gcc
CXXFLAGS += -std=gnu++0x -march=native -O3 -DNDEBUG
endif

all: log.o

clean:
	rm -f *.o *.d

.PHONY: all test clean

-include *.d

