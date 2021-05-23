#pragma once

#include "linear_code.h"
#include "target_code.h"

#include <ostream>


class Generator {
private:
	TargetCode target_code;
private:
	void AppendInst(Instruction inst) { target_code.push_back(inst); }


	std::ostream& output;


private:
	uint reg_temp_bitset = 0;

private:
	Register AllocateTempRegister() {
		uint bit = 0;
		while (reg_temp_bitset & (1 << bit)) { bit++; assert(bit < 7); }
		reg_temp_bitset &= ~(1 << bit);
		return Register::Temp(bit);
	}
	void DeallocateTempRegister(Register reg) {
		uint bit = reg.AsTemp();
		assert(reg_temp_bitset & (1 << bit));
		reg_temp_bitset |= (1 << bit);
	}

private:
	void BinaryOpReg(Register reg, OperatorType op, Register reg_src1, Register reg_src2) {

	}
	void UnaryOpReg(Register reg, OperatorType op, Register reg_src1) {

	}

private:
	void LoadValueImmediateNumber(Register reg, int value) {

	}
	void LoadValueGlobalVar(Register reg, uint offset) {

	}
	void LoadValueLocalVar(Register reg, uint offset) {

	}
	void StoreValueGlobalVar(uint offset, Register reg) {

	}
	void StoreValueLocalVar(uint offset, Register reg) {

	}
private:
	Register LoadAddrOfLocalOffset(uint offset, Register reg_offset) {

	}
	Register LoadAddrOfGlobalOffset(uint offset, Register reg_offset) {

	}
private:
	void LoadParaImm(int value, Register reg_para) {

	}
	void LoadParaAtGlobalOffset(uint offset, Register reg_para) {

	}
	void LoadParaAtLocalOffset(uint offset, Register reg_para) {

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
		case VarType::Number: return LoadValueImmediateNumber(reg, var.value);
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
	Register GetVarAddr(VarInfo var, Register reg_offset) {
		assert(var.IsRefOrAddr());
		switch (var.type) {
		case VarType::Local: return LoadAddrOfLocalOffset(GetVarOffset(var.value), reg_offset);
		case VarType::Global: return LoadAddrOfGlobalOffset(GetVarOffset(var.value), reg_offset);
		case VarType::Addr: return BinaryOpReg(OperatorType::Add, LoadValueAtLocalOffset(var.value), reg_offset);
		default: assert(false); return Register::Zero();
		}
	}
	void StoreAddr(VarInfo var, Register reg) {
		assert(var.IsAddr());
		switch (var.type) {
		case VarType::Addr: return StoreValueAtLocalOffset(GetVarOffset(var.value), reg);
		default: assert(false); return;
		}
	}
	void LoadValueOfParameter(VarInfo var, Register parameter_index) {
		assert(var.IsValid());
		switch (var.type) {
		case VarType::Addr:
		case VarType::Local: return LoadParaAtLocalOffset(GetVarOffset(var.value), parameter_index);
		case VarType::Global: return LoadParaAtGlobalOffset(GetVarOffset(var.value), parameter_index);
		case VarType::Number: return LoadParaImm(var.value, parameter_index);
		default: assert(false); return;
		}
	}

private:
	void ReadBinaryOp() {

	}
	void ReadUnaryOp() {

	}
	void ReadAddr() {

	}
	void ReadLoad() {

	}

private:
	void ReadCodeLine(const CodeLine& line) {
		switch (line.type) {
		case CodeLineType::BinaryOp: {
				Register rs1 = AllocateTempRegister(); LoadValueVar(rs1, VarInfo(line, 1));
				Register rs2 = AllocateTempRegister(); LoadValueVar(rs2, VarInfo(line, 2));
				Register rd = rs1;
				BinaryOpReg(rd, line.op, rs1, rs2);
				StoreValueVar(VarInfo(line, 0), rd);
				DeallocateTempRegister(rs1);
				DeallocateTempRegister(rs2);
			}break;
		case CodeLineType::UnaryOp: {
				Register rs1 = AllocateTempRegister(); LoadValueVar(rs1, VarInfo(line, 1));
				Register rd = rs1;
				UnaryOpReg(rd, line.op, rs1);
				StoreValueVar(VarInfo(line, 0), rd);
				DeallocateTempRegister(rs1);
			}break;
		case CodeLineType::Addr: {

			}
			StoreAddr(VarInfo(line, 0), GetVarAddr(VarInfo(line, 1), LoadVarValue(VarInfo(line, 2))));
			break;
		case CodeLineType::Load:
		#error unchecked
			StoreVarValue(VarInfo(line, 0), LoadValueAtGlobalOffset(GetVarAddr(VarInfo(line, 1), LoadVarValue(VarInfo(line, 2)))));
			break;
		case CodeLineType::Store:
			StoreValueAtGlobalOffset(GetVarAddr(VarInfo(line, 0), LoadVarValue(VarInfo(line, 1))), LoadVarValue(VarInfo(line, 2)));
			break;
		case CodeLineType::FuncCall:
			CallFunc(code_block, line_no);
			if (line.var_type[1] != CodeLineVarType::Type::Empty) {
				SetVarValue(VarInfo(line, 1), return_value);
			}
			break;
		case CodeLineType::JumpIf:
			if (EvalBinaryOperator(line.op, LoadVarValue(VarInfo(line, 1)), LoadVarValue(VarInfo(line, 2)))) {
				return ExecuteCodeLine(code_block, current_func_label_map->operator[](line.var[0]));
			} else {
				break;
			}
		case CodeLineType::Goto:
			return ExecuteCodeLine(code_block, current_func_label_map->operator[](line.var[0]));
		case CodeLineType::Return:
			if (line.var_type[0] != CodeLineVarType::Type::Empty) {
				return_value = LoadVarValue(VarInfo(line, 0));
			}
			return;
		default:
			assert(false);
			return;
		}
		return ExecuteCodeLine(code_block, line_no + 1);
	}

private:


public:
	TargetCode ReadLinearCode(const LinearCode& linear_code) {

	}
};