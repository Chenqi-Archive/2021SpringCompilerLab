#pragma once

#include "keyword.h"
#include "type_info.h"


enum class CodeLineType : uchar {
	BinaryOp,	//	op	x0	x1	x2
	UnaryOp,    //  op  x0  x1
	Load,		//	ld	x0	x1	x2
	Store,		//	st	x0	x1	x2
	Parameter,	//	pm	x0
	FuncCall,	//	cl	f0  x1
	Label,		//	lb	l0
	JumpIf,		//	jo	l0	x1	x2
	Goto,		//	gt	l0
	Return,		//	rt	x0
};


struct CodeLine {
public:
	const CodeLineType type : 4;
	const OperatorType op : 4;
	const VarType var_type[3];
	const int var[3];

private:
	CodeLine(OperatorType op, VarInfo dest, VarInfo src1, VarInfo src2) :
		type(CodeLineType::BinaryOp), op(op),
		var_type{ dest.type, src1.type, src2.type }, var{ dest.value, src1.value, src2.value }{}
	CodeLine(OperatorType op, VarInfo dest, VarInfo src) :
		type(CodeLineType::UnaryOp), op(op),
		var_type{ dest.type, src.type, VarType::Void }, var{ dest.value, src.value, 0 }{}
	CodeLine(VarInfo dest, VarInfo src_begin, VarInfo src_offset) : 
		type(CodeLineType::Load), op(OperatorType::None), 
		var_type{ dest.type, src_begin.type, src_offset.type }, var{ dest.value, src_begin.value, src_offset.value }{}
	CodeLine(bool store_tag, VarInfo dest_begin, VarInfo dest_offset, VarInfo src) :
		type(CodeLineType::Store), op(OperatorType::None),
		var_type{ dest_begin.type, dest_offset.type, src.type }, var{ dest_begin.value, dest_offset.value, src.value }{}
	CodeLine(VarInfo para) :
		type(CodeLineType::Parameter), op(OperatorType::None), var_type{ para.type }, var{ para.value }{}
	CodeLine(uint func_index, VarInfo dest) :
		type(CodeLineType::FuncCall), op(OperatorType::None),
		var_type{ VarType::Void, dest.type }, var{ (int)func_index, dest.value } {}
	CodeLine(uint func_index) :
		type(CodeLineType::FuncCall), op(OperatorType::None),
		var_type{ VarType::Void }, var{ (int)func_index } {}
	CodeLine(bool label_tag, uint label_index) :
		type(CodeLineType::Label), op(OperatorType::None),
		var_type{ VarType::Void }, var{ (int)label_index } {}
	CodeLine(uint label_index, OperatorType op, VarInfo src1, VarInfo src2) :
		type(CodeLineType::JumpIf), op(op),
		var_type{ VarType::Void, src1.type, src2.type }, var{ (int)label_index, src1.value, src2.value }{
		assert(op >= OperatorType::Equal && op <= OperatorType::GreaterEuqal); }
	CodeLine(bool goto_tag, bool goto_tag2, uint label_index) :
		type(CodeLineType::Goto), op(OperatorType::None),
		var_type{ VarType::Void }, var{ (int)label_index } {}
	CodeLine() : 
		type(CodeLineType::Return), op(OperatorType::None), var_type{}, var{} {}
	CodeLine(bool return_tag, VarInfo var):
		type(CodeLineType::Return), op(OperatorType::None), var_type{ var.type }, var{ var.value }{}

public:
	static CodeLine BinaryOperation(OperatorType op, VarInfo dest, VarInfo src1, VarInfo src2) {
		assert(IsBinaryOperator(op));
		return CodeLine(op, dest, src1, src2);
	}
	static CodeLine UnaryOperation(OperatorType op, VarInfo dest, VarInfo src) {
		assert(IsUnaryOperator(op)); 
		return CodeLine(op, dest, src);
	}
	static CodeLine Load(VarInfo dest, VarInfo src_begin, VarInfo src_offset) {
		return CodeLine(dest, src_begin, src_offset);
	}
	static CodeLine Store(VarInfo dest_begin, VarInfo dest_offset, VarInfo src) {
		return CodeLine(true, dest_begin, dest_offset, src);
	}
	static CodeLine Assign(VarInfo dest, VarInfo src) {
		// or use UnaryOperation(OperatorType::Add, dest, src);
		return Store(dest, VarInfo::Number(0), src);
	}
	static CodeLine Parameter(VarInfo para) {
		return CodeLine(para);
	}
	static CodeLine IntFuncCall(uint func_index, VarInfo dest) {
		return CodeLine(func_index, dest);
	}
	static CodeLine VoidFuncCall(uint func_index) {
		return CodeLine(func_index);
	}
	static CodeLine Label(uint label_index) {
		return CodeLine(true, label_index);
	}
	static CodeLine JumpIf(uint label_index, OperatorType op, VarInfo src1, VarInfo src2) {
		return CodeLine(label_index, op, src1, src2);
	}
	static CodeLine JumpIf(uint label_index, VarInfo src) {
		return JumpIf(label_index, OperatorType::NotEqual, src, VarInfo::Number(0));
	}
	static CodeLine JumpIfNot(uint label_index, VarInfo src) {
		return JumpIf(label_index, OperatorType::Equal, src, VarInfo::Number(0));
	}
	static CodeLine Goto(uint label_index) {
		return CodeLine(true, true, label_index);
	}
	static CodeLine ReturnVoid() {
		return CodeLine();
	}
	static CodeLine ReturnInt(VarInfo var) {
		return CodeLine(true, var);
	}
};

static_assert(sizeof(CodeLine) == 16);


using InitializingList = vector<std::pair<uint, int>>;

struct GlobalVarTable {
	uint length = 0;
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
	GlobalFuncTable global_func_table;
	uint main_func_index = -1;
};