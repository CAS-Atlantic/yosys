/**
 * TODO: these are unused in the code. is this functional and/or is it ripe to remove?
 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "odin_types.h"

#include "netlist_utils.h"
#include "node_creation_library.h"
#include "odin_util.h"

void function_to_print_node_and_its_pin(npin_t * temp_pin);
void function_to_print_node_and_its_pin(npin_t * temp_pin)
{
	int i;
	nnode_t *node;
	npin_t *pin;

	printf("\n-------Printing the related net driver pin info---------\n");
	node=temp_pin->node;

  	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];
	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  	}
  	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];
	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
 	}

}
void print_netlist_for_checking (netlist_t *netlist, char *name)
{
  int i,j,k;
  npin_t * pin;
  nnet_t * net;
  nnode_t * node;

  printf("printing the netlist : %s\n",name);
  /* gnd_node */
  node=netlist->gnd_node;
  net=netlist->zero_net;
  printf("--------gnd_node-------\n");
  printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
  printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
  for(i=0;i<node->num_input_port_sizes;i++)
  {
	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
  }
  for(i=0;i<node->num_input_pins;i++)
  {
	pin=node->input_pins[i];
	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);

  }
  printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
  for(i=0;i<node->num_output_port_sizes;i++)
  {
	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
  }
  for(i=0;i<node->num_output_pins;i++)
  {
	pin=node->output_pins[i];
	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node : %s",pin->net->name,pin->node->name);
  }

  printf("\n----------zero net----------\n");
  printf("unique_id : %ld name: %s combined: %ld \n",net->unique_id,net->name,net->combined);
//   printf("driver_pin name : %s num_fanout_pins: %ld\n",net->driver_pin->name,net->num_fanout_pins);
  for(i=0;i<net->num_fanout_pins;i++)
  {
    printf("fanout_pins %ld : %s",i,net->fanout_pins[i]->name);
  }

   /* vcc_node */
  node=netlist->vcc_node;
  net=netlist->one_net;
  printf("\n--------vcc_node-------\n");
  printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
  printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
  for(i=0;i<node->num_input_port_sizes;i++)
  {
	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
  }
  for(i=0;i<node->num_input_pins;i++)
  {
	pin=node->input_pins[i];
	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  }
  printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
  for(i=0;i<node->num_output_port_sizes;i++)
  {
	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
  }
  for(i=0;i<node->num_output_pins;i++)
  {
	pin=node->output_pins[i];
	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  }

  printf("\n----------one net----------\n");
  printf("unique_id : %ld name: %s combined: %ld \n",net->unique_id,net->name,net->combined);
//   printf("driver_pin name : %s num_fanout_pins: %ld\n",net->driver_pin->name,net->num_fanout_pins);
  for(i=0;i<net->num_fanout_pins;i++)
  {
    printf("fanout_pins %ld : %s",i,net->fanout_pins[i]->name);
  }

  /* pad_node */
  node=netlist->pad_node;
  net=netlist->pad_net;
  printf("\n--------pad_node-------\n");
  printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
  printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
  for(i=0;i<node->num_input_port_sizes;i++)
  {
	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
  }
  for(i=0;i<node->num_input_pins;i++)
  {
	pin=node->input_pins[i];
	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  }
  printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
  for(i=0;i<node->num_output_port_sizes;i++)
  {
	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
  }
  for(i=0;i<node->num_output_pins;i++)
  {
	pin=node->output_pins[i];
	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  }

  printf("\n----------pad net----------\n");
  printf("unique_id : %ld name: %s combined: %ld \n",net->unique_id,net->name,net->combined);
//   printf("driver_pin name : %s num_fanout_pins: %ld\n",net->driver_pin->name,net->num_fanout_pins);
  for(i=0;i<net->num_fanout_pins;i++)
  {
    printf("fanout_pins %ld : %s",i,net->fanout_pins[i]->name);
  }

  /* top input nodes */
  printf("\n--------------Printing the top input nodes--------------------------- \n");
  printf("num_top_input_nodes: %ld",netlist->num_top_input_nodes);
  for(j=0;j<netlist->num_top_input_nodes;j++)
  {
  	node=netlist->top_input_nodes[j];
	printf("\ttop input nodes : %ld\n",j);
  	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];
	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  	}
  	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];
	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
 	}
  }

  /* top output nodes */
  printf("\n--------------Printing the top output nodes--------------------------- \n");
  printf("num_top_output_nodes: %ld",netlist->num_top_output_nodes);
  for(j=0;j<netlist->num_top_output_nodes;j++)
  {
  	node=netlist->top_output_nodes[j];
	printf("\ttop output nodes : %ld\n",j);
  	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];
	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
		net=pin->net;
		printf("\n\t-------printing the related net info-----\n");
		printf("unique_id : %ld name: %s combined: %ld \n",net->unique_id,net->name,net->combined);
//   printf("driver_pin name : %s num_fanout_pins: %ld\n",net->driver_pin->name,net->num_fanout_pins);
  		for(k=0;k<net->num_fanout_pins;k++)
  		{
    		printf("fanout_pins %ld : %s",k,net->fanout_pins[k]->name);
  		}
		//  function_to_print_node_and_its_pin(net->driver_pin);
  	}
  	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];
	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
 	}

  }

  /* internal nodes */
  printf("\n--------------Printing the internal nodes--------------------------- \n");
  printf("num_internal_nodes: %ld",netlist->num_internal_nodes);
  for(j=0;j<netlist->num_internal_nodes;j++)
  {
  	node=netlist->internal_nodes[j];
	printf("\tinternal nodes : %ld\n",j);
  	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];
	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  	}
  	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];
	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
 	}
  }

  /* ff nodes */
  printf("\n--------------Printing the ff nodes--------------------------- \n");
  printf("num_ff_nodes: %ld",netlist->num_ff_nodes);
  for(j=0;j<netlist->num_ff_nodes;j++)
  {
  	node=netlist->ff_nodes[j];
	printf("\tff nodes : %ld\n",j);
  	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];
	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  	}
  	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];
	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s\n",pin->net->name,pin->node->name);
 	}
  }
}
