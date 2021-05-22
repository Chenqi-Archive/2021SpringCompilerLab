#pragma once

#include "linear_code.h"
#include "library_function.h"

#include <iostream>


using std::cout;
using std::endl;


inline std::ostream& operator<<(std::ostream& os, std::pair<CodeLineVarType, int> var_info) {
	switch (var_info.first) {
	case CodeLineVarType::Type::Number: return os << var_info.second;
	case CodeLineVarType::Type::Local: return os << "t" << var_info.second;
	case CodeLineVarType::Type::Global: return os << "g" << var_info.second;
	case CodeLineVarType::Type::Addr: return os << "v" << var_info.second;
	default: assert(false); return os;
	}
}


class AnalyzerDebugHelper {
private:
	static std::pair<CodeLineVarType, int> VarInfo(const CodeLine& line, int index) {
		assert(index >= 0 && index < 3);
		return { line.var_type[index],  line.var[index] };
	}
public:
	void PrintCodeLine(const CodeLine& line) {
		switch (line.type) {
		case CodeLineType::BinaryOp:
			cout << VarInfo(line, 0) << " = " << VarInfo(line, 1) << " " << GetOperatorString(line.op) << " " << VarInfo(line, 2) << endl;
			break;
		case CodeLineType::UnaryOp:
			cout << VarInfo(line, 0) << " = " << GetOperatorString(line.op) << VarInfo(line, 1) << endl;
			break;
		case CodeLineType::Addr:
			cout << VarInfo(line, 0) << " = &" << VarInfo(line, 1) << "[" << VarInfo(line, 2) << "]" << endl;
			break;
		case CodeLineType::Load:
			cout << VarInfo(line, 0) << " = " << VarInfo(line, 1) << "[" << VarInfo(line, 2) << "]" << endl;
			break;
		case CodeLineType::Store:
			cout << VarInfo(line, 0) << "[" << VarInfo(line, 1) << "]" << " = " << VarInfo(line, 2) << endl;
			break;
		case CodeLineType::FuncCall:
			line.var_type[1] == CodeLineVarType::Type::Empty ? cout : cout << VarInfo(line, 1) << " = ";
			if (IsLibraryFunc(line.var[0])) {
				cout << "call " << GetLibraryFuncString(line.var[0]) << endl;
			} else {
				cout << "call f" << line.var[0] << endl;
			}
			break;
		case CodeLineType::Parameter:
			cout << "param " << VarInfo(line, 0) << endl;
			break;
		case CodeLineType::JumpIf:
			cout << "goto " << GetLabelLineNo(line.var[0]) << " if " << VarInfo(line, 1) << " " << GetOperatorString(line.op) << " " << VarInfo(line, 2) << endl;
			break;
		case CodeLineType::Goto:
			cout << "goto " << GetLabelLineNo(line.var[0]) << endl;
			break;
		case CodeLineType::Return:
			cout << "return";
			line.var_type[0] == CodeLineVarType::Type::Empty ? cout << endl : cout << " " << VarInfo(line, 0) << endl;
			break;
		default:
			assert(false);
			return;
		}
	}

private:
	ref_ptr<const LabelMap> current_func_label_map = nullptr;
	uint GetLabelLineNo(uint label_index) {
		assert(label_index < current_func_label_map->size());
		return current_func_label_map->operator[](label_index);
	}

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
			current_func_label_map = &global_func.label_map;
			for (uint i = 0; i < global_func.code_block.size(); ++i) {
				cout << i << '\t';
				PrintCodeLine(global_func.code_block[i]);
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