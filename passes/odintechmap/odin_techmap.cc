/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2022  Daniel Khadivi <dani-kh@live.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/yosys.h"

#include "arch_fpga.h"
#include "partial_map.h"
#include "adders.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct OdinTechmapPass : public Pass {
	OdinTechmapPass() : Pass("odintechmap", "ODIN_II partial technology mapper") { }
	void help() override
	{
		log("\n");
		log("This pass calls simplemap pass by default.\n");
		log("\n");
		log("    -arch <filename>\n");
		log("        to read fpga architecture file\n");
		log("\n");
		log("    -info\n");
		log("        shows available hardblocks inside the architecture file\n");
		log("\n");
		log("    -nosimplemap\n");
		log("        skips simplemap pass, and therefore the following internal cell types are not mapped.\n");
		log("            $not, $pos, $and, $or, $xor, $xnor\n");
		log("            $reduce_and, $reduce_or, $reduce_xor, $reduce_xnor, $reduce_bool\n");
		log("            $logic_not, $logic_and, $logic_or, $mux, $tribuf\n");
		log("            $sr, $ff, $dff, $dffe, $dffsr, $dffsre, $adff, $adffe, $aldff, $aldffe, $sdff, $sdffe, $sdffce, $dlatch, $adlatch, $dlatchsr\n");
		log("\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
		bool flag_arch_file = false;
		bool flag_arch_info = false;
		bool flag_simple_map = true;
		std::string arch_file_path;
		ODIN::ArchFpga arch;

		ODIN::PartialMap mapper(&arch);
		

		log_header(design, "Starting odintechmap pass.\n");

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			if (args[argidx] == "-arch" && argidx+1 < args.size()) {
				arch_file_path = args[++argidx];
				flag_arch_file = true;
				continue;
			}
			if (args[argidx] == "-info") {
				flag_arch_info = true;
				continue;
			}
			if (args[argidx] == "-nosimplemap") {
				flag_simple_map = true;
				continue;
			}
		}
		extra_args(args, argidx, design);

log("map conditions *: %i\n", arch.min_threshold_adder());
		arch.set_default_config();
log("map conditions *: %i\n", arch.min_threshold_adder());

		/* read the confirguration file .. get options presets the config values just in case theyr'e not read in with config file */
		if (flag_arch_file)
			arch.read_arch_file(arch_file_path);
log("map conditions *: %i\n", arch.min_threshold_adder());

		if (flag_arch_info)
			arch.log_arch_info();

		mapper.initial_hard_blocks();

		/* Performing netlist optimizations */
		mapper.optimization(design);

		/* Performaing partial tech. map to the target device */
		log("map conditions *: %i\n", arch.min_threshold_adder());
		mapper.start_map(design);

		if (flag_simple_map)
			Pass::call(design, "simplemap");
		

		log("odintechmap pass finished.\n");
	}
} OdinTechmapPass;

PRIVATE_NAMESPACE_END