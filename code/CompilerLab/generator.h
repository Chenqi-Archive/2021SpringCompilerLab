#pragma once

#include "linear_code.h"
#include "target_code.h"
#include "library_function.h"

#include <ostream>
#include <map>


using std::endl;


inline std::ostream& operator<<(std::ostream& os, Register reg) {
	if (reg.index <= 0) { return os << "zero"; }
	if (reg.index <= 1) { return os << "ra"; }
	if (reg.index <= 2) { return os << "sp"; }
	if (reg.index <= 3) { assert(false); return os << "gp"; }
	if (reg.index <= 4) { assert(false); return os << "tp"; }
	if (reg.index <= 7) { return os << "t" << reg.index - 5; }
	if (reg.index <= 9) { return os << "s" << reg.index - 8; }
	if (reg.index <= 17) { return os << "a" << reg.index - 10; }
	if (reg.index <= 27) { return os << "s" << reg.index - 16; }
	if (reg.index <= 31) { return os << "t" << reg.index - 25; }
	assert(false); return os;
}


class Generator {
private:
	std::ostream& out;

public:
	Generator(std::ostream& out) : out(out) {}

private:
	static constexpr uint max_imm12_uint_value = 2047;
	static constexpr int max_imm12_int_value = 2047;
	static constexpr int min_imm12_int_value = -2048;
	static constexpr char global_var_base_label[2] = "g";

private:
	Register t0 = Register::Temp(0);
	Register t1 = Register::Temp(1);
	Register t2 = Register::Temp(2);
	Register sp = Register::StackPointer();
	Register ra = Register::ReturnAddr();
	Register a0 = Register::ReturnValue();

private:
	void BinaryOpReg(Register reg_dest, OperatorType op, Register reg_src1, Register reg_src2) {
		switch (op) {
		case OperatorType::Add: 
			out << "add " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl; 
			break;
		case OperatorType::Sub: 
			out << "sub " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl; 
			break;
		case OperatorType::Mul: 
			out << "mul " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl; 
			break;
		case OperatorType::Div: 
			out << "div " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl; 
			break;
		case OperatorType::Mod: 
			out << "rem " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl; 
			break;
		case OperatorType::And: 
			out << "snez " << reg_dest << ", " << reg_src1 << endl;
			out << "snez " << t0 << ", " << reg_src2 << endl;
			out << "and " << reg_dest << ", " << reg_dest << ", " << t0 << endl;
			break;
		case OperatorType::Or: 
			out << "or " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl;
			out << "snez " << reg_dest << ", " << reg_dest << endl;
			break;
		case OperatorType::Equal:
			out << "xor " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl;
			out << "snez " << reg_dest << ", " << reg_dest << endl;
			break;
		case OperatorType::NotEqual:
			out << "xor " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl;
			out << "seqz " << reg_dest << ", " << reg_dest << endl;
			break;
		case OperatorType::Less:
			out << "slt " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl; 
			break;
		case OperatorType::Greater: 
			out << "sgt " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl; 
			break;
		case OperatorType::LessEqual: 
			out << "sgt " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl;
			out << "seqz " << reg_dest << ", " << reg_dest << endl;
			break;
		case OperatorType::GreaterEuqal:
			out << "slt " << reg_dest << ", " << reg_src1 << ", " << reg_src2 << endl;
			out << "seqz " << reg_dest << ", " << reg_dest << endl;
			break;
		default: assert(false); break;
		}
	}
	void UnaryOpReg(Register reg_dest, OperatorType op, Register reg_src) {
		switch (op) {
		case OperatorType::Add: 
			out << "mv " << reg_dest << ", " << reg_src << endl;
			break;
		case OperatorType::Sub: 			
			out << "neg " << reg_dest << ", " << reg_src << endl;
			break;
		case OperatorType::Not: 
			out << "seqz " << reg_dest << ", " << reg_src << endl;
			break;
		default: assert(false); break;
		}
	}
	void AddReg(Register reg_dest, Register reg_src1, Register reg_src2) {
		return BinaryOpReg(reg_dest, OperatorType::Add, reg_src1, reg_src2);
	}
	void AddRegNumber(Register reg_dest, Register reg_src1, int value) {
		if (value >= min_imm12_int_value && value <= max_imm12_int_value) {
			out << "addi " << reg_dest << ", " << reg_src1 << ", " << value << endl;
		} else {
			out << "lui " << reg_dest << ", %hi(" << value << ")" << endl;
			out << "addi " << reg_dest << ", " << reg_src1 << ", %lo(" << value << ")" << endl;
		}
	}
private:
	void LoadValueNumber(Register reg, int value) {
		out << "li " << reg << ", " << value << endl;
	}
	void LoadValueGlobalVar(Register reg, uint offset) {
		out << "lui " << reg << ", %hi(g + " << offset << ")" << endl;
		out << "lw " << reg << ", %lo(g + " << offset << ")(" << reg << ")" << endl;
	}
	void LoadValueLocalVar(Register reg, uint offset) {
		if (offset <= max_imm12_uint_value) {
			out << "lw " << reg << ", " << offset << "(" << sp << ")" << endl;
		} else {
			out << "lui " << t0 << ", %hi(" << offset << ")" << endl;
			out << "add " << t0 << ", " << t0 << ", " << sp << endl;
			out << "lw " << reg << ", %lo(" << offset << ")(" << t0 << ")" << endl;
		}
	}
	void StoreValueGlobalVar(uint offset, Register reg) {
		out << "lui " << t0 << ", %hi(g + " << offset << ")" << endl;
		out << "sw " << reg << ", %lo(g + " << offset << ")(" << t0 << ")" << endl;
	}
	void StoreValueLocalVar(uint offset, Register reg) {
		if (offset <= max_imm12_uint_value) {
			out << "sw " << reg << ", " << offset << "(" << sp << ")" << endl;
		} else {
			out << "lui " << t0 << ", %hi(" << offset << ")" << endl;
			out << "add " << t0 << ", " << t0 << ", " << sp << endl;
			out << "sw " << reg << ", %lo(" << offset << ")(" << t0 << ")" << endl;
		}
	}
private:
	void LoadAddrGlobalVar(Register reg, uint offset) {
		out << "lui " << reg << ", %hi(g + " << offset << ")" << endl;
		out << "addi " << reg << ", " << reg << ", %lo(g + " << offset << ")" << endl;
	}
	void LoadAddrLocalVar(Register reg, uint offset) {
		if (offset <= max_imm12_uint_value) {
			out << "addi " << reg << ", " << sp << ", " << offset << endl;
		} else {
			out << "lui " << t0 << ", %hi(" << offset << ")" << endl;
			out << "add " << t0 << ", " << t0 << ", " << sp << endl;
			out << "addi " << t0 << ", " << t0 << ", %lo(" << offset << ")" << endl;
		}
	}
private:
	void LoadValueGlobalAddr(Register reg, Register reg_addr) {
		out << "lw " << reg << ", 0(" << reg_addr << ")" << endl;
	}
	void StoreValueGlobalAddr(Register reg_addr, Register reg) {
		out << "sw " << reg << ", 0(" << reg_addr << ")" << endl;
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
	uint GetVarOffset(uint var_index) { return var_index * 4; }
private:
	void LoadValueVar(Register reg, VarInfo var) {
		assert(var.IsIntOrRef());
		switch (var.type) {
		case VarType::Local: return LoadValueLocalVar(reg, GetVarOffset(var.value));
		case VarType::Global: return LoadValueGlobalVar(reg, GetVarOffset(var.value));
		case VarType::Number: return LoadValueNumber(reg, var.value);
		default: assert(false); return;
		}
	}
	void StoreValueVar(VarInfo var, Register reg) {
		assert(var.IsRef());
		switch (var.type) {
		case VarType::Local: return StoreValueLocalVar(GetVarOffset(var.value), reg);
		case VarType::Global: return StoreValueLocalVar(GetVarOffset(var.value), reg);
		default: assert(false); return;
		}
	}
	void LoadAddrVar(Register reg, VarInfo var) {
		assert(var.IsRefOrAddr());
		switch (var.type) {
		case VarType::Local: return LoadAddrLocalVar(reg, GetVarOffset(var.value));
		case VarType::Global: return LoadAddrGlobalVar(reg, GetVarOffset(var.value));
		case VarType::Addr: return LoadValueLocalVar(reg, GetVarOffset(var.value));
		default: assert(false); return;
		}
	}
	void StoreAddrVar(VarInfo var, Register reg) {
		assert(var.IsAddr());
		switch (var.type) {
		case VarType::Addr: return StoreValueLocalVar(GetVarOffset(var.value), reg);
		default: assert(false); return;
		}
	}
	void LoadValueParameter(Register reg, VarInfo var) {
		assert(var.IsValid());
		switch (var.type) {
		case VarType::Addr:
		case VarType::Local: return LoadValueLocalVar(reg, GetVarOffset(var.value));
		case VarType::Global: return LoadValueGlobalVar(reg, GetVarOffset(var.value));
		case VarType::Number: return LoadValueNumber(reg, var.value);
		default: assert(false); return;
		}
	}

private:
	ref_ptr<const GlobalFuncTable> global_func = nullptr;
	uint main_func_index = -1;
	uint current_func_index = -1;
private:
	std::map<uint, uint> label_map;
	uint label_index_base = 0;
private:
	void InitializeLabelMap(const LabelMap& label_line_map) {
		label_map.clear();
		for (uint i = 0; i < label_line_map.size(); ++i) {
			label_map.insert({ label_line_map[i], i });
		}
	}
private:
	uint LoadFuncParameter(const CodeBlock& code_block, uint line_no) {
		uint func_index = code_block[line_no].var[0];
		uint parameter_count;
		if (IsLibraryFunc(func_index)) {
			parameter_count = GetLibraryFuncParameterCount(func_index);
		} else {
			func_index -= library_func_number;
			assert(func_index < global_func->size());
			auto& func_def = global_func->operator[](func_index);
			assert(func_def.parameter_count <= func_def.local_var_length);
			parameter_count = func_def.parameter_count;
		}
		for (uint i = 0; i < parameter_count; ++i) {
			line_no++;
			const CodeLine& line = code_block[line_no];
			assert(line.type == CodeLineType::Parameter);
			LoadValueParameter(Register::Argument(i), VarInfo(line, 0));
		}
		return line_no;
	}
	void ReadFuncCall(const CodeBlock& code_block, uint& line_no) {
		assert(line_no < code_block.size());
		uint next_line_no = LoadFuncParameter(code_block, line_no);
		uint func_index = code_block[line_no].var[0];
		out << "call ";
		IsLibraryFunc(func_index) ? out << GetLibraryFuncString(func_index) :
			(func_index == main_func_index ? out << "main" : out << "f" << func_index);
		out << endl;
		const CodeLine& line = code_block[line_no];
		if (line.var_type[1] != CodeLineVarType::Type::Empty) {
			StoreValueVar(VarInfo(line, 1), a0);
		}
		line_no = next_line_no;
	}
	void ReadBranch(uint label_index, OperatorType op, Register rs1, Register rs2) {
		label_index += label_index_base;
		switch (op) {
		case OperatorType::Equal:
			out << "beq " << rs1 << ", " << rs2 << ", .l" << label_index << endl;
			break;
		case OperatorType::NotEqual:
			out << "bne " << rs1 << ", " << rs2 << ", .l" << label_index << endl;
			break;
		case OperatorType::Less:
			out << "blt " << rs1 << ", " << rs2 << ", .l" << label_index << endl;
			break;
		case OperatorType::Greater:
			out << "bgt " << rs1 << ", " << rs2 << ", .l" << label_index << endl;
			break;
		case OperatorType::LessEqual:
			out << "ble " << rs1 << ", " << rs2 << ", .l" << label_index << endl;
			break;
		case OperatorType::GreaterEuqal:
			out << "bge " << rs1 << ", " << rs2 << ", .l" << label_index << endl;
			break;
		default: assert(false); break;
		}
	}
private:
	void ReadCodeLine(const CodeBlock& code_block, uint& line_no) {
		assert(line_no < code_block.size());
		const CodeLine& line = code_block[line_no];
		switch (line.type) {
		case CodeLineType::BinaryOp:
			LoadValueVar(t1, VarInfo(line, 1));
			LoadValueVar(t2, VarInfo(line, 2));
			BinaryOpReg(t1, line.op, t1, t2);
			StoreValueVar(VarInfo(line, 0), t1);
			break;
		case CodeLineType::UnaryOp:
			LoadValueVar(t1, VarInfo(line, 1));
			UnaryOpReg(t1, line.op, t1);
			StoreValueVar(VarInfo(line, 0), t1);
			break;
		case CodeLineType::Addr:
			LoadAddrVar(t1, VarInfo(line, 1));
			LoadValueVar(t2, VarInfo(line, 2));
			AddReg(t1, t1, t2);
			StoreAddrVar(VarInfo(line, 0), t1);
			break;
		case CodeLineType::Load:
			LoadAddrVar(t1, VarInfo(line, 1));
			LoadValueVar(t2, VarInfo(line, 2));
			AddReg(t1, t1, t2);
			LoadValueGlobalAddr(t1, t1);
			StoreValueVar(VarInfo(line, 0), t1);
			break;
		case CodeLineType::Store:
			LoadAddrVar(t1, VarInfo(line, 0));
			LoadValueVar(t2, VarInfo(line, 1));
			AddReg(t1, t1, t2);
			LoadValueVar(t2, VarInfo(line, 2));
			StoreValueGlobalAddr(t1, t2);
			break;
		case CodeLineType::FuncCall: 
			ReadFuncCall(code_block, line_no);
			break;
		case CodeLineType::JumpIf:
			LoadValueVar(t1, VarInfo(line, 1));
			LoadValueVar(t2, VarInfo(line, 2));
			ReadBranch(line.var[0], line.op, t1, t2);
			break;
		case CodeLineType::Goto:
			out << "j .l" << label_index_base + line.var[0];
			break;
		case CodeLineType::Return:
			if (line.var_type[0] != CodeLineVarType::Type::Empty) {
				LoadValueVar(Register::ReturnAddr(), VarInfo(line, 0));
			}
			out << "j .endf" << current_func_index;
			break;
		default:
			assert(false);
			return;
		}
		line_no++;
	}
	void ReadCodeBlock(const CodeBlock& code_block) {
		uint line_no = 0;
		for (auto [label_line, label_index] : label_map) {
			while (line_no < label_line) { 
				out << "\t";
				ReadCodeLine(code_block, line_no); 
			}
			out << ".l" << label_index_base + label_index;
		}
		while (line_no < code_block.size()) {
			out << "\t";
			ReadCodeLine(code_block, line_no);
		}
	}
	void ReadFuncDef(const GlobalFuncDef& func_def) {
		current_func_index == main_func_index ? out << "main:" << endl : out << "f" << current_func_index << ":" << endl;
		InitializeLabelMap(func_def.label_map);
		uint stack_size = (func_def.local_var_length + 1) * 4;
		out << "\t"; AddRegNumber(sp, sp, -stack_size);
		out << "\t"; StoreValueLocalVar(stack_size - 4, ra);
		for (uint i = 0; i < func_def.parameter_count; ++i) {
			out << "\t"; StoreValueLocalVar(i * 4, Register::Argument(i));
		}
		ReadCodeBlock(func_def.code_block);
		out << ".endf" << current_func_index << ":" << endl;
		out << "\t"; LoadValueLocalVar(ra, stack_size - 4);
		out << "\t"; AddRegNumber(sp, sp, stack_size);
		out << "\t"; out << "ret" << endl;
		label_index_base += func_def.label_map.size();
	}

private:
	void ReadFuncTable(const GlobalFuncTable& global_func_table) {
		uint func_index = library_func_number;
		for (auto& func_def : global_func_table) {
			current_func_index = func_index;
			ReadFuncDef(func_def);
		}
	}
	void ReadGlobalVar(const GlobalVarTable& global_var_table) {
		uint current_index = 0;
		out << "g:" << endl;
		for (auto [index, value] : global_var_table.initializing_list) {
			out << "\t" << endl;
			assert(index < global_var_table.length);
			if (current_index < index) {
				out << ".zero " << (index - current_index) * 4 << endl;
			}
			out << ".word " << value << endl;
			current_index = index + 1;
		}
		if (current_index < global_var_table.length) {
			out << ".zero " << (global_var_table.length - current_index) * 4 << endl;
		}
	}

public:
	void ReadLinearCode(const LinearCode& linear_code) {
		global_func = &linear_code.global_func_table;
		main_func_index = linear_code.main_func_index;
		label_index_base = 0;
		out << ".global main" << endl;
		ReadFuncTable(linear_code.global_func_table);
		ReadGlobalVar(linear_code.global_var_table);
	}
};