#pragma once

#include "syntax_tree.h"
#include "symbol_table.h"
#include "reversion_wrapper.h"


class SemanticChecker {
private:
	FuncTable func_table;
	vector<VarTable> var_table_stack;

private:
	ref_ptr<Identifier_Base> FindIdentifier(string_view identifier) {
		for (auto& identifier_table : reverse(identifier_table_stack)) {
			if (auto it = identifier_table.find(identifier); it != identifier_table.end()) {
				return it->second.get();
			}
		}
		throw semantic_error(string("identifier \"") + string(identifier) + "\" is undefined");
	}

private:

	void CheckFuncDef(const Block& func_block) {

	}

	void CheckMain() {

	}

	void CheckGlobalScope(const Block& block) {

	}

public:
	void CheckSyntaxTree(const SyntaxTree& syntax_tree) {

	}


private:
	void EvalFunc() {}
	void EvalExp() {}


public:
	void Interpret() {

	}
};