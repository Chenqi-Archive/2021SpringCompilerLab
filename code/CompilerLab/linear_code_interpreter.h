#pragma once

#include "linear_code.h"
#include "library_function.h"

#include <vector>


using std::vector;


class LinearCodeInterpreter {
private:
	static constexpr int global_var_initial_value = 0;
	static constexpr int local_var_initial_value = 0xCCCCCCCC;
	static constexpr uint max_stack_size = 65536;

private:
	vector<int> var_stack;
	int return_value = 0;
	uint frame_pointer = 0;
	uint current_func_frame_size = 0;
	ref_ptr<const LabelMap> current_func_label_map = nullptr;

private:
	void SetValueAtGlobalIndex(uint index, int value) {
		if (index >= var_stack.size()) { throw std::runtime_error("array subscript out of range"); }
		var_stack[index] = value;
	}
	void SetValueAtLocalIndex(uint index, int value) {
		SetValueAtGlobalIndex(frame_pointer + index, value);
	}
	int GetValueAtGlobalIndex(uint index) {
		if (index >= var_stack.size()) { throw std::runtime_error("array subscript out of range"); }
		return var_stack[index];
	}
	int GetValueAtLocalIndex(uint index) {
		return GetValueAtGlobalIndex(frame_pointer + index);
	}

private:
	using VarType = CodeLineVarType::Type;

	struct VarInfo : CodeLineVarType {
	public:
		const int value;
		VarInfo(const CodeLine& line, int index) : CodeLineVarType(GetVarInfo(line, index)), value(line.var[index]) {}
	private:
		static CodeLineVarType GetVarInfo(const CodeLine& line, int index) {
			assert(index >= 0 && index < 3);
			return line.var_type[index];
		}
	};

private:
	void SetVarValue(VarInfo var, int value) {
		assert(var.IsRef());
		switch (var.type) {
		case VarType::Local: return SetValueAtLocalIndex(var.value, value);
		case VarType::Global: return SetValueAtGlobalIndex(var.value, value);
		default: assert(false); return;
		}
	}
	int GetVarValue(VarInfo var) {
		assert(var.IsIntOrRef());
		switch (var.type) {
		case VarType::Local: return GetValueAtLocalIndex(var.value);
		case VarType::Global: return GetValueAtGlobalIndex(var.value);
		case VarType::Number: return var.value;
		default: assert(false); return 0;
		}
	}
	void SetAddr(VarInfo var, uint addr) {
		assert(var.IsAddr());
		switch (var.type) {
		case VarType::Addr: return SetValueAtLocalIndex(var.value, addr);
		default: assert(false); return;
		}
	}
	uint GetVarAddr(VarInfo var, int offset) {
		assert(var.IsRefOrAddr());
		switch (var.type) {
		case VarType::Local: return frame_pointer + var.value + offset;
		case VarType::Global: return var.value + offset;
		case VarType::Addr: return GetValueAtLocalIndex(var.value) + offset;
		default: assert(false); return 0;
		}
	}
	int GetParameterValue(VarInfo var) {
		assert(var.IsValid());
		switch (var.type) {
		case VarType::Addr:
		case VarType::Local: return GetValueAtLocalIndex(var.value);
		case VarType::Global: return GetValueAtGlobalIndex(var.value);
		case VarType::Number: return var.value;
		default: assert(false); return {};
		}
	}
	Argument GetLibraryFuncParameterValue(VarInfo var) {
		assert(var.IsValid());
		switch (var.type) {
		case VarType::Local: return Argument(GetValueAtLocalIndex(var.value));
		case VarType::Global: return Argument(GetValueAtGlobalIndex(var.value));
		case VarType::Number: return Argument(var.value);
		case VarType::Addr: {
				uint offset = GetValueAtLocalIndex(var.value);
				uint length = var_stack.size() > offset ? var_stack.size() - offset : 0;
				return Argument(var_stack.data() + offset, length);
			}
		default: assert(false); return {};
		}
	}

private:
	void ExecuteCodeLine(const CodeBlock& code_block, uint line_no) {
		if (line_no >= code_block.size()) { return; }
		const CodeLine& line = code_block[line_no];
		switch (line.type) {
		case CodeLineType::BinaryOp:
			SetVarValue(VarInfo(line, 0), EvalBinaryOperator(line.op, GetVarValue(VarInfo(line, 1)), GetVarValue(VarInfo(line, 2))));
			break;
		case CodeLineType::UnaryOp:
			SetVarValue(VarInfo(line, 0), EvalUnaryOperator(line.op, GetVarValue(VarInfo(line, 1))));
			break;
		case CodeLineType::Addr:
			SetAddr(VarInfo(line, 0), GetVarAddr(VarInfo(line, 1), GetVarValue(VarInfo(line, 2))));
			break;
		case CodeLineType::Load:
			SetVarValue(VarInfo(line, 0), GetValueAtGlobalIndex(GetVarAddr(VarInfo(line, 1), GetVarValue(VarInfo(line, 2)))));
			break;
		case CodeLineType::Store:
			SetValueAtGlobalIndex(GetVarAddr(VarInfo(line, 0), GetVarValue(VarInfo(line, 1))), GetVarValue(VarInfo(line, 2)));
			break;
		case CodeLineType::FuncCall:
			CallFunc(code_block, line_no);
			if (line.var_type[1] != CodeLineVarType::Type::Empty) {
				SetVarValue(VarInfo(line, 1), return_value);
			}
			break;
		case CodeLineType::JumpIf:
			if (EvalBinaryOperator(line.op, GetVarValue(VarInfo(line, 1)), GetVarValue(VarInfo(line, 2)))) {
				return ExecuteCodeLine(code_block, current_func_label_map->operator[](line.var[0]));
			} else {
				break;
			}
		case CodeLineType::Goto:
			return ExecuteCodeLine(code_block, current_func_label_map->operator[](line.var[0]));
		case CodeLineType::Return:
			if (line.var_type[0] != CodeLineVarType::Type::Empty) {
				return_value = GetVarValue(VarInfo(line, 0));
			}
			return;
		default:
			assert(false);
			return;
		}
		return ExecuteCodeLine(code_block, line_no + 1);
	}
	void ExecuteCodeBlock(const CodeBlock& code_block) {
		return ExecuteCodeLine(code_block, 0);
	}
	int ReadParameter(const CodeBlock& code_block, uint line_no) {
		const CodeLine& line = code_block[line_no];
		assert(line.type == CodeLineType::Parameter);
		return GetParameterValue(VarInfo(line, 0));
	}
	Argument ReadLibraryFuncParameter(const CodeBlock& code_block, uint line_no) {
		const CodeLine& line = code_block[line_no];
		assert(line.type == CodeLineType::Parameter);
		return GetLibraryFuncParameterValue(VarInfo(line, 0));
	}
	void CallFunc(const CodeBlock& code_block, uint& line_no) {
		assert(line_no < code_block.size());
		uint func_index = code_block[line_no].var[0];
		if (IsLibraryFunc(func_index)) {
			uint parameter_count = GetLibraryFuncParameterCount(func_index);
			vector<Argument> args; args.reserve(parameter_count);
			for(uint i = 0; i < parameter_count; ++i) {
				line_no++;
				args.push_back(ReadLibraryFuncParameter(code_block, line_no));
			}
			switch (parameter_count) {
			case 0: return CallLibraryFunc(func_index, Argument(), Argument(), return_value); 
			case 1: return CallLibraryFunc(func_index, args[0], Argument(), return_value);
			case 2: return CallLibraryFunc(func_index, args[0], args[1], return_value);
			default: assert(false); return;
			}
		} else {
			func_index -= library_func_number;
			assert(func_index < global_func->size());
			auto& func_def = global_func->operator[](func_index);
			assert(func_def.parameter_count <= func_def.local_var_length);
			if (var_stack.size() + func_def.local_var_length > max_stack_size) { throw std::runtime_error("stack overflow"); }
			uint old_func_frame_size = current_func_frame_size;
			ref_ptr<const LabelMap> old_func_label_map = current_func_label_map;
			current_func_label_map = &func_def.label_map;
			current_func_frame_size = func_def.local_var_length;
			var_stack.insert(var_stack.end(), current_func_frame_size, 0xCCCCCCCC);
			uint new_frame_pointer = frame_pointer + old_func_frame_size;
			for (uint i = 0; i < func_def.parameter_count; ++i) {
				line_no++;
				var_stack[new_frame_pointer + i] = ReadParameter(code_block, line_no);
			}
			frame_pointer = new_frame_pointer;
			ExecuteCodeBlock(func_def.code_block);
			var_stack.erase(var_stack.begin() + frame_pointer, var_stack.end());
			frame_pointer -= old_func_frame_size;
			current_func_frame_size = old_func_frame_size;
			current_func_label_map = old_func_label_map;
		}
	}
	void CallMainFunc(uint main_func_index) {
		main_func_index -= library_func_number;
		assert(main_func_index < global_func->size());
		auto& func_def = global_func->operator[](main_func_index);
		assert(func_def.parameter_count == 0);
		if (var_stack.size() + func_def.local_var_length > max_stack_size) { throw std::runtime_error("stack overflow"); }
		uint old_func_frame_size = current_func_frame_size;
		ref_ptr<const LabelMap> old_func_label_map = current_func_label_map;
		current_func_label_map = &func_def.label_map;
		current_func_frame_size = func_def.local_var_length;
		var_stack.insert(var_stack.end(), current_func_frame_size, local_var_initial_value);
		frame_pointer += old_func_frame_size;
		ExecuteCodeBlock(func_def.code_block);
		var_stack.erase(var_stack.begin() + frame_pointer, var_stack.end());
		frame_pointer -= old_func_frame_size;
		current_func_frame_size = old_func_frame_size;
		current_func_label_map = old_func_label_map;
	}

private:
	ref_ptr<const GlobalFuncTable> global_func = nullptr;
private:
	void InitializeGlobalVar(const GlobalVarTable& global_var_table) {
		var_stack.insert(var_stack.end(), global_var_table.length, global_var_initial_value);
		for (auto [index, value] : global_var_table.initializing_list) {
			assert(index < global_var_table.length);
			SetValueAtGlobalIndex(index, value);
		}
		current_func_frame_size = global_var_table.length;
		frame_pointer = 0;
	}
	void InitializeFuncTable(const GlobalFuncTable& global_func_table) {
		global_func = &global_func_table;
	}
public:
	int ExecuteLinearCode(const LinearCode& linear_code) {
		InitializeGlobalVar(linear_code.global_var_table);
		InitializeFuncTable(linear_code.global_func_table);
		LibraryInitialize();
		CallMainFunc(linear_code.main_func_index);
		LibraryUninitialize();
		return return_value;
	}
};