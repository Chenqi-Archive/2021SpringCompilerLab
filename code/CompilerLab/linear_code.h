#pragma once

#include "keyword.h"

#include <vector>


using std::vector;


enum class LineType : uchar {
	Operation,	//	op	x0	x1	x2
	Load,		//	ld	x0	x1	x2
	Store,		//	st	x0	x1	x2
	Parameter,	//	pm	x0
	FuncCall,	//	cl	f0  x1
	Label,		//	lb	l0
	JumpIf,		//	jo	l0	x1	x2
	Goto,		//	gt	l0
	Return,		//	rt	x0
};


enum class VarType : uchar {
	Empty,
	Global,
	Local,
	Number
};


struct VarInfo {
public:
	const VarType type;   // both VarType and array dimension are used as type info when checking function parameters
	const ushort length;  // should be array dimension, and there may be overflow with ushort
	const int value;
public:
	constexpr VarInfo() : type(VarType::Empty), length(1), value() {}
	explicit constexpr VarInfo(VarType type, int value) : type(type), length(1), value(value) {
		assert(type != VarType::Empty && type <= VarType::Number);
		if (type == VarType::Global || type == VarType::Local) { assert(value >= 0); }
	}
	explicit constexpr VarInfo(VarType type, ushort length, int value) : VarInfo(type, value) { length = length; }
	bool IsEmpty() const { return type == VarType::Empty; }
	bool IsNumber() const { return type == VarType::Number; }
};


struct CodeLine {
private: friend class CodeLinePrinter;
	LineType type : 4;
	OperatorType op : 4;
	VarType var_type[3];
	int var[3];

private:
	CodeLine(LineType type) : type(type), op(OperatorType::None), var_type(), var() {}

private:
	void SetVar(VarInfo var0, VarInfo var1, VarInfo var2) {
		var_type[0] = var0.type; var[0] = var0.value;
		var_type[1] = var1.type; var[1] = var1.value;
		var_type[2] = var2.type; var[2] = var2.value;
	}
	static CodeLine Operation(OperatorType op, VarInfo dest, VarInfo src1, VarInfo src2) {
		CodeLine code_line(LineType::Operation); 
		code_line.op = op;
		code_line.SetVar(dest, src1, src2);
		return code_line;
	}

public:
	static CodeLine BinaryOperation(OperatorType op, VarInfo dest, VarInfo src1, VarInfo src2) {
		assert(IsBinaryOperator(op));
		assert(dest.type != VarType::Number);
		return Operation(op, dest, src1, src2);
	}
	static CodeLine UnaryOperation(OperatorType op, VarInfo dest, VarInfo src) {
		assert(IsUnaryOperator(op));
		assert(dest.type != VarType::Number);
		return Operation(op, dest, src, {});
	}
	static CodeLine Load(VarInfo dest, VarInfo src_begin, VarInfo src_offset) {
		assert(dest.type != VarType::Number);
		assert(src_begin.type != VarType::Number);
		CodeLine code_line(LineType::Load);
		code_line.SetVar(dest, src_begin, src_offset);
		return code_line;
	}
	static CodeLine Store(VarInfo dest_begin, VarInfo dest_offset, VarInfo src) {
		assert(dest_begin.type != VarType::Number);
		CodeLine code_line(LineType::Store);
		code_line.SetVar(dest_begin, dest_offset, src);
		return code_line;
	}
	static CodeLine Assign(VarInfo dest, VarInfo src) {
		// or use UnaryOperation(OperatorType::Add, dest, src);
		return Store(dest, VarInfo(VarType::Number, 0), src);
	}
	static CodeLine Parameter(VarInfo para) {
		CodeLine code_line(LineType::Parameter);
		code_line.SetVar(para, {}, {});
		return code_line;
	}
	static CodeLine IntFuncCall(uint func_index, VarInfo dest) {
		assert(dest.type != VarType::Number);
		CodeLine code_line(LineType::FuncCall);
		code_line.var[0] = (int)func_index; 
		code_line.var_type[1] = dest.type; code_line.var[1] = dest.value;
		return code_line;
	}
	static CodeLine VoidFuncCall(uint func_index) {
		CodeLine code_line(LineType::FuncCall);
		code_line.var[0] = (int)func_index;
		return code_line;
	}
	static CodeLine Label(uint label_index) {
		CodeLine code_line(LineType::Label);
		code_line.var[0] = (int)label_index;
		return code_line;
	}
	static CodeLine JumpIf(uint label_index, OperatorType op, VarInfo src1, VarInfo src2) {
		assert(op >= OperatorType::Equal && op <= OperatorType::GreaterEuqal);
		CodeLine code_line(LineType::JumpIf);
		code_line.op = op;
		code_line.SetVar({}, src1, src2);
		code_line.var[0] = (int)label_index;
		return code_line;
	}
	static CodeLine JumpIf(uint label_index, VarInfo src) {
		return JumpIf(label_index, OperatorType::NotEqual, src, VarInfo(VarType::Number, 0));
	}
	static CodeLine JumpIfNot(uint label_index, VarInfo src) {
		return JumpIf(label_index, OperatorType::Equal, src, VarInfo(VarType::Number, 0));
	}
	static CodeLine Goto(uint label_index) {
		CodeLine code_line(LineType::Goto);
		code_line.var[0] = (int)label_index;
		return code_line;
	}
	static CodeLine ReturnVoid() {
		CodeLine code_line(LineType::Return);
		return code_line;
	}
	static CodeLine ReturnInt(VarInfo var) {
		CodeLine code_line(LineType::Return);
		code_line.var_type[0] = var.type; code_line.var[0] = var.value;
		return code_line;
	}
};
static_assert(sizeof(CodeLine) == 16);


using InitializingList = vector<std::pair<uint, int>>;

struct GlobalVarTable {
	uint length;
	InitializingList initializing_list;
};


using CodeBlock = vector<CodeLine>;

struct GlobalFuncDef {
	uint parameter_count;
	uint local_var_length;
	bool is_int;
	CodeBlock code_block;
};

using GlobalFuncTable = vector<GlobalFuncDef>;


struct LinearCode {
	GlobalVarTable global_var_table;
	GlobalFuncTable global_func_table;  // main function is the last one
};