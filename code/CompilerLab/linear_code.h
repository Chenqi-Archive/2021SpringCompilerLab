#pragma once

#include "keyword.h"
#include "type_info.h"


enum class CodeLineType : uchar {
	BinaryOp,	//	op	x0	x1	x2
	UnaryOp,    //  op  x0  x1
	Addr,		//  ad  x0  x1  x2
	Load,		//	ld	x0	x1	x2
	Store,		//	st	x0	x1	x2
	Parameter,	//	pm	x0
	FuncCall,	//	cl	f0  x1
	Label,		//	lb	l0
	JumpIf,		//	jo	l0	x1	x2
	Goto,		//	gt	l0
	Return,		//	rt	x0
};


struct CodeLineVarType {
public:
	const enum class Type : uchar { Empty, Number, Local, Global, Addr } type;
	operator Type() const { return type; }
public:
	CodeLineVarType() : type(Type::Empty) {}
	CodeLineVarType(const VarInfo& var_info) : type(ConvertVarType(var_info)) {}
public:
	bool IsValid() const { return type != Type::Empty; }
	bool IsIntOrRef() const { return type == Type::Number || IsRef(); }
	bool IsRef() const { return type == Type::Local || type == Type::Global; }
	bool IsAddr() const { return type == Type::Addr; }
	bool IsRefOrAddr() const { return IsRef() || type == Type::Addr; }
private:
	Type ConvertVarType(const VarInfo& var_info) {
		switch (var_info.type) 	{
		case VarType::Int: return var_info.is_number ? Type::Number : Type::Local;
		case VarType::IntRef: return var_info.is_global ? Type::Global : Type::Local;
		case VarType::ArrayPtr: return Type::Addr;
		default: assert(false); return Type::Empty;
		}
	}
};


struct CodeLine {
public:
	const CodeLineType type : 4;
	const OperatorType op : 4;
	const CodeLineVarType var_type[3];
	const int var[3];

private:
	CodeLine(OperatorType op, const VarInfo& dest, const VarInfo& src1, const VarInfo& src2) :
		type(CodeLineType::BinaryOp), op(op),
		var_type{ dest, src1, src2 }, var{ dest.value, src1.value, src2.value }{ 
		assert(IsBinaryOperator(op)); 
		assert(var_type[0].IsRef() && var_type[1].IsIntOrRef() && var_type[2].IsIntOrRef());
	}
	CodeLine(OperatorType op, const VarInfo& dest, const VarInfo& src) :
		type(CodeLineType::UnaryOp), op(op),
		var_type{ dest, src }, var{ dest.value, src.value, 0 }{
		assert(IsUnaryOperator(op)); 
		assert(var_type[0].IsRef() && var_type[1].IsIntOrRef());
	}
	CodeLine(const VarInfo& dest, const VarInfo& src_begin, const VarInfo& src_offset) :
		type(CodeLineType::Addr), op(OperatorType::None),
		var_type{ dest, src_begin, src_offset }, var{ dest.value, src_begin.value, src_offset.value }{
		assert(var_type[0].IsAddr() && var_type[1].IsRefOrAddr() && var_type[2].IsIntOrRef());
	}
	CodeLine(bool load_tag, const VarInfo& dest, const VarInfo& src_begin, const VarInfo& src_offset) :
		type(CodeLineType::Load), op(OperatorType::None), 
		var_type{ dest, src_begin, src_offset }, var{ dest.value, src_begin.value, src_offset.value }{
		assert(var_type[0].IsRef() && var_type[1].IsRefOrAddr() && var_type[2].IsIntOrRef());
	}
	CodeLine(bool store_tag, bool store_tag_2, const VarInfo& dest_begin, const VarInfo& dest_offset, const VarInfo& src) :
		type(CodeLineType::Store), op(OperatorType::None),
		var_type{ dest_begin, dest_offset, src }, var{ dest_begin.value, dest_offset.value, src.value }{
		assert(var_type[0].IsRefOrAddr() && var_type[1].IsIntOrRef() && var_type[2].IsIntOrRef());
	}
	CodeLine(const VarInfo& para) :
		type(CodeLineType::Parameter), op(OperatorType::None), var_type{ para }, var{ para.value }{
		assert(var_type[0].IsValid());
	}
	CodeLine(uint func_index, const VarInfo& dest) :
		type(CodeLineType::FuncCall), op(OperatorType::None),
		var_type{ {}, dest }, var{ (int)func_index, dest.value } {
		assert(var_type[1].IsRef());
	}
	CodeLine(uint func_index) :
		type(CodeLineType::FuncCall), op(OperatorType::None),
		var_type{}, var{ (int)func_index } {
	}
	CodeLine(bool label_tag, uint label_index) :
		type(CodeLineType::Label), op(OperatorType::None),
		var_type{}, var{ (int)label_index } {
	}
	CodeLine(uint label_index, OperatorType op, const VarInfo& src1, const VarInfo& src2) :
		type(CodeLineType::JumpIf), op(op),
		var_type{ {}, src1, src2 }, var{ (int)label_index, src1.value, src2.value }{
		assert(op >= OperatorType::Equal && op <= OperatorType::GreaterEuqal);
		assert(var_type[1].IsIntOrRef() && var_type[2].IsIntOrRef());
	}
	CodeLine(bool goto_tag, bool goto_tag_2, uint label_index) :
		type(CodeLineType::Goto), op(OperatorType::None),
		var_type{}, var{ (int)label_index } {}
	CodeLine() : 
		type(CodeLineType::Return), op(OperatorType::None), var_type{}, var{} {
	}
	CodeLine(bool return_tag, const VarInfo& var):
		type(CodeLineType::Return), op(OperatorType::None), var_type{ var }, var{ var.value }{
		assert(var_type[0].IsIntOrRef());
	}

public:
	static CodeLine BinaryOperation(OperatorType op, const VarInfo& dest, const VarInfo& src1, const VarInfo& src2) {
		return CodeLine(op, dest, src1, src2);
	}
	static CodeLine UnaryOperation(OperatorType op, const VarInfo& dest, const VarInfo& src) {
		return CodeLine(op, dest, src);
	}
	static CodeLine Addr(const VarInfo& dest, const VarInfo& src_begin, const VarInfo& src_offset) {
		return CodeLine(dest, src_begin, src_offset);
	}
	static CodeLine Load(const VarInfo& dest, const VarInfo& src_begin, const VarInfo& src_offset) {
		return CodeLine(true, dest, src_begin, src_offset);
	}
	static CodeLine Store(const VarInfo& dest_begin, const VarInfo& dest_offset, const VarInfo& src) {
		return CodeLine(true, true, dest_begin, dest_offset, src);
	}
	static CodeLine Assign(const VarInfo& dest, const VarInfo& src) {
		// or use UnaryOperation(OperatorType::Add, dest, src);
		return Store(dest, VarInfo::Number(0), src);
	}
	static CodeLine Parameter(const VarInfo& para) {
		return CodeLine(para);
	}
	static CodeLine IntFuncCall(uint func_index, const VarInfo& dest) {
		return CodeLine(func_index, dest);
	}
	static CodeLine VoidFuncCall(uint func_index) {
		return CodeLine(func_index);
	}
	static CodeLine Label(uint label_index) {
		return CodeLine(true, label_index);
	}
	static CodeLine JumpIf(uint label_index, OperatorType op, const VarInfo& src1, const VarInfo& src2) {
		return CodeLine(label_index, op, src1, src2);
	}
	static CodeLine JumpIf(uint label_index, const VarInfo& src) {
		return JumpIf(label_index, OperatorType::NotEqual, src, VarInfo::Number(0));
	}
	static CodeLine JumpIfNot(uint label_index, const VarInfo& src) {
		return JumpIf(label_index, OperatorType::Equal, src, VarInfo::Number(0));
	}
	static CodeLine Goto(uint label_index) {
		return CodeLine(true, true, label_index);
	}
	static CodeLine ReturnVoid() {
		return CodeLine();
	}
	static CodeLine ReturnInt(const VarInfo& var) {
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