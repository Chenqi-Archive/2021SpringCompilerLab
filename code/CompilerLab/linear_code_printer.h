#pragma once

#include "linear_code.h"

#include <iostream>


using std::cout;
using std::endl;


static std::ostream& operator<<(std::ostream& os, std::pair<VarType, int> var_info) {
	switch (var_info.first) {
	case VarType::Global: return os << "g" << var_info.second;
	case VarType::Local: return os << "l" << var_info.second;
	case VarType::Number: return os << var_info.second;
	default: assert(false); return os;
	}
}


class CodeLinePrinter {
private:
	static std::pair<VarType, int> VarInfo(const CodeLine& line, int index) {
		assert(index >= 0 && index < 3);
		return { line.var_type[index],  line.var[index] };
	}
public:
	static void PrintCodeLine(const CodeLine& line) {
		switch (line.type) {
		case LineType::Operation:
			line.var_type[2] == VarType::Empty ?
				cout << '\t' << VarInfo(line, 0) << " = " << GetOperatorString(line.op) << VarInfo(line, 1) << endl:
				cout << '\t' << VarInfo(line, 0) << " = " << VarInfo(line, 1) << " " << GetOperatorString(line.op) << " " << VarInfo(line, 2) << endl;
			break;
		case LineType::Load:
			cout << "\t" << VarInfo(line, 0) << " = " << VarInfo(line, 1) << "[" << VarInfo(line, 2) << "]" << endl;
			break;
		case LineType::Store:
			cout << "\t" << VarInfo(line, 1) << "[" << VarInfo(line, 2) << "]" << " = " << VarInfo(line, 0) << endl;
			break;
		case LineType::Parameter:
			cout << "\tparam " << VarInfo(line, 0) << endl;
			break;
		case LineType::FuncCall:
			line.var_type[1] == VarType::Empty ? cout : cout << VarInfo(line, 1) << " = ";
			cout << "\tcall f" << line.var[0] << endl;
			break;
		case LineType::Label:
			cout << "label" << line.var[0] << ":" << endl;
			break;
		case LineType::JumpIf:
			cout << "\tif " << VarInfo(line, 1) << GetOperatorString(line.op) << VarInfo(line, 2) << " goto label" << line.var[0] << endl; 
			break;
		case LineType::Goto:
			cout << "\tgoto label" << line.var[0] << endl;
			break;
		case LineType::Return:
			cout << "\treturn"; 
			line.var_type[0] == VarType::Empty ? cout << endl : cout << VarInfo(line, 0) << endl;
			break;
		default:
			assert(false);
			return;
		}
	}
};


class LinearCodePrinter {
private:
	void PrintGlobalVar(const GlobalVarTable& global_var_table) {
		uint counter = 0;
		for (auto& global_var : global_var_table) {
			cout << "g" << counter++ << ": " << global_var.length << endl;
			for (auto& [index, val] : global_var.initializing_list) {
				cout << "\t[" << index << "] " << val << endl;
			}
		}
	}
	void PrintGlobalFunc(const GlobalFuncTable& global_func_table) {
		uint counter = 0;
		for (auto& global_func : global_func_table) {
			cout << "func" << counter++ << ": " << global_func.parameter_count << " " << global_func.local_var_count << endl;
			for (auto& line : global_func.code_block) {
				CodeLinePrinter::PrintCodeLine(line);
			}
		}
	}
public:
	void PrintLinearCode(const LinearCode& linear_code) {
		PrintGlobalVar(linear_code.global_var_table);
		PrintGlobalFunc(linear_code.global_func_table);
	}
};