#include "kernel/yosys.h"

#include "libs/ODIN_II/odin_ii.h"
#include "libs/ODIN_II/odin_types.h"

#include "libs/libvtrutil/vtr_util.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct OdinPass : public Pass {
	OdinPass() : Pass("odin", "ODIN_II Partial Mapper for Yosys :)") { }
	void help() override
	{
		log("ODIN_II help will be shown here\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
		log_header(design, "Executing ODIN_II pass.\n");

		/* pass arguments here to odin */
    	std::vector<std::string> arguments = 
		{
			"libs/ODIN_II/odin_ii", 							//	pass the odin location
        	// "-a", "4bit_adder_double_chain_arch.xml",		//	pass the arch file
			// "-b", "simple.blif",								//	pass the input blif file
			// "-o", "mapped.blif",								//	pass the output blif file
			"--coarsen"
    	};

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{

			if (args[argidx] == "-a" && argidx+1 < args.size()) {
				arguments.push_back("-a");
				arguments.push_back(args[++argidx]);
				continue;
			}

			if (args[argidx] == "-b" && argidx+1 < args.size()) {
				arguments.push_back("-b");
				arguments.push_back(args[++argidx]);
				continue;
			}

			if (args[argidx] == "-o" && argidx+1 < args.size()) {
				arguments.push_back("-o");
				arguments.push_back(args[++argidx]);
				continue;
			}

		}
		extra_args(args, argidx, design);

		bool did_something = true;

		while (did_something)
		{
			did_something = false;

		}

		char **arg_list;
    	int arg_len;

    	/* converting into format digestible by odin */
	    arg_list = (char**)malloc(arguments.size()*sizeof(char*));
		arg_len = arguments.size();

		for( int i=0; i<arg_len; i ++)
	    {
    	    arg_list[i] = vtr::strdup(arguments[i].c_str());
    	}

		netlist_t* odin_netlist = start_odin_ii(arg_len, arg_list);
    	terminate_odin_ii(odin_netlist);

		log("ODIN_II pass finished.\n");
	}
} OdinPass;

PRIVATE_NAMESPACE_END