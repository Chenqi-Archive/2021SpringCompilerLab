#pragma once

#include "syntax_tree.h"
#include "symbol_table.h"
#include "flow_graph.h"


class Analyzer {


private:
	int EvalConstExp(const ExpTree& exp_tree) {

	}


	void ReadGlobalVarDef(const AstNode_VarDef& var_def) {

	}

	void ReadGlobalFuncDef(const AstNode_FuncDef& func_def) {

	}

public:
	FlowGraph ReadSyntaxTree(const SyntaxTree& syntax_tree) {

	}
};