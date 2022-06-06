
CONFIG := clang-debug
# CONFIG := gcc-debug
# CONFIG := profile
# CONFIG := release

CC = clang
CXX = clang
CXXFLAGS = -MD -Wall -Wextra -ggdb -fPIE 
LDLIBS = -lstdc++

library-objects = \
	vtr_assert.o \
	vtr_color_map.o \
	vtr_digest.o \
	vtr_expr_eval.o \
	vtr_geometry.o \
	vtr_list.o \
	vtr_log.o \
	vtr_math.o \
	vtr_memory.o \
	vtr_path.o \
	vtr_random.o \
	vtr_rusage.o \
	vtr_time.o \
	vtr_token.o \
	vtr_util.o \


library-headers = \
	breakpoint_state_globals.h \
	picosha2.h \
	vtr_array_view.h \
	vtr_assert.h \
	vtr_bimap.h \
	vtr_cache.h \
	vtr_color_map.h \
	vtr_digest.h \
	vtr_dynamic_bitset.h \
	vtr_error.h \
	vtr_expr_eval.h \
	vtr_flat_map.h \
	vtr_geometry.h \
	vtr_hash.h \
	vtr_linear_map.h \
	vtr_list.h \
	vtr_log.h \
	vtr_logic.h \
	vtr_map_util.h \
	vtr_math.h \
	vtr_memory.h \
	vtr_ndmatrix.h \
	vtr_ndoffsetmatrix.h \
	vtr_ostream_guard.h \
	vtr_pair_util.h \
	vtr_path.h \
	vtr_ragged_matrix.h \
	vtr_random.h \
	vtr_range.h \
	vtr_rusage.h \
	vtr_sentinels.h \
	vtr_small_vector.h \
	vtr_string_interning.h \
	vtr_string_view.h \
	vtr_strong_id_range.h \
	vtr_strong_id.h \
	vtr_time.h \
	vtr_token.h \
	vtr_util.h \
	vtr_vec_id_set.h \
	vtr_vector_map.h \
	vtr_vector.h \
	vtr_version.h \

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

