#pragma once

#include "syntax_tree.h"
#include "symbol_table.h"
#include "linear_code.h"


class Analyzer {
private:
	VarSymbolTableStack var_symbol_table_stack;
	LocalVarIndexStack var_index_stack;
	FuncSymbolTable func_symbol_table;
private:
	uint AllocateVarIndex(uint length) { uint index = var_index_stack.back(); var_index_stack.back() += length; return index; }
	uint AllocateFuncIndex() { return func_symbol_table.size(); }
private:
	const VarEntry& AddVar(string_view identifier, const ArraySize& array_size, bool is_global);
	void AddConstVar(string_view identifier, const ArraySize& array_size, const InitializingList& initializing_list);
	void AddGlobalFunc(string_view identifier, bool is_int, ParameterTypeList&& parameter_type_list);
private:
	void EnterLocalBlock();
	void LeaveLocalBlock();
private:
	const VarEntry& FindVar(string_view identifier);
	const FuncEntry& FindFunc(string_view identifier);

private:
	int EvalConstExp(const ExpTree& exp_tree);
	ArrayIndex EvalArrayIndex(const ArraySubscript& array_subscript);
	ArraySize EvalArraySize(const ArrayDimension& array_dimension);
	ArraySize EvalParameterArraySize(const ArrayDimension& array_dimension);

private:
	using initializer_iterator = InitializerList::const_iterator;
	using ExpTreeInitializingList = vector<std::pair<uint, const ExpTree&>>;

	static void FillExpTreeInitializingList(
		const vector<uint>& array_dimension, uint current_index, uint current_offset, uint current_size,
		const InitializerList& initializer_list, initializer_iterator& current_initializer,
		ExpTreeInitializingList& exp_tree_initializing_list
	);
	static ExpTreeInitializingList GetExpTreeInitializingList(const ArraySize& array_size, const InitializerList& initializer_list);
	InitializingList EvalExpTreeInitializingList(const ExpTreeInitializingList& exp_tree_initializing_list);
	InitializingList GetInitializingList(const ArraySize& array_size, const InitializerList& initializer_list);

private:
	CodeBlock current_func_code_block;
	bool is_return_type_int = false;
	uint max_local_var_size = 0;
	uint current_label_count = 0;
	uint label_break = -1;
	uint label_continue = -1;

private:
	VarInfo AllocateTempVar(vector<uint> dimension = {}) { return VarInfo::LocalTemp(dimension, AllocateVarIndex(1)); }
	uint AllocateLabel() { return current_label_count++; }
	void AppendCodeLine(CodeLine code_line) { current_func_code_block.push_back(code_line); }
	VarInfo AllocateTempVarInitializedWith(int value, vector<uint> dimension = {});

private:
	VarInfo ReadArraySubscript(const ArraySize& array_size, const ArraySubscript& subscript);
	VarInfo ReadVarRef(const ExpNode_Var& exp_node_var);
	VarInfo ReadFuncCall(const ExpNode_FuncCall& exp_node_func_call);
	VarInfo ReadUnaryOp(const ExpNode_UnaryOp& exp_node_unary_op);
	VarInfo ReadBinaryOp(const ExpNode_BinaryOp& exp_node_binary_op);
	VarInfo ReadExpTree(const ExpTree& exp_tree);
	VarInfo ReadExpTreeAsInt(const ExpTree& exp_tree);

private:
	void ReadLocalVarDef(const AstNode_VarDef& node_var_def);
	void ReadExp(const AstNode_Exp& node_exp);
	void ReadBlock(const AstNode_Block& node_block);
	void ReadIf(const AstNode_If& node_if);
	void ReadWhile(const AstNode_While& node_while);
	void ReadBreak(const AstNode_Break& node_break);
	void ReadContinue(const AstNode_Continue& node_continue);
	void ReadReturn(const AstNode_Return& node_return);
	void ReadLocalBlock(const Block& block);

private:
	void AddParameterList(const ParameterList& parameter_list);
	void RemoveParameterList();
	ParameterTypeList GetParameterTypeList();
private:
	std::pair<uint, InitializingList> ReadGlobalVarDef(const AstNode_VarDef& var_def);
	GlobalFuncDef ReadGlobalFuncDef(const AstNode_FuncDef& func_def);
	LinearCode ReadGlobalBlock(const Block& block);
public:
	LinearCode ReadSyntaxTree(const SyntaxTree& syntax_tree) { return ReadGlobalBlock(syntax_tree); }
};