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
#include "kernel/celltypes.h"

#include <regex>

// #include "arch_fpga.h"
// #include "partial_map.h"
// #include "adders.h"
#include "odin_ii.h"
#include "odin_util.h"
#include "odin_globals.h"
#include "netlist_utils.h"

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_path.h"

#include "netlist_check.h"

#include "partial_map.h"

#include "GenericWriter.hpp"
#include "GenericReader.hpp"

#include "netlist_visualizer.h"

#include "BLIFElaborate.hpp"

#include "adders.h"
#include "hard_blocks.h"
#include "multipliers.h"
#include "memories.h"
#include "BlockMemories.hpp"
#include "subtractions.h"
#include "netlist_cleanup.h"
#include "netlist_statistic.h"
#include "arch_util.h"
#include "read_xml_config_file.h"

#include "Verilog.hpp"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct OdinoPass : public Pass {

	const std::string str(RTLIL::IdString id)
	{
		std::string str = RTLIL::unescape_id(id);
		for (size_t i = 0; i < str.size(); i++)
			if (str[i] == '#' || str[i] == '=' || str[i] == '<' || str[i] == '>')
				str[i] = '?';
		return str;
	}

	const std::string str(RTLIL::SigBit sig)
	{
		// if (sig.wire == NULL) {
		// 	if (sig == RTLIL::State::S0) return config->false_type == "-" || config->false_type == "+" ? config->false_out.c_str() : "$false";
		// 	if (sig == RTLIL::State::S1) return config->true_type == "-" || config->true_type == "+" ? config->true_out.c_str() : "$true";
		// 	return config->undef_type == "-" || config->undef_type == "+" ? config->undef_out.c_str() : "$undef";
		// }

		std::string str = RTLIL::unescape_id(sig.wire->name);
		for (size_t i = 0; i < str.size(); i++)
			if (str[i] == '#' || str[i] == '=' || str[i] == '<' || str[i] == '>')
				str[i] = '?';

		if (sig.wire->width != 1)
			str += stringf("[%d]", sig.wire->upto ? sig.wire->start_offset+sig.wire->width-sig.offset-1 : sig.wire->start_offset+sig.offset);

		return str;
	}

	void hook_up_nets(netlist_t* odin_netlist, Hashtable* output_nets_hash, dict<std::string, std::string> &conns) {
    	nnode_t** node_sets[] = {odin_netlist->internal_nodes, odin_netlist->ff_nodes, odin_netlist->top_output_nodes};
    	int counts[] = {odin_netlist->num_internal_nodes, odin_netlist->num_ff_nodes, odin_netlist->num_top_output_nodes};
    	int num_sets = 3;

    	/* hook all the input pins in all the internal nodes to the net */
    	int i;
    	for (i = 0; i < num_sets; i++) {
        	int j;
	        for (j = 0; j < counts[i]; j++) {
    	        nnode_t* node = node_sets[i][j];
        	    hook_up_node(node, output_nets_hash, conns);
        	}
    	}
	}

	void hook_up_node(nnode_t* node, Hashtable* output_nets_hash, dict<std::string, std::string> &conns) {
	    int j;
    	for (j = 0; j < node->num_input_pins; j++) {
        	npin_t* input_pin = node->input_pins[j];

	        nnet_t* output_net = (nnet_t*)output_nets_hash->get(input_pin->name);

			if (!output_net){
				log("trying connections...\n");
				output_net = (nnet_t*)output_nets_hash->get(conns[input_pin->name]);
			}
				
    	    if (!output_net)
        	    error_message(PARSE_BLIF, my_location, "Error: Could not hook up the pin %s: not available, related node: %s.", input_pin->name, node->name);
	        add_fanout_pin_to_net(output_net, input_pin);
    	}
	}

	void build_top_output_node(const char* name_str, netlist_t* odin_netlist, Hashtable* output_nets_hash) {
		// char* temp_string = resolve_signal_name_based_on_blif_type(blif_netlist->identifier, ptr);

        /*add_a_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);*/

        /* create a new top output node and */
        nnode_t* new_node = allocate_nnode(my_location);
        new_node->related_ast_node = NULL;
        new_node->type = OUTPUT_NODE;

        /* add the name of the output variable */
        new_node->name = vtr::strdup(name_str);

        /* allocate the input pin needed */
        allocate_more_input_pins(new_node, 1);
        add_input_port_information(new_node, 1);

        /* Create the pin connection for the net */
        npin_t* new_pin = allocate_npin();
        new_pin->name = vtr::strdup(name_str);
        /* hookup the pin, net, and node */
        add_input_pin_to_node(new_node, new_pin, 0);

        /*adding the node to the blif_netlist output nodes
         * add_node_to_netlist() function can also be used */
        odin_netlist->top_output_nodes = (nnode_t**)vtr::realloc(odin_netlist->top_output_nodes, sizeof(nnode_t*) * (odin_netlist->num_top_output_nodes + 1));
        odin_netlist->top_output_nodes[odin_netlist->num_top_output_nodes++] = new_node;
        // vtr::free(temp_string);
	}

	void build_top_input_node(const char* name_str, netlist_t* odin_netlist, Hashtable* output_nets_hash) {
    	// char* temp_string = resolve_signal_name_based_on_blif_type(blif_netlist->identifier, name_str);

    	/* create a new top input node and net*/

		loc_t my_loc;
    	nnode_t* new_node = allocate_nnode(my_loc);

    	new_node->related_ast_node = NULL;
    	new_node->type = INPUT_NODE;

    	/* add the name of the input variable */
    	new_node->name = vtr::strdup(name_str);

    	/* allocate the pins needed */
    	allocate_more_output_pins(new_node, 1);
    	add_output_port_information(new_node, 1);

    	/* Create the pin connection for the net */
    	npin_t* new_pin = allocate_npin();
    	new_pin->name = vtr::strdup(name_str);
    	new_pin->type = OUTPUT;

    	/* hookup the pin, net, and node */
    	add_output_pin_to_node(new_node, new_pin, 0);

    	nnet_t* new_net = allocate_nnet();
    	new_net->name = vtr::strdup(name_str);

    	add_driver_pin_to_net(new_net, new_pin);

    	odin_netlist->top_input_nodes = (nnode_t**)vtr::realloc(odin_netlist->top_input_nodes, sizeof(nnode_t*) * (odin_netlist->num_top_input_nodes + 1));
    	odin_netlist->top_input_nodes[odin_netlist->num_top_input_nodes++] = new_node;

    	//long sc_spot = sc_add_string(output_nets_sc, temp_string);
    	//if (output_nets_sc->data[sc_spot])
    	//warning_message(NETLIST,linenum,-1, "Net (%s) with the same name already created\n",temp_string);

    	//output_nets_sc->data[sc_spot] = new_net;

    	output_nets_hash->add(name_str, new_net);
    	// vtr::free(temp_string);
	}

	void create_top_driver_nets(netlist_t* odin_netlist, Hashtable* output_nets_hash) {
		npin_t* new_pin;
    	/* create the constant nets */

    	/* ZERO net */
    	/* description given for the zero net is same for other two */
    	odin_netlist->zero_net = allocate_nnet();                  // allocate memory to net pointer
    	odin_netlist->gnd_node = allocate_nnode(unknown_location); // allocate memory to node pointer
    	odin_netlist->gnd_node->type = GND_NODE;                   // mark the type
    	allocate_more_output_pins(odin_netlist->gnd_node, 1);      // alloacate 1 output pin pointer to this node
    	add_output_port_information(odin_netlist->gnd_node, 1);    // add port info. this port has 1 pin ,till now number of port for this is one
    	new_pin = allocate_npin();
    	add_output_pin_to_node(odin_netlist->gnd_node, new_pin, 0); // add this pin to output pin pointer array of this node
    	add_driver_pin_to_net(odin_netlist->zero_net, new_pin);     // add this pin to net as driver pin

    	/*ONE net*/
	    odin_netlist->one_net = allocate_nnet();
    	odin_netlist->vcc_node = allocate_nnode(unknown_location);
	    odin_netlist->vcc_node->type = VCC_NODE;
    	allocate_more_output_pins(odin_netlist->vcc_node, 1);
	    add_output_port_information(odin_netlist->vcc_node, 1);
    	new_pin = allocate_npin();
	    add_output_pin_to_node(odin_netlist->vcc_node, new_pin, 0);
    	add_driver_pin_to_net(odin_netlist->one_net, new_pin);

    	/* Pad net */
	    odin_netlist->pad_net = allocate_nnet();
    	odin_netlist->pad_node = allocate_nnode(unknown_location);
	    odin_netlist->pad_node->type = PAD_NODE;
    	allocate_more_output_pins(odin_netlist->pad_node, 1);
	    add_output_port_information(odin_netlist->pad_node, 1);
    	new_pin = allocate_npin();
	    add_output_pin_to_node(odin_netlist->pad_node, new_pin, 0);
    	add_driver_pin_to_net(odin_netlist->pad_net, new_pin);

	    /* CREATE the driver for the ZERO */
    	odin_netlist->zero_net->name = make_full_ref_name(odin_netlist->identifier, NULL, NULL, zero_string, -1);
	    output_nets_hash->add("gnd", odin_netlist->zero_net);

    	/* CREATE the driver for the ONE and store twice */
	    odin_netlist->one_net->name = make_full_ref_name(odin_netlist->identifier, NULL, NULL, one_string, -1);
    	output_nets_hash->add("vcc", odin_netlist->one_net);

	    /* CREATE the driver for the PAD */
    	odin_netlist->pad_net->name = make_full_ref_name(odin_netlist->identifier, NULL, NULL, pad_string, -1);
	    output_nets_hash->add("unconn", odin_netlist->pad_net);

    	odin_netlist->vcc_node->name = vtr::strdup("vcc");
	    odin_netlist->gnd_node->name = vtr::strdup("gnd");
    	odin_netlist->pad_node->name = vtr::strdup("unconn");
	}

	int infer_wire_index(RTLIL::SigBit sig) {
		if (sig.wire != NULL) {
			return sig.wire->upto ? sig.wire->start_offset+sig.wire->width-sig.offset-1 : sig.wire->start_offset+sig.offset;
		}

		return -1;
	}

	int infer_wire_index_ref(RTLIL::SigBit sig, IdString first, RTLIL::Design* design, RTLIL::Cell* cell, int index) {

		Module *m = design->module(cell->type);
		Wire *w = m ? m->wire(first) : nullptr;

		if (w == nullptr) {
			return index;
		} else {
				SigBit sig(w, index);
				return sig.wire->upto ?
						sig.wire->start_offset+sig.wire->width-sig.offset-1 :
						sig.wire->start_offset+sig.offset;
		}
	}

	std::string infer_wire_name(RTLIL::SigBit sig) {
		if (sig.wire == NULL) {
			if (sig == RTLIL::State::S0)
				return "gnd";
			else if (sig == RTLIL::State::S1)
				return "vcc";
			else
				return "unconn";
		} else {
			return RTLIL::unescape_id(sig.wire->name);
		}
	}

	char* sig_full_ref_name_sig(RTLIL::SigBit sig, char *identifier, pool<SigBit> &cstr_bits_seen) {

		cstr_bits_seen.insert(sig);

		if (sig.wire == NULL) {
			if (sig == RTLIL::State::S0)
				return "gnd";
			else if (sig == RTLIL::State::S1)
				return "vcc";
			else
				return "unconn";
		} else {
			std::string str =  RTLIL::unescape_id(sig.wire->name);
			if (sig.wire->width == 1)
				return make_full_ref_name(identifier, NULL, NULL, str.c_str(), -1);
			else {
				int idx = sig.wire->upto ? sig.wire->start_offset+sig.wire->width-sig.offset-1 : sig.wire->start_offset+sig.offset;
				return make_full_ref_name(identifier, NULL, NULL, str.c_str(), idx);
			}

		}
	}

	void map_input_port(SigSpec in_port, nnode_t *node, char *identifier, pool<SigBit> &cstr_bits_seen) {

		int base_pin_idx = node->num_input_pins;

        allocate_more_input_pins(node, in_port.size()); // port_B.size() + all other input width ?
		add_input_port_information(node, in_port.size());
				
		for (int i=0 ; i<in_port.size() ; i++) {

			char* in_pin_name = sig_full_ref_name_sig(in_port[i], identifier, cstr_bits_seen);

			npin_t* in_pin = allocate_npin();
			in_pin->name = vtr::strdup(in_pin_name);
	        add_input_pin_to_node(node, in_pin, base_pin_idx + i);
		}
	}

	void map_input_port(RTLIL::IdString mapping, SigSpec in_port, nnode_t *node, char *identifier, pool<SigBit> &cstr_bits_seen) {

		int base_pin_idx = node->num_input_pins;

        allocate_more_input_pins(node, in_port.size()); // port_B.size() + all other input width ?
		add_input_port_information(node, in_port.size());
				
		for (int i=0 ; i<in_port.size() ; i++) {

			char* in_pin_name = sig_full_ref_name_sig(in_port[i], identifier, cstr_bits_seen);

			npin_t* in_pin = allocate_npin();
			in_pin->name = vtr::strdup(in_pin_name);
			in_pin->mapping = vtr::strdup(RTLIL::unescape_id(mapping).c_str());
	        add_input_pin_to_node(node, in_pin, base_pin_idx + i);
		}
	}

	void map_output_port(SigSpec out_port, nnode_t *node, char *identifier, Hashtable* output_nets_hash, pool<SigBit> &cstr_bits_seen) {
		
		int base_pin_idx = node->num_output_pins;

        allocate_more_output_pins(node, out_port.size()); //?
        add_output_port_information(node, out_port.size());

        /*add name information and a net(driver) for the output */

		for (int i=0 ; i<out_port.size() ; i++) {
			npin_t* out_pin = allocate_npin();
			out_pin->name = NULL;//vtr::strdup(str(s).c_str());
            add_output_pin_to_node(node, out_pin, base_pin_idx + i); // if more than one output then i + something
        			
			char* output_pin_name = sig_full_ref_name_sig(out_port[i], identifier, cstr_bits_seen);
			nnet_t* out_net = (nnet_t*)output_nets_hash->get(output_pin_name);
        	if (out_net == nullptr) {
            	out_net = allocate_nnet();
        		out_net->name = vtr::strdup(output_pin_name);
				output_nets_hash->add(output_pin_name, out_net);
			}
        	add_driver_pin_to_net(out_net, out_pin);

			// CLEAN UP
			vtr::free(output_pin_name);
		}
	}

	void map_output_port(RTLIL::IdString mapping, SigSpec out_port, nnode_t *node, char *identifier, Hashtable* output_nets_hash, pool<SigBit> &cstr_bits_seen) {
		
		int base_pin_idx = node->num_output_pins;

        allocate_more_output_pins(node, out_port.size()); //?
        add_output_port_information(node, out_port.size());

        /*add name information and a net(driver) for the output */

		for (int i=0 ; i<out_port.size() ; i++) {
			npin_t* out_pin = allocate_npin();
			out_pin->name = NULL;//vtr::strdup(str(s).c_str());
			out_pin->mapping = vtr::strdup(RTLIL::unescape_id(mapping).c_str());
            add_output_pin_to_node(node, out_pin, base_pin_idx + i); // if more than one output then i + something
        			
			char* output_pin_name = sig_full_ref_name_sig(out_port[i], identifier, cstr_bits_seen);
			nnet_t* out_net = (nnet_t*)output_nets_hash->get(output_pin_name);
        	if (out_net == nullptr) {
            	out_net = allocate_nnet();
        		out_net->name = vtr::strdup(output_pin_name);
				output_nets_hash->add(output_pin_name, out_net);
			}
        	add_driver_pin_to_net(out_net, out_pin);

			// CLEAN UP
			vtr::free(output_pin_name);
		}
	}

	bool is_param_required(operation_list op) {
		switch (op) {
        	case (SL): 
        	case (SR):      
	        case (ASL):  
    	    case (ASR):    
        	case (DLATCH):  
 			case (ADLATCH): 
    	    case (SETCLR):  
        	case (SDFF): 
	        case (DFFE):    
    	    case (SDFFE):  
        	case (SDFFCE):  
	        case (DFFSR):   
    	    case (DFFSRE): 
        	case (SPRAM):  
	        case (DPRAM):  
    	    case (YMEM):    
        	case (BRAM):   
	        case (ROM):    
    	    case (FF_NODE): 
        	    return true;
	        default:
    	        return false;
    	}
	}

	int off(RTLIL::SigBit sig) {
		if (sig.wire == NULL)
			return -1;
		if (sig.wire->width != 1)
			return sig.wire->upto ? sig.wire->start_offset+sig.wire->width-sig.offset-1 : sig.wire->start_offset+sig.offset;
		return -1;
	}
	
	netlist_t* to_netlist(RTLIL::Module *top_module, RTLIL::Design *design) {

		std::vector<RTLIL::Module*> mod_list;

		std::string top_module_name;
		if (top_module_name.empty())
			for (auto module : design->modules())
				if (module->get_bool_attribute(ID::top))
					top_module_name = module->name.str();

		for (auto module : design->modules())
		{

			if (module->processes.size() != 0)
				log_error("Found unmapped processes in module %s: unmapped processes are not supported in BLIF backend!\n", log_id(module->name));
			if (module->memories.size() != 0)
				log_error("Found unmapped memories in module %s: unmapped memories are not supported in BLIF backend!\n", log_id(module->name));

			if (module->name == RTLIL::escape_id(top_module_name)) {
				top_module_name.clear();
				continue;
			}

			mod_list.push_back(module);
		}
/*
		for (auto module : mod_list) {
			log("@@@@@@@@@@@@@@\n");
			log(".model %s\n", str(module->name).c_str());

			std::map<int, RTLIL::Wire*> inputs, outputs;

			for (auto wire : module->wires()) {
				if (wire->port_input)
					inputs[wire->port_id] = wire;
				if (wire->port_output)
					outputs[wire->port_id] = wire;
			}

			log(".inputs\n");
			for (auto &it : inputs) {
				RTLIL::Wire *wire = it.second;
				for (int i = 0; i < wire->width; i++)
					log(" %s", str(RTLIL::SigSpec(wire, i)).c_str());
			}
			

			log(".outputs\n");
			for (auto &it : outputs) {
				RTLIL::Wire *wire = it.second;
				for (int i = 0; i < wire->width; i++)
					log(" %s", str(RTLIL::SigSpec(wire, i)).c_str());
			}
			log("\n");
		}
*/
		pool<SigBit> cstr_bits_seen;

		netlist_t* odin_netlist = allocate_netlist();
		Hashtable* output_nets_hash = new Hashtable();
		odin_netlist->identifier = vtr::strdup(log_id(top_module->name));

		create_top_driver_nets(odin_netlist, output_nets_hash);

		// build_top_input_node(DEFAULT_CLOCK_NAME, odin_netlist, output_nets_hash);

		std::map<int, RTLIL::Wire*> inputs, outputs;

		for (auto wire : top_module->wires()) {
			if (wire->port_input)
				inputs[wire->port_id] = wire;
			if (wire->port_output)
				outputs[wire->port_id] = wire;
		}

		for (auto &it : inputs) {
			RTLIL::Wire *wire = it.second;
			for (int i = 0; i < wire->width; i++){
				char* name_string = sig_full_ref_name_sig(RTLIL::SigBit(wire, i), odin_netlist->identifier, cstr_bits_seen);
				build_top_input_node(name_string, odin_netlist, output_nets_hash);
			}
		}

		for (auto &it : outputs) {
			RTLIL::Wire *wire = it.second;
			for (int i = 0; i < wire->width; i++) {
				char* name_string = sig_full_ref_name_sig(RTLIL::SigBit(wire, i), odin_netlist->identifier, cstr_bits_seen);
				build_top_output_node(name_string, odin_netlist, output_nets_hash);
			}
		}

		dict<std::string, std::string> connctions;
		auto &conns = top_module->connections();
		for(auto sigsig : conns)
		{
			auto s1 = sigsig.first;
			auto s2 = sigsig.second;
			for(int i=0; i<std::min(s1.size(), s2.size()); i++)
			{
				auto w1 = s1[i].wire;
				auto w2 = s2[i].wire;
				std::string s1_i_name;
				std::string s2_i_name;
				
				s1_i_name = (w1 == NULL) ? infer_wire_name(s1[i]) : make_full_ref_name(odin_netlist->identifier, NULL, NULL, infer_wire_name(s1[i]).c_str(), w1->width==1 ? off(s1[i]) : infer_wire_index(s1[i]));
				s2_i_name = (w2 == NULL) ? infer_wire_name(s2[i]) : make_full_ref_name(odin_netlist->identifier, NULL, NULL, infer_wire_name(s2[i]).c_str(), w2->width==1 ? off(s2[i]) : infer_wire_index(s2[i]));

				// log("s1_i_name:%s s2_i_name:%s\n", s1_i_name.c_str(), s2_i_name.c_str());

				if (w1 != NULL)
					connctions[s1_i_name] = s2_i_name;
				if (w2 != NULL)
					connctions[s2_i_name] = s1_i_name;
			}
		}

		for (const auto &port_name : top_module->ports) {
			// top_module->port
			// log("port in module: %s\n",log_id(port_name));
		}

		long hard_id = 0;
		for(auto cell : top_module->cells()) 
		{
			// log("cell type: %s %s\n", cell->type.c_str(), str(cell->type).c_str());

    		nnode_t* new_node = allocate_nnode(my_location);
			new_node->cell = cell;

    		new_node->related_ast_node = NULL;

			// new_node->type = yosys_subckt_strmap[cell->type.c_str()];
			new_node->type = yosys_subckt_strmap[str(cell->type).c_str()];

			if (new_node->type == NO_OP) {

				/**
				 * ast.cc:1657
				 * 	std::string modname;
				 *	if (parameters.size() == 0)
				 *		modname = stripped_name;
				 *	else if (para_info.size() > 60)
				 *		modname = "$paramod$" + sha1(para_info) + stripped_name;
				 *	else
				 *		modname = "$paramod" + stripped_name + para_info;
				 * 
				 */

				// printf("sth weird detected!!! %s\n", str(cell->type).c_str());
				if (cell->type.begins_with("$paramod$")) // e.g. $paramod$b509a885304d9c8c49f505bb9d0e99a9fb676562\dual_port_ram
				{
					std::regex regex("^\\$paramod\\$\\w+\\\\(\\w+)$");
        			std::smatch m;
					std::string modname(str(cell->type));
					if(regex_match(modname, m, regex)) {
						// printf("\tand translated to!!! %s\n", m.str(1).c_str());
						new_node->type = yosys_subckt_strmap[m.str(1).c_str()];
					}
				} else if (cell->type.begins_with("$paramod\\")) // e.g. $paramod\dual_port_ram\ADDR_WIDTH?4'0100\DATA_WIDTH?4'0101
				{
					std::regex regex("^\\$paramod\\\\(\\w+)(\\\\\\S+)*$");
        			std::smatch m;
					std::string modname(str(cell->type));
					if(regex_match(modname, m, regex)) {
						// printf("\tand translated to!!! %s\n", m.str(1).c_str());
						new_node->type = yosys_subckt_strmap[m.str(1).c_str()];
					}
				} else {
					new_node->type = HARD_IP;
					// odin_sprintf(new_name, "\\%s~%ld", subcircuit_stripped_name, hard_block_number - 1);
                    /* Detect used hard block for the blif generation */
                    t_model* hb_model = find_hard_block(str(cell->type).c_str());
                    if (hb_model) {
                        hb_model->used = 1;
                    }
					std::string modname(str(cell->type));
					// Create a fake ast node.
        			if (new_node->type == HARD_IP) {
            			new_node->related_ast_node = create_node_w_type(HARD_BLOCK, my_location);
            			new_node->related_ast_node->children = (ast_node_t**)vtr::calloc(1, sizeof(ast_node_t*));
            			new_node->related_ast_node->identifier_node = create_tree_node_id(vtr::strdup(modname.c_str()), my_location);
        			}
				}
			}

			for (auto &conn : cell->connections()) {

				if (cell->input(conn.first) && conn.second.size()>0) {
					// if (conn.first == ID::RD_ARST || conn.first == ID::RD_SRST )
					// 	continue;
					// log("cell->input %s\n", RTLIL::unescape_id(conn.first).c_str());
					map_input_port(conn.first, conn.second, new_node, odin_netlist->identifier, cstr_bits_seen);
				}

				if (cell->output(conn.first) && conn.second.size()>0) {
					map_output_port(conn.first, conn.second, new_node, odin_netlist->identifier, output_nets_hash, cstr_bits_seen);
				}

				// Module *m = design->module(cell->type);
				// Wire *w = m ? m->wire(conn.first) : nullptr;

				// if (w == nullptr) {
				// 	for (int i = 0; i < GetSize(conn.second); i++)
				// 		log(" -%s[%d]=%s\n", unescape_id(conn.first).c_str(), i, str(conn.second[i]).c_str());
				// } else {
				// 	for (int i = 0; i < std::min(GetSize(conn.second), GetSize(w)); i++) {
				// 		SigBit sig(w, i);
				// 		log(" +%s[%d]=%s\n", unescape_id(conn.first).c_str(), sig.wire->upto ?
				// 				sig.wire->start_offset+sig.wire->width-sig.offset-1 :
				// 				sig.wire->start_offset+sig.offset, str(conn.second[i]).c_str());
				// 	}
				// }
            }

			// if (new_node->type == HARD_IP) {
			// 	RTLIL::Module* inst_module = design->module(cell->type);
			// 	if (inst_module && inst_module->get_blackbox_attribute()) {
			// 		std::map<int, RTLIL::Wire*> inputs, outputs;

			// 		for (auto wire : inst_module->wires()) {
			// 			if (wire->port_input)
			// 				inputs[wire->port_id] = wire;
			// 			if (wire->port_output)
			// 				outputs[wire->port_id] = wire;
			// 		}

			// 		log("\ninputs:\n");
			// 		int in_idx=0;
			// 		for (auto &it : inputs) {
			// 			RTLIL::Wire *wire = it.second;
			// 			for (int i = 0; i < wire->width; i++){
			// 				log(" %s,", str(RTLIL::SigSpec(wire, i)).c_str());
			// 				new_node->input_pins[in_idx++]->mapping = vtr::strdup(RTLIL::unescape_id(wire->name).c_str());
			// 				// new_node->input_pins[i]->mapping = vtr::strdup(str(RTLIL::SigSpec(wire, i)).c_str());
			// 			}	
			// 		}
			

			// 		log("\noutputs:\n");
			// 		int out_idx=0;
			// 		for (auto &it : outputs) {
			// 			RTLIL::Wire *wire = it.second;
			// 			for (int i = 0; i < wire->width; i++) {
			// 				log(" %s,", str(RTLIL::SigSpec(wire, i)).c_str());
			// 				new_node->output_pins[out_idx++]->mapping = vtr::strdup(RTLIL::unescape_id(wire->name).c_str());
			// 			}
			// 		}	
			// 	}
			// }

			if (is_param_required(new_node->type)) {
				
				if (cell->hasParam(ID::SRST_VALUE)) {
					auto value = vtr::strdup(cell->getParam(ID::SRST_VALUE).as_string().c_str());
                    new_node->attributes->sreset_value = std::bitset<sizeof(long) * 8>(value).to_ulong();
					vtr::free(value);
                }
				
				if (cell->hasParam(ID::ARST_VALUE)) {
					auto value = vtr::strdup(cell->getParam(ID::ARST_VALUE).as_string().c_str());
                    new_node->attributes->areset_value = std::bitset<sizeof(long) * 8>(value).to_ulong();
					vtr::free(value);
                } 
				
				if (cell->hasParam(ID::OFFSET)) {
					auto value = vtr::strdup(cell->getParam(ID::OFFSET).as_string().c_str());
                    new_node->attributes->offset = std::bitset<sizeof(long) * 8>(value).to_ulong();
					vtr::free(value);
                }
				
				if (cell->hasParam(ID::SIZE)) {
					auto value = vtr::strdup(cell->getParam(ID::SIZE).as_string().c_str());
                    new_node->attributes->size = std::bitset<sizeof(long) * 8>(value).to_ulong();
					vtr::free(value);
                } 
				
				if (cell->hasParam(ID::WIDTH)) {
					auto value = vtr::strdup(cell->getParam(ID::WIDTH).as_string().c_str());
                    new_node->attributes->DBITS = std::bitset<sizeof(long) * 8>(value).to_ulong();
					vtr::free(value);
                }  
				
				if (cell->hasParam(ID::RD_PORTS)) {
					auto value = vtr::strdup(cell->getParam(ID::RD_PORTS).as_string().c_str());
                    new_node->attributes->RD_PORTS = std::bitset<sizeof(long) * 8>(value).to_ulong();
					vtr::free(value);
                }  
				
				if (cell->hasParam(ID::WR_PORTS)) {
					auto value = vtr::strdup(cell->getParam(ID::WR_PORTS).as_string().c_str());
                    new_node->attributes->WR_PORTS = std::bitset<sizeof(long) * 8>(value).to_ulong();
					vtr::free(value);
                }  
				
				if (cell->hasParam(ID::ABITS)) {
					auto value = vtr::strdup(cell->getParam(ID::ABITS).as_string().c_str());
                    new_node->attributes->ABITS = std::bitset<sizeof(long) * 8>(value).to_ulong();
					vtr::free(value);
                }

				// if (cell->hasParam(ID::MEMID)) {
                //     std::string memory_id(vtr::strtok(NULL, TOKENS, file, buffer));
                //     unsigned first_back_slash = memory_id.find_last_of(YOSYS_ID_FIRST_DELIMITER);
                //     unsigned last_double_quote = memory_id.find_last_not_of(YOSYS_ID_LAST_DELIMITER);
                //     attributes->memory_id = vtr::strdup(
                //         memory_id.substr(
                //                      first_back_slash + 1, last_double_quote - first_back_slash)
                //             .c_str());
                // }

				if (cell->hasParam(ID::MEMID)) {
					auto value = vtr::strdup(cell->getParam(ID::MEMID).as_string().c_str());
					RTLIL::IdString ids = cell->getParam(ID::MEMID).decode_string();
					// log("MEMID: %s\t%s\n", value, RTLIL::unescape_id(ids).c_str());
					new_node->attributes->memory_id = vtr::strdup(RTLIL::unescape_id(ids).c_str());
				}

				if (cell->hasParam(ID::A_SIGNED)) {
					new_node->attributes->port_a_signed = cell->getParam(ID::A_SIGNED).as_bool() ? SIGNED : UNSIGNED;
				}

				if (cell->hasParam(ID::B_SIGNED)) {
					new_node->attributes->port_b_signed = cell->getParam(ID::B_SIGNED).as_bool() ? SIGNED : UNSIGNED;
				}

				if (cell->hasParam(ID::CLK_POLARITY)) {
					new_node->attributes->clk_edge_type = cell->getParam(ID::CLK_POLARITY).as_bool() ? RISING_EDGE_SENSITIVITY : FALLING_EDGE_SENSITIVITY;
				}

				if (cell->hasParam(ID::CLR_POLARITY)) {
					new_node->attributes->clr_polarity = cell->getParam(ID::CLR_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

				if (cell->hasParam(ID::SET_POLARITY)) {
					new_node->attributes->set_polarity = cell->getParam(ID::SET_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

				if (cell->hasParam(ID::EN_POLARITY)) {
					new_node->attributes->enable_polarity = cell->getParam(ID::EN_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

				if (cell->hasParam(ID::ARST_POLARITY)) {
					new_node->attributes->areset_polarity = cell->getParam(ID::ARST_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

				if (cell->hasParam(ID::SRST_POLARITY)) {
					new_node->attributes->sreset_polarity = cell->getParam(ID::SRST_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

				if (cell->hasParam(ID::RD_CLK_ENABLE)) {
					new_node->attributes->RD_CLK_ENABLE = cell->getParam(ID::RD_CLK_ENABLE).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

				if (cell->hasParam(ID::WR_CLK_ENABLE)) {
					new_node->attributes->WR_CLK_ENABLE = cell->getParam(ID::WR_CLK_ENABLE).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

				if (cell->hasParam(ID::RD_CLK_POLARITY)) {
					new_node->attributes->RD_CLK_POLARITY = cell->getParam(ID::RD_CLK_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

				if (cell->hasParam(ID::WR_CLK_POLARITY)) {
					new_node->attributes->WR_CLK_POLARITY = cell->getParam(ID::WR_CLK_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
				}

			}

			/* add a name for the node, keeping the name of the node same as the output */
        	// new_node->name = vtr::strdup(log_id(port_Y[0].wire->name)); // @TODO recheck later
			
			new_node->name = vtr::strdup(stringf("%s~%ld", (((new_node->type == HARD_IP) ? "\\" : "") + str(cell->type)).c_str(), hard_id++).c_str()); 
			// new_node->name = vtr::strdup(stringf("%s~%ld", str(cell->type).c_str(), hard_id++).c_str()); 

        	/*add this node to blif_netlist as an internal node */
        	odin_netlist->internal_nodes = (nnode_t**)vtr::realloc(odin_netlist->internal_nodes, sizeof(nnode_t*) * (odin_netlist->num_internal_nodes + 1));
    		odin_netlist->internal_nodes[odin_netlist->num_internal_nodes++] = new_node;

		}

		// for (auto bit : cstr_bits_seen)
		// 	log("pool %s[%d]\n", infer_wire_name(bit).c_str(), infer_wire_index(bit));
		
		// for (auto &conn : top_module->connections())
		// for (int i = 0; i < conn.first.size(); i++)
		// {
		// 	SigBit lhs_bit = conn.first[i];
		// 	SigBit rhs_bit = conn.second[i];

			// if (cstr_bits_seen.count(lhs_bit) != 0)
			// 	continue;
			// log("found something %s[%d] %s[%d]\n", infer_wire_name(lhs_bit).c_str(), infer_wire_index(lhs_bit), infer_wire_name(rhs_bit).c_str(), infer_wire_index(rhs_bit));
		// }

		// add intermediate buffer nodes
		for (auto &conn : top_module->connections())
		for (int i = 0; i < conn.first.size(); i++)
		{
			SigBit lhs_bit = conn.first[i];
			SigBit rhs_bit = conn.second[i];

			if (cstr_bits_seen.count(lhs_bit) == 0)
				continue;
			
			nnode_t* buf_node = allocate_nnode(my_location);
			buf_node->cell = NULL;

    		buf_node->related_ast_node = NULL;

			buf_node->type = BUF_NODE;

        	allocate_more_input_pins(buf_node, 1); 
			add_input_port_information(buf_node, 1);
				
			char* in_pin_name = sig_full_ref_name_sig(rhs_bit, odin_netlist->identifier, cstr_bits_seen);
			npin_t* in_pin = allocate_npin();
			in_pin->name = vtr::strdup(in_pin_name);
            in_pin->type = INPUT;
	        add_input_pin_to_node(buf_node, in_pin, 0);

			// CLEAN UP
			// vtr::free(in_pin_name);

        	allocate_more_output_pins(buf_node, 1); //?
        	add_output_port_information(buf_node, 1);

			npin_t* out_pin = allocate_npin();
			out_pin->name = NULL;
            // out_pin->type = OUTPUT;
            add_output_pin_to_node(buf_node, out_pin, 0); // if more than one output then i + something
        			
			char* output_pin_name = sig_full_ref_name_sig(lhs_bit, odin_netlist->identifier, cstr_bits_seen);
			nnet_t* out_net = (nnet_t*)output_nets_hash->get(output_pin_name);
        	if (out_net == nullptr) {
            	out_net = allocate_nnet();
        		out_net->name = vtr::strdup(output_pin_name);
				output_nets_hash->add(output_pin_name, out_net);
			}
        	add_driver_pin_to_net(out_net, out_pin);

			buf_node->name = vtr::strdup(output_pin_name); 

        	/*add this node to blif_netlist as an internal node */
        	odin_netlist->internal_nodes = (nnode_t**)vtr::realloc(odin_netlist->internal_nodes, sizeof(nnode_t*) * (odin_netlist->num_internal_nodes + 1));
    		odin_netlist->internal_nodes[odin_netlist->num_internal_nodes++] = buf_node;

			// CLEAN UP
			// vtr::free(output_pin_name);
		}

		// output_nets_hash->log();
		// print_netlist_for_checking(odin_netlist, "pre");
		// hook_up_nets(odin_netlist, output_nets_hash, top_module);
		hook_up_nets(odin_netlist, output_nets_hash, connctions);
		log("after hookup\n");
		delete output_nets_hash;
		return odin_netlist;
	}

	void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_mode* mode) {
    	for (int i = 0; i < mode->num_pb_type_children; i++) {
        	get_physical_luts(pb_lut_list, &mode->pb_type_children[i]);
	    }
	}

	void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_pb_type* pb_type) {
    	if (pb_type) {
        	if (pb_type->class_type == LUT_CLASS) {
            	pb_lut_list.push_back(pb_type);
	        } else {
    	        for (int i = 0; i < pb_type->num_modes; i++) {
        	        get_physical_luts(pb_lut_list, &pb_type->modes[i]);
            	}
	        }
    	}
	}

	void set_physical_lut_size(std::vector<t_logical_block_type>& logical_block_types) {
    	std::vector<t_pb_type*> pb_lut_list;

	    for (t_logical_block_type& logical_block : logical_block_types) {
    	    if (logical_block.index != EMPTY_TYPE_INDEX) {
        	    get_physical_luts(pb_lut_list, logical_block.pb_type);
	        }
    	}
	    for (t_pb_type* pb_lut : pb_lut_list) {
    	    if (pb_lut) {
        	    if (pb_lut->num_input_pins < physical_lut_size || physical_lut_size < 1) {
            	    physical_lut_size = pb_lut->num_input_pins;
	            }
    	    }
	    }
	}

	void elaborate(netlist_t *odin_netlist) {
    	double elaboration_time = wall_time();

	    /* Perform any initialization routines here */
    	find_hard_multipliers();
    	find_hard_adders();
    	//find_hard_adders_for_sub();
    	register_hard_blocks();

    	// module_names_to_idx = sc_new_string_cache();

		blif_elaborate_top(odin_netlist);

    	elaboration_time = wall_time() - elaboration_time;
    	log("\nElaboration Time: ");
    	log_time(elaboration_time);
    	log("\n--------------------------------------------------------------------\n");
	}

	void optimization(netlist_t *odin_netlist) {
    	double optimization_time = wall_time();

	    if (odin_netlist) {
    	    // Can't levelize yet since the large muxes can look like combinational loops when they're not
        	check_netlist(odin_netlist);

	        //START ################# NETLIST OPTIMIZATION ############################

    	    /* point for all netlist optimizations. */
        	log("Performing Optimization on the Netlist\n");
	        if (hard_multipliers) {
    	        /* Perform a splitting of the multipliers for hard block mults */
        	    reduce_operations(odin_netlist, MULTIPLY);
            	iterate_multipliers(odin_netlist);
	            clean_multipliers();
    	    }

	        if (block_memories_info.read_only_memory_list || block_memories_info.block_memory_list) {
    	        /* Perform a hard block registration and splitting in width for Yosys generated memory blocks */
        	    iterate_block_memories(odin_netlist);
            	free_block_memories();
        	}

	        if (single_port_rams || dual_port_rams) {
    	        /* Perform a splitting of any hard block memories */
        	    iterate_memories(odin_netlist);
            	free_memory_lists();
        	}

	        if (hard_adders) {
    	        /* Perform a splitting of the adders for hard block add */
        	    reduce_operations(odin_netlist, ADD);
            	iterate_adders(odin_netlist);
	            clean_adders();

    	        /* Perform a splitting of the adders for hard block sub */
        	    reduce_operations(odin_netlist, MINUS);
            	iterate_adders_for_sub(odin_netlist);
	            clean_adders_for_sub();
    	    }

        	//END ################# NETLIST OPTIMIZATION ############################

	        if (configuration.output_netlist_graphs)
    	        graphVizOutputNetlist(configuration.debug_output_path, "optimized", 2, odin_netlist); /* Path is where we are */
    	}

	    optimization_time = wall_time() - optimization_time;
    	log("\nOptimization Time: ");
    	log_time(optimization_time);
    	log("\n--------------------------------------------------------------------\n");
	}

	void techmap(netlist_t *odin_netlist) {
    	double techmap_time = wall_time();

    	if (odin_netlist) {
        	/* point where we convert netlist to FPGA or other hardware target compatible format */
	        log("Performing Partial Technology Mapping to the target device\n");
    	    partial_map_top(odin_netlist);
        	mixer->perform_optimizations(odin_netlist);

	        /* Find any unused logic in the netlist and remove it */
    	    remove_unused_logic(odin_netlist);
    	}

	    techmap_time = wall_time() - techmap_time;
    	log("\nTechmap Time: ");
	    log_time(techmap_time);
    	log("\n--------------------------------------------------------------------\n");
	}

	void report(netlist_t *odin_netlist) {

    	if (odin_netlist) {

        	report_mult_distribution();
        	report_add_distribution();
        	report_sub_distribution();

        	compute_statistics(odin_netlist, true);
    	}
	}

	void log_time(double time) {
    	log("%.1fms", time * 1000);
	}
	
	OdinoPass() : Pass("odino", "ODIN_II partial technology mapper") { }
	void help() override
	{
		log("\n");
		log("This pass calls simplemap pass by default.\n");
		log("\n");
		log("    -a ARCHITECTURE_FILE\n");
		log("        VTR FPGA architecture description file (XML)\n");
		log("\n");
		log("    -c XML_CONFIGURATION_FILE\n");
		log("        Configuration file\n");
		log("\n");
		log("    -b BLIF_FILE\n");
		log("        input BLIF_FILE\n");
		log("\n");
		log("    -top top_module\n");
		log("        set the specified module as design top module\n");
		log("\n");
		log("    -y YOSYS_OUTPUT_FILE_PATH\n");
		log("        Output blif file path after yosys elaboration\n");
		log("\n");
		log("    -o ODIN_OUTPUT_FILE_PATH\n");
		log("        Output blif file path after odin partial mapper\n");
		log("\n");
		log("    -fflegalize\n");
		log("        Make all flip-flops rising edge to be compatible with VPR (may add inverters)\n");
		log("\n");
		log("    -exact_mults int_value\n");
		log("        To enable mixing hard block and soft logic implementation of adders\n");
		log("\n");
		log("    -mults_ratio float_value\n");
		log("        To enable mixing hard block and soft logic implementation of adders\n");
		log("\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
		bool flag_arch_file = false;
		bool flag_config_file = false;
		bool flag_no_pass = false;
		bool flag_load_primitives = false;
		bool flag_read_verilog_input = false;
		bool flag_sim_hold_low = false;
		bool flag_sim_hold_high = false;
		std::string arch_file_path;
		std::string config_file_path;
		std::string top_module_name;
		std::string yosys_coarsen_blif_output("yosys_coarsen.blif");
		std::string odin_mapped_blif_output("odin_mapped.blif");
		std::string verilog_input_path;
		std::vector<std::string> sim_hold_low;
		std::vector<std::string> sim_hold_high;
		std::string DEFAULT_OUTPUT(".");
		
		global_args.sim_directory.set(DEFAULT_OUTPUT, argparse::Provenance::DEFAULT);

		global_args.exact_mults.set(-1, argparse::Provenance::DEFAULT);
		global_args.mults_ratio.set(-1.0, argparse::Provenance::DEFAULT);
		
		log_header(design, "Starting odintechmap pass.\n");

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			if (args[argidx] == "-a" && argidx+1 < args.size()) {
				arch_file_path = args[++argidx];
				flag_arch_file = true;
				continue;
			}
			if (args[argidx] == "-c" && argidx+1 < args.size()) {
				config_file_path = args[++argidx];
				flag_config_file = true;
				continue;
			}
			if (args[argidx] == "-nopass") {
				flag_no_pass = true;
				continue;
			}
			if (args[argidx] == "-top" && argidx+1 < args.size()) {
				top_module_name = args[++argidx];
				continue;
			}
			if (args[argidx] == "-y" && argidx+1 < args.size()) {
				yosys_coarsen_blif_output = args[++argidx];
				continue;
			}
			if (args[argidx] == "-o" && argidx+1 < args.size()) {
				// global_args.output_file @TODO
				odin_mapped_blif_output = args[++argidx];
				continue;
			}
			if (args[argidx] == "-prim") {
				flag_load_primitives = true;
				continue;
			}
			if (args[argidx] == "-v" && argidx+1 < args.size()) {
				flag_read_verilog_input = true;
				verilog_input_path = args[++argidx];
				continue;
			}
			if (args[argidx] == "-b" && argidx+1 < args.size()) {
				global_args.blif_file.set(args[++argidx], argparse::Provenance::SPECIFIED);
				continue;
			}
			if (args[argidx] == "-fflegalize") {
				configuration.fflegalize = true;
				continue;
			}
			if (args[argidx] == "-exact_mults" && argidx+1 < args.size()) {
				global_args.exact_mults.set(atoi(args[++argidx].c_str()), argparse::Provenance::SPECIFIED);
				continue;
			}
			if (args[argidx] == "-mults_ratio" && argidx+1 < args.size()) {
				global_args.mults_ratio.set(atof(args[++argidx].c_str()), argparse::Provenance::SPECIFIED);
				continue;
			}
		}
		extra_args(args, argidx, design);

		// t_arch Arch;
		// global_args_t global_args;
		std::vector<t_physical_tile_type> physical_tile_types;
		std::vector<t_logical_block_type> logical_block_types;

		try {
        	/* Some initialization */
        	one_string = vtr::strdup(ONE_VCC_CNS);
        	zero_string = vtr::strdup(ZERO_GND_ZERO);
        	pad_string = vtr::strdup(ZERO_PAD_ZERO);

    	} catch (vtr::VtrError& vtr_error) {
        	log_error("Odin failed to initialize %s with exit code%d\n", vtr_error.what(), ERROR_INITIALIZATION);
    	}

		mixer = new HardSoftLogicMixer();
		set_default_config();

		if (global_args.mults_ratio >= 0.0 && global_args.mults_ratio <= 1.0) {
        	delete mixer->_opts[MULTIPLY];
        	mixer->_opts[MULTIPLY] = new MultsOpt(global_args.mults_ratio);
    	} else if (global_args.exact_mults >= 0) {
        	delete mixer->_opts[MULTIPLY];
        	mixer->_opts[MULTIPLY] = new MultsOpt(global_args.exact_mults);
    	}

		configuration.coarsen = true;

		/* read the confirguration file .. get options presets the config values just in case theyr'e not read in with config file */
    	if (flag_config_file) {
        	log("Reading Configuration file\n");
        	try {
            	read_config_file(config_file_path.c_str());
        	} catch (vtr::VtrError& vtr_error) {
            	log_error("Odin Failed Reading Configuration file %s with exit code%d\n", vtr_error.what(), ERROR_PARSE_CONFIG);
        	}
    	}

		if (flag_arch_file) {
			log("Architecture: %s\n", vtr::basename(arch_file_path).c_str());

			log("Reading FPGA Architecture file\n");
        	try {
            	XmlReadArch(arch_file_path.c_str(), false, &Arch, physical_tile_types, logical_block_types);
	            set_physical_lut_size(logical_block_types);
    	    } catch (vtr::VtrError& vtr_error) {
        	    log_error("Odin Failed to load architecture file: %s with exit code%d\n", vtr_error.what(), ERROR_PARSE_ARCH);
        	}
		}
		log("Using Lut input width of: %d\n", physical_lut_size);

		if(flag_load_primitives) {

		}

		if(!flag_no_pass) {
			run_pass("read_verilog -nomem2reg +/odintechmap/primitives.v");
			run_pass("setattr -mod -set keep_hierarchy 1 single_port_ram");
			run_pass("setattr -mod -set keep_hierarchy 1 dual_port_ram");
		}

		// ********* start dsp handling ************
		Verilog::Writer vw = Verilog::Writer();
    	vw._create_file(configuration.dsp_verilog.c_str());

    	t_model* hb = Arch.models;
    	while (hb) {
        	// declare hardblocks in a verilog file
        	if (strcmp(hb->name, SINGLE_PORT_RAM_string) && strcmp(hb->name, DUAL_PORT_RAM_string) && strcmp(hb->name, "multiply") && strcmp(hb->name, "adder"))
            	vw.declare_blackbox(hb->name);

        	hb = hb->next;
    	}

    	vw._write(NULL);
    	run_pass(std::string("read_verilog -nomem2reg " + configuration.dsp_verilog));
		// ********* finished dsp handling ************

		if (flag_read_verilog_input) {
			log("Verilog: %s\n", vtr::basename(verilog_input_path).c_str());
			run_pass("read_verilog -sv -nolatches " + verilog_input_path);
		}

		if (top_module_name.empty()) {
			run_pass("hierarchy -check -auto-top -purge_lib", design);
		} else {
			run_pass("hierarchy -check -top " + top_module_name, design);
		}

		if(!flag_no_pass) {
			run_pass("proc; opt;");
			run_pass("fsm; opt;");
			run_pass("memory_collect; memory_dff; opt;");
			run_pass("autoname");
			run_pass("check");

			run_pass("techmap -map +/odintechmap/adff2dff.v");
        	run_pass("techmap -map +/odintechmap/adffe2dff.v");
        	run_pass("techmap */t:$shift */t:$shiftx");

			run_pass("flatten");
			run_pass("pmuxtree");
			run_pass("wreduce");
			run_pass("opt -undriven -full; opt_muxtree; opt_expr -mux_undef -mux_bool -fine;;;");
			run_pass("autoname");
			run_pass("stat");
			run_pass("write_blif -blackbox -param -impltf " + yosys_coarsen_blif_output);
		}

		design->sort();

		if (design->top_module()->processes.size() != 0)
			log_error("Found unmapped processes in top module %s: unmapped processes are not supported in odintechmap pass!\n", log_id(design->top_module()->name));
		if (design->top_module()->memories.size() != 0)
			log_error("Found unmapped memories in module %s: unmapped memories are not supported in BLIF backend!\n", log_id(design->top_module()->name));



		double synthesis_time = wall_time();

		log("--------------------------------------------------------------------\n");
    	log("High-level Synthesis Begin\n");

		netlist_t* transformed = to_netlist(design->top_module(), design);

		/* Performing elaboration for input digital circuits */
    	try {
        	elaborate(transformed);
        	log("Successful Elaboration of the design by Odin-II\n");
    	} catch (vtr::VtrError& vtr_error) {
        	log_error("Odin-II Failed to parse Verilog / load BLIF file: %s with exit code:%d \n", vtr_error.what(), ERROR_ELABORATION);
    	}

		/* Performing netlist optimizations */
    	try {
        	optimization(transformed);
        	log("Successful Optimization of netlist by Odin-II\n");
    	} catch (vtr::VtrError& vtr_error) {
        	log_error("Odin-II Failed to perform netlist optimization %s with exit code:%d \n", vtr_error.what(), ERROR_OPTIMIZATION);
    	}

		/* Performaing partial tech. map to the target device */
    	try {
        	techmap(transformed);
        	log("Successful Partial Technology Mapping by Odin-II\n");
    	} catch (vtr::VtrError& vtr_error) {
        	log_error("Odin-II Failed to perform partial mapping to target device %s with exit code:%d \n", vtr_error.what(), ERROR_TECHMAP);
    	}

		synthesis_time = wall_time() - synthesis_time;

		log("Writing Netlist to BLIF file\n");

		GenericWriter after_writer = GenericWriter();
		after_writer._create_file(odin_mapped_blif_output.c_str(), _BLIF);
		after_writer._write(transformed);

		report(transformed);
		// compute_statistics(transformed, true);

		log("\nTotal Synthesis Time: ");
    	log_time(synthesis_time);
    	log("\n--------------------------------------------------------------------\n");

		free_netlist(transformed);
		free_arch(&Arch);
    	free_type_descriptors(logical_block_types);
    	free_type_descriptors(physical_tile_types);

		if (one_string)
        	vtr::free(one_string);
    	if (zero_string)
        	vtr::free(zero_string);
    	if (pad_string)
        	vtr::free(pad_string);

		log("odino pass finished.\n");
	}
} OdinoPass;

PRIVATE_NAMESPACE_END