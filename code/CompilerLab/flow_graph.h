#pragma once

#include "core.h"

#include <vector>
#include <memory>


using std::vector;
using std::unique_ptr;
using std::shared_ptr;


enum class NodeType {

};


struct ABSTRACT_BASE Node_Base {

};


struct Flow {
	shared_ptr<Node_Base> target;
	ushort parameter_index;
	bool type;  // true:int, false:void
};

using FlowGroup = vector<Flow>;


struct Node_Var : Node_Base {
	FlowGroup output;
};


struct Node_Branch : Node_Base {
	vector<FlowGroup> branch_list;  // size==2
};


struct FlowGraph {
	vector<Node_Var> parameters;
    shared_ptr<Node_Var> ret;
};


struct Node_Func {
	ref_ptr<FlowGraph> flow_graph;
	FlowGroup output;
};