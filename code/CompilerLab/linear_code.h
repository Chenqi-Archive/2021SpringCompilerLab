#pragma once

#include "keyword.h"

#include <vector>


using std::vector;


enum class LineType : uchar {
	Operation,	// op x0 x1 x2
	Load,		// ld x0 x1 x2
	Store,		// st x0 x1 x2
	Parameter,	// pm x0
	FuncCall,	// cl f0
	Label,		// lb l0
	JumpIf,		// ji l0 x1   // use op or not?
	Goto,		// gt l0
	Return		// rt x0
};


struct VarIndex {
private:
	ushort index;
public:
	VarIndex() : index(-1) {}
	VarIndex(ushort index, bool is_global) : index((index << 1) | is_global) {}
	bool IsValid() const { return index != -1; }
	bool IsGlobal() const { return index & 1; }
	ushort GetIndex() const { return index >> 1; }
	ushort GetRawIndex() const { return index; }
};

using FuncIndex = ushort;
using LabelIndex = ushort;


struct CodeLine {
	LineType type;
	OperatorType op = OperatorType::None;
	union {
		VarIndex var0;
		FuncIndex func;
		LabelIndex label;
	};
	VarIndex var1;
	VarIndex var2;
};
static_assert(sizeof(CodeLine) == 8);

using CodeBlock = vector<CodeLine>;


struct GlobalVarDef {
	uint length;
	vector<std::pair<uint, int>> initializer_list;
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