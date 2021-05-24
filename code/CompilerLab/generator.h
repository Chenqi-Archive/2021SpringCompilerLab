#pragma once

#include "linear_code.h"
#include "target_code.h"
#include "library_function.h"

#include <ostream>
#include <map>


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
	void BinaryOpReg(Register reg_dest, OperatorType op, Register reg_src1, Register reg_src2);
	void UnaryOpReg(Register reg_dest, OperatorType op, Register reg_src);
	void AddReg(Register reg_dest, Register reg_src1, Register reg_src2);
	void AddRegNumber(Register reg_dest, Register reg_src, int value);
	void ShiftLeftRegNumber(Register reg_dest, Register reg_src, uint value);
private:
	void LoadValueNumber(Register reg, int value);
	void LoadValueGlobalVar(Register reg, uint offset);
	void LoadValueLocalVar(Register reg, uint offset);
	void StoreValueGlobalVar(uint offset, Register reg);
	void StoreValueLocalVar(uint offset, Register reg);
private:
	void LoadAddrGlobalVar(Register reg, uint offset);
	void LoadAddrLocalVar(Register reg, uint offset);
private:
	void LoadValueGlobalAddr(Register reg, Register reg_addr);
	void StoreValueGlobalAddr(Register reg_addr, Register reg);

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
	void LoadValueVar(Register reg, VarInfo var);
	void StoreValueVar(VarInfo var, Register reg);
	void LoadAddrVar(Register reg, VarInfo var);
	void StoreAddrVar(VarInfo var, Register reg);
	void LoadValueParameter(Register reg, VarInfo var);

private:
	ref_ptr<const GlobalFuncTable> global_func = nullptr;
	uint main_func_index = -1;
	uint current_func_index = -1;
private:
	std::multimap<uint, uint> label_map;
	uint label_index_base = 0;
private:
	void InitializeLabelMap(const LabelMap& label_line_map);
private:
	uint LoadFuncParameter(const CodeBlock& code_block, uint line_no);
	void ReadFuncCall(const CodeBlock& code_block, uint& line_no);
	void ReadBranch(uint label_index, OperatorType op, Register rs1, Register rs2);
private:
	void ReadCodeLine(const CodeBlock& code_block, uint& line_no);
	void ReadCodeBlock(const CodeBlock& code_block);
	void ReadFuncDef(const GlobalFuncDef& func_def);

private:
	void ReadFuncTable(const GlobalFuncTable& global_func_table);
	void ReadGlobalVar(const GlobalVarTable& global_var_table);

public:
	void ReadLinearCode(const LinearCode& linear_code);
};