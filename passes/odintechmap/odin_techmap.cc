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

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct OdinTechmapPass : public Pass {
	OdinTechmapPass() : Pass("odintechmap", "ODIN_II partial technology mapper") { }
	void help() override
	{
		log("\n");
		log("    -arch <filename>\n");
		log("        to read fpga architecture file\n");

		log("\n");
		log("    -info\n");
		log("        shows available hardblocks inside the architecture file\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
		bool flag_arch_file = false;
		bool flag_arch_info = false;
		std::string arch_file_path;
		ODIN::ArchFpga arch;

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
		}
		extra_args(args, argidx, design);

		if (flag_arch_file)
			arch.read_arch_file(arch_file_path);

		if (flag_arch_info)
			arch.log_arch_info();

		log("odintechmap pass finished.\n");
	}
} OdinTechmapPass;

PRIVATE_NAMESPACE_END