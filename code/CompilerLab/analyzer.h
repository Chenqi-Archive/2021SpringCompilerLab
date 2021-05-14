#pragma once

#include "syntax_tree.h"
#include "symbol_table.h"
#include "flow_graph.h"
#include "program.h"

constexpr int a[3] = 1;


class Analyzer {
private:
	// Symbol table for functions and variables.


private:
	int EvalConstExp(const ExpTree& exp_tree) {
		assert(exp_tree != nullptr);


	}
	LocalVarInitializerList EvalLocalVarInitializerList(const vector<uint>& array_dimension, const Initializer& initializer);
	GlobalVarInitializerList EvalGlobalVarInitializerList(const vector<uint>& array_dimension, const Initializer& initializer) {
		LocalVarInitializerList local_result = EvalLocalVarInitializerList(array_dimension, initializer);
		GlobalVarInitializerList global_result; global_result.reserve(local_result.size());
		try {
			for (auto local_item : local_result) {
				global_result.push_back(std::make_pair(local_item.first, EvalConstExp(local_item.second)));
			}
		} catch (const compile_error&) {
			throw compile_error("")
		}
		return global_result;
	}


private:
	ref_ptr<Node_Base> ReadExp(const ExpTree& exp_tree) {

		if(is_leaf)
		if(is_mid) {readexp(left), ReadExp(right) }
		find node in symbol table
			found : return the node;
			not fount: create a new node, connect the leaves, and put it in the symbol table.

	}

	void ReadLocalBlock(const Block& block) {

	}

private:
	void ReadGlobalVarDef(const AstNode_VarDef& var_def) {

	}
	void ReadGlobalFuncDef(const AstNode_FuncDef& func_def) {

	}
	Program ReadGlobalBlock(const Block& block) {
		constexpr int z[8][4][3] = { {4, {3}, 3, 0 }, {2}, 2, {1} };

	}

public:
	Program ReadSyntaxTree(const SyntaxTree& syntax_tree) {
		return ReadGlobalBlock(syntax_tree);
	}
};