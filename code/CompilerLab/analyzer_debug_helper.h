#pragma once

#include "linear_code.h"
#include "symbol_table.h"

#include <iostream>


using std::cout;
using std::endl;


static std::ostream& operator<<(std::ostream& os, std::pair<VarType, int> var_info) {
	switch (var_info.first) {
	case VarType::Number: return os << var_info.second;
	case VarType::LocalTemp: return os << "t" << var_info.second;
	case VarType::Local: return os << "a" << var_info.second;
	case VarType::Global: return os << "g" << var_info.second;
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
		case CodeLineType::BinaryOp:
			cout << '\t' << VarInfo(line, 0) << " = " << VarInfo(line, 1) << " " << GetOperatorString(line.op) << " " << VarInfo(line, 2) << endl;
			break;
		case CodeLineType::UnaryOp:
			cout << '\t' << VarInfo(line, 0) << " = " << GetOperatorString(line.op) << VarInfo(line, 1) << endl;
			break;
		case CodeLineType::Load:
			cout << "\t" << VarInfo(line, 0) << " = " << VarInfo(line, 1) << "[" << VarInfo(line, 2) << "]" << endl;
			break;
		case CodeLineType::Store:
			cout << "\t" << VarInfo(line, 0) << "[" << VarInfo(line, 1) << "]" << " = " << VarInfo(line, 2) << endl;
			break;
		case CodeLineType::Parameter:
			cout << "\tparam " << VarInfo(line, 0) << endl;
			break;
		case CodeLineType::FuncCall:
			line.var_type[1] == VarType::Void ? cout << "\t" : cout << "\t" << VarInfo(line, 1) << " = ";
			if (IsLibraryFunc(line.var[0])) {
				cout << "call " << GetLibraryFuncString(line.var[0]) << endl;
			} else {
				cout << "call f" << line.var[0] << endl;
			}
			break;
		case CodeLineType::Label:
			cout << "  label" << line.var[0] << ":" << endl;
			break;
		case CodeLineType::JumpIf:
			cout << "\tif " << VarInfo(line, 1) << " " << GetOperatorString(line.op) << " " << VarInfo(line, 2) << " goto label" << line.var[0] << endl;
			break;
		case CodeLineType::Goto:
			cout << "\tgoto label" << line.var[0] << endl;
			break;
		case CodeLineType::Return:
			cout << "\treturn"; 
			line.var_type[0] == VarType::Void ? cout << endl : cout << " " << VarInfo(line, 0) << endl;
			break;
		default:
			assert(false);
			return;
		}
	}
};


class AnalyzerDebugHelper {
private:
	void PrintGlobalVar(const GlobalVarTable& global_var_table) {
		cout << global_var_table.length << endl;
		for (auto& [index, val] : global_var_table.initializing_list) {
			cout << "\t[" << index << "] " << val << endl;
		}
	}
	void PrintGlobalFunc(const GlobalFuncTable& global_func_table) {
		uint counter = 0;
		for (auto& global_func : global_func_table) {
			cout << "func" << library_func_number + counter++ << ": " << global_func.parameter_count << " " << global_func.local_var_length << endl;
			for (auto& line : global_func.code_block) {
				CodeLinePrinter::PrintCodeLine(line);
			}
		}
	}
public:
	void PrintLinearCode(const LinearCode& linear_code) {
		PrintGlobalVar(linear_code.global_var_table);
		PrintGlobalFunc(linear_code.global_func_table);
		cout << "main function is " << linear_code.main_func_index << endl;
	}
};