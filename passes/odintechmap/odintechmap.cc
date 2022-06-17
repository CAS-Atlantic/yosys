#include "kernel/yosys.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct OdintechmapPass : public Pass {
	OdintechmapPass() : Pass("odintechmap", "ODIN Tech Map for Yosys :)") { }
	void help() override
	{
		log("Odintechmap help will be written here\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
		log_header(design, "Starting odintechmap pass.\n");

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{

			if (args[argidx] == "-a" && argidx+1 < args.size()) {
				
				continue;
			}

		}
		extra_args(args, argidx, design);


		log("odintechmap pass finished.\n");
	}
} OdintechmapPass;

PRIVATE_NAMESPACE_END