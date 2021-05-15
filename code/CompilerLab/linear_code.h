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
private: friend class CodeLine;
	VarType type;
	int value;
private:
	constexpr VarInfo() : type(VarType::Empty), value() {}
public:
	explicit constexpr VarInfo(VarType type, int value) : type(type), value(value) {
		assert(type != VarType::Empty && type <= VarType::Number);
		if (type == VarType::Global || type == VarType::Local) { assert(value >= 0); }
	}
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
	static CodeLine Store(VarInfo src, VarInfo dest_begin, VarInfo dest_offset) {
		assert(dest_begin.type != VarType::Number);
		CodeLine code_line(LineType::Store);
		code_line.SetVar(src, dest_begin, dest_offset);
		return code_line;
	}
	static CodeLine Parameter(VarInfo para) {
		CodeLine code_line(LineType::Parameter);
		code_line.SetVar(para, {}, {});
		return code_line;
	}
	static CodeLine IntFuncCall(int func_index, VarInfo dest) {
		assert(func_index >= 0);
		assert(dest.type != VarType::Number);
		CodeLine code_line(LineType::FuncCall);
		code_line.var[0] = func_index; 
		code_line.var_type[1] = dest.type; code_line.var[1] = dest.value;
		return code_line;
	}
	static CodeLine VoidFuncCall(int func_index) {
		assert(func_index >= 0);
		CodeLine code_line(LineType::FuncCall);
		code_line.var[0] = func_index;
		return code_line;
	}
	static CodeLine Label(int label_index) {
		assert(label_index >= 0);
		CodeLine code_line(LineType::Label);
		code_line.var[0] = label_index;
		return code_line;
	}
	static CodeLine JumpIf(int label_index, OperatorType op, VarInfo src1, VarInfo src2) {
		assert(label_index >= 0);
		assert(op >= OperatorType::Equal && op <= OperatorType::GreaterEuqal);
		CodeLine code_line(LineType::JumpIf);
		code_line.op = op;
		code_line.SetVar({}, src1, src2);
		code_line.var[0] = label_index;
		return code_line;
	}
	static CodeLine JumpIf(int label_index, VarInfo src) {
		return JumpIf(label_index, OperatorType::Greater, src, VarInfo(VarType::Number, 0));
	}
	static CodeLine Goto(int label_index) {
		assert(label_index >= 0);
		CodeLine code_line(LineType::Goto);
		code_line.var[0] = label_index;
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


using CodeBlock = vector<CodeLine>;


struct GlobalVarDef {
	uint length;
	vector<std::pair<uint, int>> initializing_list;
};

using GlobalVarTable = vector<GlobalVarDef>;


struct GlobalFuncDef {
	uint parameter_count;
	uint local_var_count;  // include the array or not?
	CodeBlock code_block;
};

using GlobalFuncTable = vector<GlobalFuncDef>;


struct LinearCode {
	GlobalVarTable global_var_table;
	GlobalFuncTable global_func_table;  // main function is the last one
};