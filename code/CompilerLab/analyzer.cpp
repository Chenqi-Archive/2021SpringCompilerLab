#include "analyzer.h"

int a = { {1} };


LocalVarInitializerList Analyzer::EvalLocalVarInitializerList(const vector<uint>& array_dimension, const Initializer& initializer) {
	// int
	if (array_dimension.empty()) {
		// initializer should be a single exp_tree after being expanded
		ref_ptr<const Initializer> current_initializer = &initializer;
		while (!current_initializer->IsExpression()) {
			if (current_initializer->initializer_list.size() != 1) { break; }
			current_initializer = &current_initializer->initializer_list[0];
		}
		if (current_initializer->IsExpression()) {

		}
		if (exp_tree == nullptr) { throw compile_error("too many initializer values"); }
		return LocalVarInitializerList(1, std::make_pair(0, initializer.expression));
	}

	// int[]...
	if (initializer.IsExpression()) {
		throw compile_error("initialization with {...} expected for array");
	}


}
