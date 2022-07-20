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

// #include "arch_fpga.h"
// #include "partial_map.h"
// #include "adders.h"
#include "odin_ii.h"
#include "odin_util.h"
#include "odin_globals.h"
#include "netlist_utils.h"

#include "vtr_util.h"
#include "vtr_memory.h"

#include "netlist_check.h"

#include "partial_map.h"

#include "GenericWriter.hpp"

#include "netlist_visualizer.h"

#include "BLIFElaborate.hpp"

#include "simulate_blif.h"
#include "adders.h"
#include "BlockMemories.hpp"
#include "subtractions.h"
#include "netlist_cleanup.h"
#include "netlist_statistic.h"

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

	void get_type_mappers(dict<IdString, operation_list> &mappers)
	{
		mappers[ID($logic_not)]         = LOGICAL_NOT;

		// binary ops
		mappers[ID($and)]        = BITWISE_AND;
		mappers[ID($or)]         = BITWISE_OR;
		mappers[ID($xor)]        = BITWISE_XOR;
		mappers[ID($xnor)]       = BITWISE_XNOR;
		// mappers[ID($shl)]         =  
		// mappers[ID($shr)]         = 
		// mappers[ID($sshl)]         = 
		// mappers[ID($sshr)]         = 
		// mappers[ID($shift)]         =  
		// mappers[ID($shiftx)]         = 
		// mappers[ID($lt)]         =  
		// mappers[ID($le)]         =  
		// mappers[ID($eq)]         = 
		// mappers[ID($ne)]         = 
		// mappers[ID($eqx)]         = 
		// mappers[ID($nex)]         = 
		// mappers[ID($ge)]         = 
		// mappers[ID($gt)]         = 
		// mappers[ID($add)]         = 
		// mappers[ID($sub)]         = 
		// mappers[ID($mul)]         = MULTIPLY;
		// mappers[ID($div)]         = DIVIDE;
		// mappers[ID($mod)]         = MODULO;
		// mappers[ID($divfloor)]         = 
		// mappers[ID($modfloor)]         = 
		// mappers[ID($pow)]         = 
		mappers[ID($logic_and)]         = LOGICAL_AND;
		mappers[ID($logic_or)]         = LOGICAL_OR;
		// mappers[ID($concat)]         = 
		// mappers[ID($macc)]         = 
		mappers[ID($mux)]		    	= MUX_2;

		//unary ops
		mappers[ID($not)]         = BITWISE_NOT;
		mappers[ID($pos)]         = ADD;
		mappers[ID($neg)]         = MINUS;
		// mappers[ID($reduce_and)]  =  
		// mappers[ID($reduce_or)]   =  
		// mappers[ID($reduce_xor)]  =  
		// mappers[ID($reduce_xnor)] =  
		// mappers[ID($reduce_bool)] = 
		mappers[ID($logic_not)]   =  LOGICAL_NOT;
		// mappers[ID($slice)]       =  
		// mappers[ID($lut)]         =  
		// mappers[ID($sop)]         = 

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

	std::string dump_param(RTLIL::Const param) {
		std::string f;
		if (param.flags & RTLIL::CONST_FLAG_STRING) {
			std::string str = param.decode_string();
			for (char ch : str)
				if (ch == '"' || ch == '\\')
					f.append(stringf("\\%c", ch));
				else if (ch < 32 || ch >= 127)
					f.append(stringf("\\%03o", ch));
				else
					f.append(stringf("%c", ch));
				f.append(stringf("\"\n"));
		} else
			f.append(stringf("%s\n", param.as_string().c_str()));
		return f;
	}

	int off(RTLIL::SigBit sig) {
		if (sig.wire == NULL)
			return -1;
		if (sig.wire->width != 1)
			return sig.wire->upto ? sig.wire->start_offset+sig.wire->width-sig.offset-1 : sig.wire->start_offset+sig.offset;
		return -1;
	}
	
	netlist_t* to_netlist(RTLIL::Module *top_module, RTLIL::Design *design) {

		pool<SigBit> cstr_bits_seen;

		dict<IdString, operation_list> mappers;
		get_type_mappers(mappers);

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

				log("s1_i_name:%s s2_i_name:%s\n", s1_i_name.c_str(), s2_i_name.c_str());

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
			log("cell type: %s %s\n", cell->type.c_str(), str(cell->type).c_str());

    		nnode_t* new_node = allocate_nnode(my_location);
			new_node->cell = cell;

    		new_node->related_ast_node = NULL;

			// new_node->type = yosys_subckt_strmap[cell->type.c_str()];
			new_node->type = yosys_subckt_strmap[str(cell->type).c_str()];

			for (auto &conn : cell->connections()) {
                auto p = cell->getPort(conn.first);
                auto q = conn.second;
                log("%s, %s, %s\n", 
                unescape_id(conn.first).c_str(), 
                conn.second.as_string().c_str(), 
                cell->getPort(conn.first).as_string().c_str()
                );

				if (cell->input(conn.first) /*&& conn.second.size() == 1*/ && conn.second.size()>0) {
					// if (conn.first == ID::RD_ARST || conn.first == ID::RD_SRST )
					// 	continue;
					map_input_port(conn.second, new_node, odin_netlist->identifier, cstr_bits_seen);
				}

				if (cell->output(conn.first) /*&& conn.second.size() == 1*/ && conn.second.size()>0) {
					map_output_port(conn.second, new_node, odin_netlist->identifier, output_nets_hash, cstr_bits_seen);
				}

                // if (conn.second.size() == 1) {
				// 	log(" .%s=%s\n", unescape_id(conn.first).c_str(), str(conn.second[0]).c_str());

				// 	continue;
				// }

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
					log("MEMID: %s\t%s\n", value, RTLIL::unescape_id(ids).c_str());
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
			new_node->name = vtr::strdup(stringf("%s~%ld", str(cell->type).c_str(), hard_id++).c_str()); 

        	/*add this node to blif_netlist as an internal node */
        	odin_netlist->internal_nodes = (nnode_t**)vtr::realloc(odin_netlist->internal_nodes, sizeof(nnode_t*) * (odin_netlist->num_internal_nodes + 1));
    		odin_netlist->internal_nodes[odin_netlist->num_internal_nodes++] = new_node;

		}

		for (auto bit : cstr_bits_seen)
			log("pool %s[%d]\n", infer_wire_name(bit).c_str(), infer_wire_index(bit));
		
		for (auto &conn : top_module->connections())
		for (int i = 0; i < conn.first.size(); i++)
		{
			SigBit lhs_bit = conn.first[i];
			SigBit rhs_bit = conn.second[i];

			// if (cstr_bits_seen.count(lhs_bit) != 0)
			// 	continue;
			log("found something %s[%d] %s[%d]\n", infer_wire_name(lhs_bit).c_str(), infer_wire_index(lhs_bit), infer_wire_name(rhs_bit).c_str(), infer_wire_index(rhs_bit));
		}

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

		output_nets_hash->log();
		// print_netlist_for_checking(odin_netlist, "pre");
		// hook_up_nets(odin_netlist, output_nets_hash, top_module);
		hook_up_nets(odin_netlist, output_nets_hash, connctions);
		log("after hookup\n");
		delete output_nets_hash;
		return odin_netlist;
	}
	
	OdinoPass() : Pass("odino", "ODIN_II partial technology mapper") { }
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
		log("    -top top_module\n");
		log("        set the specified module as design top module\n");
		log("\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
		bool flag_arch_file = false;
		bool flag_arch_info = false;
		bool flag_simple_map = true;
		bool flag_no_pass = false;
		std::string arch_file_path;
		std::string top_module_name;
		
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
			if (args[argidx] == "-nopass") {
				flag_no_pass = true;
				continue;
			}
			if (args[argidx] == "-top" && argidx+1 < args.size()) {
				top_module_name = args[++argidx];
				continue;
			}
		}
		extra_args(args, argidx, design);

		// CellTypes yosys_types;
		// yosys_types.setup();
		// auto ct = yosys_types.cell_types.at(ID($mem_v2));
		// log("$mem_v2 inputs : %d\n", ct.inputs.size());
		// for (int i=0; i<ct.inputs.size(); i++) {
		// 	log("$mem_v2 inputs : %s\n", ct.inputs.element(i));
		// }
		

		design->sort();

		if (top_module_name.empty()) {
			run_pass("hierarchy -check -auto-top", design);
		} else {
			run_pass("hierarchy -check -top " + top_module_name, design);
		}

		if(!flag_no_pass) {
			run_pass("proc; opt;");
			run_pass("fsm; opt;");
			run_pass("memory_collect; memory_dff; opt;");
			run_pass("autoname");
			run_pass("check");
			run_pass("flatten");
			run_pass("pmuxtree");
			run_pass("wreduce");
			run_pass("opt -undriven -full; opt_muxtree; opt_expr -mux_undef -mux_bool -fine;;;");
			run_pass("autoname");
			run_pass("stat");
		}

		
/*
		for(auto module : design->modules()) 
		{

			log("%s\n", module->name.c_str());
			auto &conns = module->connections();
			for(auto sigsig : conns)
			{
				log("----------------------------------------\n");
				auto s1 = sigsig.first;
				auto s2 = sigsig.second;
				log("%s|%s\n", log_signal(s1), log_signal(s2));
				for(int i=0; i<s1.size(); i++)
				{
					auto sb_1 = s1[i];
					auto w1 = s1[i].wire;
					log("sigbit_1[%d]: %s | wire: %s %d %d %d %d %d %d %d\n", i, str(sb_1).c_str(), log_id(w1->name), w1->width, w1->start_offset, w1->port_id, w1->port_input, w1->port_output, w1->upto, w1->is_signed);
				}

				for(int i=0; i<s2.size(); i++)
				{
					auto sb_2 = s2[i];
					auto w2 = s2[i].wire;
					log("sigbit_2[%d]: %s | wire: %s %d %d %d %d %d %d %d\n", i, str(sb_2).c_str(), log_id(w2->name), w2->width, w2->start_offset, w2->port_id, w2->port_input, w2->port_output, w2->upto, w2->is_signed);
				}
				

				int len1 = s1.size();
				int len2 = s2.size();
				log("%d|%d\n", len1, len2);

			}

			for (const auto &port_name : module->ports) {
				log("port in module: %s\n",log_id(port_name));
			}

			log("cells:\n");
			
			for(auto cell : module->cells())
			{
				if (cell->hasPort(ID::A)) {
					auto port_A = cell->getPort(ID::A);
					for(int i=0; i<port_A.size(); i++)
					{
						auto pb_a = port_A[i];
						auto w1 = pb_a.wire;
						log("port_A[%s].sigbit_A[%d]: %s | wire: %s %d %d %d %d %d %d %d\n", log_signal(port_A), i, str(pb_a).c_str(), log_id(w1->name), w1->width, w1->start_offset, w1->port_id, w1->port_input, w1->port_output, w1->upto, w1->is_signed);
					}
				}
				if (cell->hasPort(ID::B)) {
					auto port_B = cell->getPort(ID::B);
					for(int i=0; i<port_B.size(); i++)
					{
						auto pb_b = port_B[i];
						auto w1 = pb_b.wire;
						log("port_B[%s].sigbit_B[%d]: %s | wire: %s %d %d %d %d %d %d %d\n", log_signal(port_B), i, str(pb_b).c_str(), log_id(w1->name), w1->width, w1->start_offset, w1->port_id, w1->port_input, w1->port_output, w1->upto, w1->is_signed);
					}
				}
				if (cell->hasPort(ID::Y)) {
					auto port_Y = cell->getPort(ID::Y);
					for(int i=0; i<port_Y.size(); i++)
					{
						auto pb_y = port_Y[i];
						auto w1 = pb_y.wire;
						log("port_Y[%s].sigbit_Y[%d]: %s | wire: %s %d %d %d %d %d %d %d\n", log_signal(port_Y), i, str(pb_y).c_str(), log_id(w1->name), w1->width, w1->start_offset, w1->port_id, w1->port_input, w1->port_output, w1->upto, w1->is_signed);
					}
				}
				log("\t%s of type %s\n", log_id(cell->name), log_id(cell->type));
				for (auto it : cell->connections()) {
					log("\t\t%s\n", log_id(it.first));
					auto s = it.second;
					for(int i=0; i<s.size(); i++)
					{
						auto sb = s[i];
						auto w = s[i].wire;
						log("sigbit[%d]: %s | wire: %s %d %d %d %d %d %d %d\n", i, str(sb).c_str(), log_id(w->name), w->width, w->start_offset, w->port_id, w->port_input, w->port_output, w->upto, w->is_signed);
					}
				}
			
			}
		}
*/		
		mixer = new HardSoftLogicMixer();
		set_default_config();

		/* Perform any initialization routines here */
    	find_hard_multipliers();
    	find_hard_adders();
    	//find_hard_adders_for_sub();
    	register_hard_blocks();

        /* Some initialization */
        one_string = vtr::strdup(ONE_VCC_CNS);
        zero_string = vtr::strdup(ZERO_GND_ZERO);
        pad_string = vtr::strdup(ZERO_PAD_ZERO);


		if (design->top_module()->processes.size() != 0)
			log_error("Found unmapped processes in top module %s: unmapped processes are not supported in odintechmap pass!\n", log_id(design->top_module()->name));

		netlist_t* transformed = to_netlist(design->top_module(), design);

		// configuration.output_netlist_graphs = true;
		// check_netlist(transformed);
		graphVizOutputNetlist(configuration.debug_output_path, "before", 1, transformed);

		printf("Elaborating the netlist created from the input BLIF file\n");
    	//blif_elaborate_top(transformed);
		        /* do the elaboration without any larger structures identified */
        depth_first_traversal_to_blif_elaborate(BLIF_ELABORATE_TRAVERSE_VALUE, transformed);

		// GenericWriter before_writer = GenericWriter();
		// before_writer._create_file("before.blif", _BLIF);
		// before_writer._write(transformed);

		//START ################# NETLIST OPTIMIZATION ############################

        /* point for all netlist optimizations. */
        printf("Performing Optimization on the Netlist\n");
        if (hard_multipliers) {
            /* Perform a splitting of the multipliers for hard block mults */
            reduce_operations(transformed, MULTIPLY);
            iterate_multipliers(transformed);
            clean_multipliers();
        }

        if (block_memories_info.read_only_memory_list || block_memories_info.block_memory_list) {
            /* Perform a hard block registration and splitting in width for Yosys generated memory blocks */
            iterate_block_memories(transformed);
            free_block_memories();
        }

        if (single_port_rams || dual_port_rams) {
            /* Perform a splitting of any hard block memories */
            iterate_memories(transformed);
            free_memory_lists();
        }

        if (hard_adders) {
            /* Perform a splitting of the adders for hard block add */
            reduce_operations(transformed, ADD);
            iterate_adders(transformed);
            clean_adders();

            /* Perform a splitting of the adders for hard block sub */
            reduce_operations(transformed, MINUS);
            iterate_adders_for_sub(transformed);
            clean_adders_for_sub();
        }

        //END ################# NETLIST OPTIMIZATION ############################


		// print_netlist_for_checking(transformed, "before");

		printf("Performing Partial Mapping on the Netlist\n");

		// GenericWriter middle_writer = GenericWriter();
		// middle_writer._create_file("middle.blif", _BLIF);
		// middle_writer._write(transformed);

		depth_first_traversal_to_partial_map(PARTIAL_MAP_TRAVERSE_VALUE, transformed);
		mixer->perform_optimizations(transformed);

        /* Find any unused logic in the netlist and remove it */
        remove_unused_logic(transformed);

		// check_netlist(transformed);
		graphVizOutputNetlist(configuration.debug_output_path, "after", 1, transformed);

		// print_netlist_for_checking(transformed, "after");

		printf("Writing Netlist to BLIF file\n");

		GenericWriter after_writer = GenericWriter();
		after_writer._create_file("after.blif", _BLIF);
		after_writer._write(transformed);

		compute_statistics(transformed, true);

		free_netlist(transformed);
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