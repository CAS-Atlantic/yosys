
CONFIG := clang-debug
# CONFIG := gcc-debug
# CONFIG := profile
# CONFIG := release

CC = clang
CXX = clang
CXXFLAGS = -MD -Wall -Wextra -ggdb -fPIE
LDLIBS = -lstdc++

library-objects = \
	pugixml.o \

library-headers = \
	pugixml.hpp \
	pugiconfig.hpp \

# To ``make the library'', make all its objects using the implicit rule.
library: $(library-objects)

# Conservatively assume that all the objects depend on all the headers.
$(library-objects): $(library-headers)

# Components of the program.
program = sample
program-objects = sample.o

# Conservatively assume all the program source files depend on all the library
# headers.  You can change this if it is not the case.
$(program-objects) : $(library-headers)

# How to link the program.  The implicit rule covers individual objects.
$(program) : $(program-objects) $(library-objects)
	g++ $^ -o $@

all: library $(program)

clean:
	rm -f *.o *.d

-include *.d

