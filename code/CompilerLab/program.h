#pragma once

#include "flow_graph.h"


using GlobalVarInitializerList = vector<std::pair<uint, int>>;  // (index, val)

struct GlobalVarDef {
	uint length;
	GlobalVarInitializerList initializer_list;
};


struct GlobalFuncDef {

};


struct Program {
	vector<GlobalVarDef> global_var;
	vector<GlobalFuncDef> global_func;
	GlobalFuncDef func_main;
};