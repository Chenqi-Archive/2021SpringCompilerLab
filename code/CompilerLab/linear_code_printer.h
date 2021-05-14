#pragma once

#include "linear_code.h"

#include <iostream>


using std::cout;
using std::endl;


static std::ostream& operator<<(std::ostream& os, VarIndex var_index) {
	assert(var_index.IsValid());
	return os << (var_index.IsGlobal() ? "G" : "l") << var_index.GetIndex();
}


class LinearCodePrinter {
public:
	void PrintLinearCode(const LinearCode& linear_code) {
		uint counter;

		// print global var
		counter = 0;
		for (auto& global_var : linear_code.global_var_table) {
			cout << "G" << counter++ << ": " << global_var.length << endl;
			for (auto& [index, val] : global_var.initializer_list) {
				cout << "\t[" << index << "] " << val << endl;
			}
		}

		// print global func
		counter = 0;
		for (auto& global_func : linear_code.global_func_table) {
			cout << "Func" << counter++ << ": " << global_func.parameter_count << " " << global_func.local_var_count << endl;
			for (auto& line : global_func.code_block) {
				switch (line.type) 	{
				case LineType::Operation: 
					cout << "\t" << line.var0 << " = " << line.var1 << " " << GetOperatorString(line.op) << " " << line.var2 << endl; 
					break;
				case LineType::Load: 
					cout << "\t" << line.var0 << " = " << line.var1 << "[" << line.var2 << "]" << endl;
					break;
				case LineType::Store: 
					cout << "\t" << line.var1 << "[" << line.var2 << "]" << " = " << line.var0 << endl;
					break;
				case LineType::Parameter: 
					cout << "\tparam " << line.var0 << endl;
					break;
				case LineType::FuncCall: 
					cout << "\tcall f" << line.func << endl;
					break;
				case LineType::Label: 
					cout << "label" << line.label << ":" << endl;
					break;
				case LineType::JumpIf: 
					cout << "\tif " << line.var1 << " goto label" << line.label << endl; break;
				case LineType::Goto: 
					cout << "\tgoto label" << line.label << endl; 
					break;
				case LineType::Return: 
					cout << "\treturn"; line.var0.IsValid() ? cout << line.var0 << endl : cout << endl;
					break;
				default: 
					assert(false); 
					return;
				}
			}
		}
	}
};