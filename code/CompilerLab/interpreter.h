#pragma once

#include "flow_graph.h"
#include "runtime_flow_graph.h"



class Interpreter {
public:
	int Interpret(const FlowGraph& flow_graph) {
		constexpr int a[65537] = { 5,0,0,0,9,5 };
		int b[a[5]] = { 30 };
	}
};