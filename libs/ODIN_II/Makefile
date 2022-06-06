
CONFIG := clang-debug
# CONFIG := gcc-debug
# CONFIG := profile
# CONFIG := release

CC = clang
CXX = clang
CXXFLAGS = -MD -Wall -Wextra -ggdb -pthread
LDLIBS = -lstdc++ -lpthread

library-objects = \
	adders.o \
	ast_elaborate.o \
	ast_loop_unroll.o \
	ast_util.o \
	BLIF.o \
	BLIFElaborate.o \
	BLIFReader.o \
	BLIFWriter.o \
	BlockMemories.o \
	CaseEqual.o \
	Division.o \
	enum_str.o \
	FlipFlop.o \
	GenericIO.o \
	GenericReader.o \
	GenericWriter.o \
	hard_blocks.o \
	HardSoftLogicMixer.o \
	Hashtable.o \
	hierarchy_util.o \
	implicit_memory.o \
	Latch.o \
	LogicalOps.o \
	main.o \
	memories.o \
	MixingOptimization.o \
	Modulo.o \
	Multiplexer.o \
	multipliers.o \
	netlist_check.o \
	netlist_cleanup.o \
	netlist_create_from_ast.o \
	netlist_statistic.o \
	netlist_utils.o \
	netlist_visualizer.o \
	node_creation_library.o \
	odin_error.o \
	odin_ii.o \
	odin_memory.o \
	odin_util.o \
	parse_making_ast.o \
	partial_map.o \
	Power.o \
	read_xml_config_file.o \
	scope_util.o \
	Shift.o \
	simulate_blif.o \
	string_cache.o \
	subtractions.o \
	Verilog.o \
	VerilogReader.o \
	VerilogWriter.o \

library-headers = \
	adders.h \
	ast_elaborate.h \
	ast_loop_unroll.h \
	ast_util.h \
	AtomicBuffer.hpp \
	BLIF.hpp \
	BLIFElaborate.hpp \
	BlockMemories.hpp \
	CaseEqual.hpp \
	config_t.h \
	Division.hpp \
	FlipFlop.hpp \
	GenericIO.hpp \
	GenericReader.hpp \
	GenericWriter.hpp \
	hard_blocks.h \
	HardSoftLogicMixer.hpp \
	Hashtable.hpp \
	hierarchy_util.h \
	implicit_memory.h \
	Latch.hpp \
	LogicalOps.hpp \
	memories.h \
	MixingOptimization.hpp \
	Modulo.hpp \
	Multiplexer.hpp \
	multipliers.h \
	netlist_check.h \
	netlist_cleanup.h \
	netlist_create_from_ast.h \
	netlist_statistic.h \
	netlist_utils.h \
	netlist_visualizer.h \
	node_creation_library.h \
	odin_buffer.hpp \
	odin_error.h \
	odin_globals.h \
	odin_ii.h \
	odin_memory.hpp \
	odin_types.h \
	odin_util.h \
	parse_making_ast.h \
	partial_map.h \
	Power.hpp \
	read_xml_config_file.h \
	scope_util.h \
	Shift.hpp \
	sim_block.h \
	simulate_blif.h \
	string_cache.h \
	subtractions.h \
	verilog_bison_user_defined.h \
	Verilog.hpp \


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

