#pragma once

#include "linear_code.h"
#include "library_function.h"


#include <deque>
#include <vector>


using std::deque;
using std::vector;


class LinearCodeInterpreter {
private:
	void CallLibraryFunc() {

	}


private:
	uint FindLabel(uint label_index) {

	}

	void JumpToLabel(uint label_index) {

	}

private:
	deque<int> var_stack;
	uint frame_pointer;

private:
	void ExecuteCodeLine(const CodeLine& code_line) {

	}
	void ExecuteCodeBlock(const CodeBlock& code_block) {

	}
	void CallFunc(uint func_index) {
		assert(func_index < global_func->size());
		auto& func_def = global_func->operator[](func_index);

		vector<int> local_var(func_def.local_var_length, 0xCCCCCCCC);
		ref_ptr<vector<int>> old_local_var = this->local_var;
		this->local_var = &local_var;
		ExecuteCodeBlock(func_def.code_block);
		this->local_var = old_local_var;
	}

private:
	ref_ptr<const GlobalFuncTable> global_func = nullptr;
private:
	void InitializeGlobalVar(const GlobalVarTable& global_var_table) {
		global_var.assign(global_var_table.length, 0);
		for (auto [index, value] : global_var_table.initializing_list) {
			assert(index < global_var_table.length);
			global_var[index] = value;
		}
	}
	void InitializeFuncTable(const GlobalFuncTable& global_func_table) {
		global_func = &global_func_table;
	}
public:
	int ExecuteLinearCode(const LinearCode& linear_code) {
		InitializeGlobalVar(linear_code.global_var_table);
		InitializeFuncTable(linear_code.global_func_table);
		CallFunc(linear_code.main_func_index);
	}
};