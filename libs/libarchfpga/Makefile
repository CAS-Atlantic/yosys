
CONFIG := clang-debug
# CONFIG := gcc-debug
# CONFIG := profile
# CONFIG := release

CC = clang
CXX = clang
CXXFLAGS = -MD -Wall -Wextra -ggdb
LDLIBS = -lstdc++

library-objects = \
	arch_check.o \
	arch_error.o \
	arch_util.o \
	echo_arch.o \
	parse_switchblocks.o \
	physical_types_util.o \
	physical_types.o \
	read_xml_arch_file.o \
	read_xml_util.o \
#	read_fpga_interchange_arch.o \

library-headers = \
	arch_check.h \
	arch_error.h \
	arch_types.h \
	arch_util.h \
	cad_types.h \
	clock_types.h \
	echo_arch.h \
	logic_types.h \
	parse_switchblocks.h \
	physical_types_util.h \
	physical_types.h \
	read_xml_arch_file.h \
	read_xml_util.h \
#	read_fpga_interchange_arch.h \


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

