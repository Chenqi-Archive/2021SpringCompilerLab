#include "analyzer.h"
#include "reversion_wrapper.h"


inline const VarEntry& Analyzer::AddVar(string_view identifier, const ArraySize& array_size, bool is_global) {
	auto [it, success] = var_symbol_table_stack.back().insert(std::make_pair(identifier, VarEntry(array_size, AllocateVarIndex(array_size.length), is_global)));
	if (!success) { throw compile_error("variable redefinition"); }
	return it->second;
}

inline void Analyzer::AddConstVar(string_view identifier, const ArraySize& array_size, const InitializingList& initializing_list) {
	auto [it, success] = var_symbol_table_stack.back().insert(std::make_pair(identifier, VarEntry(array_size, initializing_list)));
	if (!success) { throw compile_error("variable redefinition"); }
}

inline void Analyzer::AddGlobalFunc(string_view identifier, bool is_int, ParameterTypeList&& parameter_type_list) {
	auto [it, success] = func_symbol_table.insert(std::make_pair(identifier, FuncEntry{ AllocateFuncIndex(), is_int, std::move(parameter_type_list) }));
	if (!success) { throw compile_error("function redefinition"); }
}

inline void Analyzer::EnterLocalBlock() {
	var_symbol_table_stack.push_back({});
	var_index_stack.push_back(var_index_stack.back());
}

inline void Analyzer::LeaveLocalBlock() {
	max_local_var_size = std::max(max_local_var_size, var_index_stack.back());
	var_symbol_table_stack.pop_back();
	var_index_stack.pop_back();
}

inline const VarEntry& Analyzer::FindVar(string_view identifier) {
	for (auto& symbol_table : reverse(var_symbol_table_stack)) {
		if (auto it = symbol_table.find(identifier); it != symbol_table.end()) {
			return it->second;
		}
	}
	throw compile_error("variable undefined");
}

inline const FuncEntry& Analyzer::FindFunc(string_view identifier) {
	if (auto it = func_symbol_table.find(identifier); it != func_symbol_table.end()) { return it->second; }
	throw compile_error("function undefined");
}

inline int Analyzer::EvalConstExp(const ExpTree& exp_tree) {
	assert(exp_tree != nullptr);
	switch (exp_tree->GetType()) {
	case ExpNodeType::Var: {
			auto& exp_node_var = exp_tree->As<ExpNode_Var>();
			auto& const_var_entry = FindVar(exp_node_var.identifier);
			if (!const_var_entry.IsConst()) { throw compile_error("expression must have a constant value"); }
			return const_var_entry.ReadAtIndex(EvalArrayIndex(exp_node_var.array_subscript));
		}
	case ExpNodeType::FuncCall: throw compile_error("expression must have a constant value");
	case ExpNodeType::Integer: return exp_tree->As<ExpNode_Integer>().number;
	case ExpNodeType::UnaryOp: {
			auto& exp_node_unary_op = exp_tree->As<ExpNode_UnaryOp>();
			return EvalUnaryOperator(exp_node_unary_op.op, EvalConstExp(exp_node_unary_op.child));
		}
	case ExpNodeType::BinaryOp: {
			auto& exp_node_binary_op = exp_tree->As<ExpNode_BinaryOp>();
			return EvalBinaryOperator(exp_node_binary_op.op, EvalConstExp(exp_node_binary_op.left), EvalConstExp(exp_node_binary_op.right));
		}
	default: assert(false); return 0;
	}
}

inline ArrayIndex Analyzer::EvalArrayIndex(const ArraySubscript& array_subscript) {
	vector<uint> index; index.reserve(array_subscript.size());
	for (auto& exp_tree : array_subscript) {
		int size = EvalConstExp(exp_tree);
		if (size < 0) { throw compile_error("can not access position before the begin of an array"); }
		index.push_back((uint)size);
	}
	return index;
}

inline ArraySize Analyzer::EvalArraySize(const ArrayDimension& array_dimension) {
	vector<uint> dimension; dimension.reserve(array_dimension.size());
	for (auto& exp_tree : array_dimension) {
		int size = EvalConstExp(exp_tree);
		if (size <= 0) { throw compile_error("the size of an array must be greater than zero"); }
		dimension.push_back((uint)size);
	}
	return ArraySize(std::move(dimension));
}

inline ArraySize Analyzer::EvalParameterArraySize(const ArrayDimension& array_dimension) {
	if (array_dimension.empty()) { return ArraySize(vector<uint>()); }
	vector<uint> dimension; dimension.reserve(array_dimension.size()); dimension[0] = 1;
	for (auto it = array_dimension.begin() + 1; it != array_dimension.end(); ++it) {
		int size = EvalConstExp(*it);
		if (size <= 0) { throw compile_error("the size of an array must be greater than zero"); }
		dimension.push_back((uint)size);
	}
	return ArraySize(std::move(dimension));
}

inline void Analyzer::FillExpTreeInitializingList(
	const vector<uint>& array_dimension, uint current_index, uint current_offset, uint current_size,
	const InitializerList& initializer_list, initializer_iterator& current_initializer,
	ExpTreeInitializingList& exp_tree_initializing_list
) {
	assert(current_index <= array_dimension.size());
	if (current_index == array_dimension.size()) {
		assert(current_size == 1);
		initializer_iterator initializer = current_initializer;
		while (!initializer->IsExpression()) {
			auto& current_initializer_list = initializer->initializer_list;
			if (current_initializer_list.size() == 0) { return; }
			if (current_initializer_list.size() == 1) { initializer = current_initializer_list.begin(); continue; }
			throw compile_error("too many initializer values");
		}
		exp_tree_initializing_list.push_back({ current_offset, initializer->expression });
	} else {
		assert(current_size % array_dimension[current_index] == 0);
		uint next_offset = current_offset;
		uint next_size = current_size / array_dimension[current_index];
		for (; next_offset < current_size + current_offset && current_initializer != initializer_list.end(); next_offset += next_size) {
			if (current_initializer->IsExpression()) {
				FillExpTreeInitializingList(
					array_dimension, current_index + 1, next_offset, next_size,
					initializer_list, current_initializer, exp_tree_initializing_list
				);
			} else {
				auto& current_initializer_list = current_initializer->initializer_list;
				if (current_initializer_list.empty()) { continue; }
				auto child_initializer_iterator = current_initializer_list.begin();
				FillExpTreeInitializingList(
					array_dimension, current_index + 1, next_offset, next_size,
					current_initializer_list, child_initializer_iterator, exp_tree_initializing_list
				);
				if (child_initializer_iterator != current_initializer_list.end()) {
					throw compile_error("too many initializer values");
				}
			}
		}
	}
}

inline Analyzer::ExpTreeInitializingList Analyzer::GetExpTreeInitializingList(const ArraySize& array_size, const InitializerList& initializer_list) {
	if (initializer_list.empty()) { return {}; }
	assert(initializer_list.size() == 1);
	ExpTreeInitializingList exp_tree_initializing_list;
	auto current_initializer = initializer_list.begin();
	if (array_size.dimension.empty()) {
		FillExpTreeInitializingList(
			array_size.dimension, 0, 0, array_size.length,
			initializer_list, current_initializer, exp_tree_initializing_list
		);
	} else {
		if (current_initializer->IsExpression()) {
			throw compile_error("initialization with {...} expected for array");
		}
		auto& current_initializer_list = current_initializer->initializer_list;
		if (current_initializer_list.empty()) { return {}; }
		auto child_initializer_iterator = current_initializer_list.begin();
		FillExpTreeInitializingList(
			array_size.dimension, 0, 0, array_size.length,
			current_initializer_list, child_initializer_iterator, exp_tree_initializing_list
		);
		if (child_initializer_iterator != current_initializer_list.end()) {
			throw compile_error("too many initializer values");
		}
	}
	return exp_tree_initializing_list;
}

inline InitializingList Analyzer::EvalExpTreeInitializingList(const ExpTreeInitializingList& exp_tree_initializing_list) {
	InitializingList initializing_list; initializing_list.reserve(exp_tree_initializing_list.size());
	for (auto [index, exp_tree] : exp_tree_initializing_list) {
		initializing_list.push_back(std::make_pair(index, EvalConstExp(exp_tree)));
	}
	return initializing_list;
}

inline InitializingList Analyzer::GetInitializingList(const ArraySize& array_size, const InitializerList& initializer_list) {
	return EvalExpTreeInitializingList(GetExpTreeInitializingList(array_size, initializer_list));
}

inline void Analyzer::AddParameterList(const ParameterList& parameter_list) {
	assert(var_index_stack.size() == 1);
	var_index_stack.push_back(0);
	for (auto& parameter_def : parameter_list) {
		AddVar(parameter_def.identifier, EvalParameterArraySize(parameter_def.array_dimension), false);
	}
}

inline ParameterTypeList Analyzer::GetParameterTypeList() {
	assert(var_symbol_table_stack.size() == 2);
	ParameterTypeList parameter_type_list(var_symbol_table_stack.back().size());
	for (const auto& [identifier, local_var_entry] : var_symbol_table_stack.back()) {
		assert(local_var_entry.index < parameter_type_list.size());
		parameter_type_list[local_var_entry.index] = &local_var_entry.GetArraySize();
	}
	return parameter_type_list;
}

inline VarInfo Analyzer::ReadArraySubscript(const ArraySize& array_size, const ArraySubscript& subscript) {
	if (subscript.size() > array_size.dimension.size()) { throw compile_error("expression must have a value type"); }
	uint current_size = array_size.length;
	VarInfo var_current_offset = AllocateTempVar();
	AppendCodeLine(CodeLine::Assign(var_current_offset, VarInfo(VarType::Number, 0)));
	for (uint i = 0; i < subscript.size(); ++i) {
		assert(current_size % array_size.dimension[i] == 0);
		current_size = current_size / array_size.dimension[i];
		VarInfo var_index = ReadExpTree(subscript[i]);
		if (var_index.IsEmpty()) { throw compile_error("expression must have int type"); }
		AppendCodeLine(CodeLine::BinaryOperation(OperatorType::Mul, var_index, var_index, VarInfo(VarType::Number, current_size)));
		AppendCodeLine(CodeLine::BinaryOperation(OperatorType::Add, var_current_offset, var_current_offset, var_index));
	}
	return VarInfo(var_current_offset.type, current_size, var_current_offset.value);
}

inline VarInfo Analyzer::ReadVarRef(const ExpNode_Var& exp_node_var) {
	auto& var_entry = FindVar(exp_node_var.identifier);
	if (var_entry.IsConst()) {
		return VarInfo(VarType::Number, var_entry.ReadAtIndex(EvalArrayIndex(exp_node_var.array_subscript)));
	} else {
		VarInfo var_offset = ReadArraySubscript(var_entry.GetArraySize(), exp_node_var.array_subscript);
		AppendCodeLine(CodeLine::BinaryOperation(OperatorType::Add, var_offset, var_offset, VarInfo(VarType::Local, var_entry.index)));
		return var_offset;
	}
}

inline VarInfo Analyzer::ReadFuncCall(const ExpNode_FuncCall& exp_node_func_call) {
	auto& func_entry = FindFunc(exp_node_func_call.identifier);
	if (exp_node_func_call.argument_list.size() != func_entry.parameter_type_list.size()) {
		throw compile_error("too few or too many arguments in function call");
	}
	vector<VarInfo> argument_list; argument_list.reserve(func_entry.parameter_type_list.size());
	for (uint i = 0; i < exp_node_func_call.argument_list.size(); ++i) {
		VarInfo var_argument = ReadExpTree(exp_node_func_call.argument_list[i]);
		if (var_argument.IsEmpty() || var_argument.length != func_entry.parameter_type_list[i]->length) {
			throw compile_error("argument type mismatch");
		}
		argument_list.push_back(var_argument);
	}
	for (auto& var_argument : argument_list) {
		AppendCodeLine(CodeLine::Parameter(var_argument));
	}
	if (func_entry.is_int) {
		VarInfo var_return_value = AllocateTempVar();
		AppendCodeLine(CodeLine::IntFuncCall(func_entry.index, var_return_value));
		return var_return_value;
	} else {
		AppendCodeLine(CodeLine::VoidFuncCall(func_entry.index));
		return {};
	}
}

inline VarInfo Analyzer::ReadUnaryOp(const ExpNode_UnaryOp& exp_node_unary_op) {
	VarInfo var_info = ReadExpTree(exp_node_unary_op.child);
	if (var_info.IsEmpty() || var_info.length != 1) { throw compile_error("expression must have int type"); }
	if (var_info.IsNumber()) {
		return VarInfo(VarType::Number, EvalUnaryOperator(exp_node_unary_op.op, var_info.value));
	} else {
		if (exp_node_unary_op.op == OperatorType::Add) { return var_info; }
		VarInfo var_temp = AllocateTempVar();
		AppendCodeLine(CodeLine::UnaryOperation(exp_node_unary_op.op, var_temp, var_info));
		return var_temp;
	}
}

inline VarInfo Analyzer::ReadBinaryOp(const ExpNode_BinaryOp& exp_node_binary_op) {
	VarInfo var_info_left = ReadExpTree(exp_node_binary_op.left);
	if (var_info_left.IsEmpty() || var_info_left.length != 1) { throw compile_error("expression must have int type"); }
	uint label_next = -1;
	if (exp_node_binary_op.op == OperatorType::And) {
		if (var_info_left.IsNumber() && !var_info_left.value) { return VarInfo(VarType::Number, 0); }
		label_next = AllocateLabel();
		AppendCodeLine(CodeLine::JumpIfNot(label_next, var_info_left));
	}
	if (exp_node_binary_op.op == OperatorType::Or) {
		if (var_info_left.IsNumber() && var_info_left.value) { return VarInfo(VarType::Number, 1); }
		label_next = AllocateLabel();
		AppendCodeLine(CodeLine::JumpIf(label_next, var_info_left));
	}
	VarInfo var_info_right = ReadExpTree(exp_node_binary_op.right);
	if (var_info_right.IsEmpty() || var_info_right.length != 1) { throw compile_error("expression must have int type"); }
	if (var_info_left.IsNumber() && var_info_right.IsNumber()) {
		assert(label_next == -1);
		return VarInfo(VarType::Number, EvalBinaryOperator(exp_node_binary_op.op, var_info_left.value, var_info_right.value));
	} else if (exp_node_binary_op.op == OperatorType::Assign) {
		if (var_info_left.IsNumber()) { throw compile_error("expression must be a modifiable lvalue"); }
		AppendCodeLine(CodeLine::Assign(var_info_left, var_info_right));
	} else {
		AppendCodeLine(CodeLine::BinaryOperation(exp_node_binary_op.op, var_info_left, var_info_left, var_info_right));
	}
	if (label_next != -1) { AppendCodeLine(CodeLine::Label(label_next)); }
	return var_info_left;
}

inline VarInfo Analyzer::ReadExpTree(const ExpTree& exp_tree) {
	assert(exp_tree != nullptr);
	switch (exp_tree->GetType()) {
	case ExpNodeType::Var: return ReadVarRef(exp_tree->As<ExpNode_Var>());
	case ExpNodeType::FuncCall: return ReadFuncCall(exp_tree->As<ExpNode_FuncCall>());
	case ExpNodeType::Integer: return VarInfo(VarType::Number, exp_tree->As<ExpNode_Integer>().number);
	case ExpNodeType::UnaryOp: return ReadUnaryOp(exp_tree->As<ExpNode_UnaryOp>());
	case ExpNodeType::BinaryOp: return ReadBinaryOp(exp_tree->As<ExpNode_BinaryOp>());
	default: assert(false); return {};
	}
}

inline void Analyzer::ReadLocalVarDef(const AstNode_VarDef& node_var_def) {
	ArraySize array_size = EvalArraySize(node_var_def.array_dimension);
	ExpTreeInitializingList exp_tree_initializing_list = GetExpTreeInitializingList(array_size, node_var_def.initializer_list);
	if (node_var_def.is_const) {
		AddConstVar(node_var_def.identifier, array_size, EvalExpTreeInitializingList(exp_tree_initializing_list));
	} else {
		const VarEntry& var_entry = AddVar(node_var_def.identifier, array_size, false);
		VarInfo dest_begin = VarInfo(VarType::Local, var_entry.index);
		for (auto& [index, exp_tree] : exp_tree_initializing_list) {
			VarInfo src = ReadExpTree(exp_tree);
			if (src.IsEmpty()) { throw compile_error("expression must have int type"); }
			VarInfo dest_offset = VarInfo(VarType::Number, index);
			AppendCodeLine(CodeLine::Store(src, dest_begin, dest_offset));
		}
	}
}

inline void Analyzer::ReadExp(const AstNode_Exp& node_exp) {
	ReadExpTree(node_exp.expression);
}

inline void Analyzer::ReadBlock(const AstNode_Block& node_block) {
	ReadLocalBlock(node_block.block);
}

inline void Analyzer::ReadIf(const AstNode_If& node_if) {
	VarInfo var_expression_value = ReadExpTree(node_if.expression);
	if (var_expression_value.IsEmpty()) { throw compile_error("expression must have int type"); }
	uint label_else = AllocateLabel();
	AppendCodeLine(CodeLine::JumpIfNot(label_else, var_expression_value));
	ReadLocalBlock(node_if.then_block);
	uint label_next = AllocateLabel();
	AppendCodeLine(CodeLine::Goto(label_next));
	AppendCodeLine(CodeLine::Label(label_else));
	ReadLocalBlock(node_if.else_block);
	AppendCodeLine(CodeLine::Label(label_next));
}

inline void Analyzer::ReadWhile(const AstNode_While& node_while) {
	VarInfo var_expression_value = ReadExpTree(node_while.expression);
	if (var_expression_value.IsEmpty()) { throw compile_error("expression must have int type"); }
	uint old_label_break = label_break;
	uint old_label_continue = label_continue;
	label_continue = AllocateLabel();
	AppendCodeLine(CodeLine::Label(label_continue));
	label_break = AllocateLabel();
	AppendCodeLine(CodeLine::JumpIfNot(label_break, var_expression_value));
	ReadLocalBlock(node_while.block);
	AppendCodeLine(CodeLine::Goto(label_continue));
	AppendCodeLine(CodeLine::Label(label_break));
	label_break = old_label_break;
	label_continue = old_label_continue;
}

inline void Analyzer::ReadBreak(const AstNode_Break& node_break) {
	if (label_break == -1) {
		throw compile_error("a break statement may only be used within a loop or switch");
	}
	AppendCodeLine(CodeLine::Goto(label_break));
}

inline void Analyzer::ReadContinue(const AstNode_Continue& node_continue) {
	if (label_continue == -1) {
		throw compile_error("a continue statement may only be used within a loop");
	}
	AppendCodeLine(CodeLine::Goto(label_continue));
}

inline void Analyzer::ReadReturn(const AstNode_Return& node_return) {
	AppendCodeLine(node_return.expression == nullptr ? CodeLine::ReturnVoid() : CodeLine::ReturnInt(ReadExpTree(node_return.expression)));
}

inline void Analyzer::ReadLocalBlock(const Block& block) {
	EnterLocalBlock();
	for (auto& node : block) {
		switch (node->GetType()) {
		case AstNodeType::VarDef: ReadLocalVarDef(node->As<AstNode_VarDef>()); break;
		case AstNodeType::FuncDef: throw compile_error("invalid function definition");
		case AstNodeType::Exp: ReadExp(node->As<AstNode_Exp>()); break;
		case AstNodeType::Block: ReadBlock(node->As<AstNode_Block>()); break;
		case AstNodeType::If: ReadIf(node->As<AstNode_If>()); break;
		case AstNodeType::While: ReadWhile(node->As<AstNode_While>()); break;
		case AstNodeType::Break: ReadBreak(node->As<AstNode_Break>()); break;
		case AstNodeType::Continue: ReadContinue(node->As<AstNode_Continue>()); break;
		case AstNodeType::Return: ReadReturn(node->As<AstNode_Return>()); break;
		default: assert(false); break;
		}
	}
	LeaveLocalBlock();
}

inline std::pair<uint, InitializingList> Analyzer::ReadGlobalVarDef(const AstNode_VarDef& var_def) {
	ArraySize array_size = EvalArraySize(var_def.array_dimension);
	InitializingList initializing_list = GetInitializingList(array_size, var_def.initializer_list);
	if (var_def.is_const) {
		AddConstVar(var_def.identifier, array_size, initializing_list);
		return { (uint)-1, {} };
	} else {
		auto& var_entry = AddVar(var_def.identifier, array_size, true);
		return { var_entry.index, std::move(initializing_list) };
	}
}

inline GlobalFuncDef Analyzer::ReadGlobalFuncDef(const AstNode_FuncDef& func_def) {
	assert(current_func_code_block.empty());
	is_return_type_int = func_def.is_int;
	max_local_var_size = 0;
	current_label_count = 0;
	assert(label_break == -1);
	assert(label_continue == -1);
	AddParameterList(func_def.parameter_list);
	AddGlobalFunc(func_def.identifier, func_def.is_int, GetParameterTypeList());
	ReadLocalBlock(func_def.block);
	return GlobalFuncDef{ func_def.parameter_list.size(), max_local_var_size, func_def.is_int, std::move(current_func_code_block) };
}

LinearCode Analyzer::ReadGlobalBlock(const Block& block) {
	assert(var_symbol_table_stack.empty());
	assert(var_index_stack.empty());
	assert(func_symbol_table.empty());
	var_symbol_table_stack.push_back({});
	var_index_stack.push_back(0);
	LinearCode linear_code;
	for (auto& node : block) {
		switch (node->GetType()) {
		case AstNodeType::VarDef:
			if (auto [index, initializing_list] = ReadGlobalVarDef(node->As<AstNode_VarDef>()); index != -1) {
				for (auto& [offset, value] : initializing_list) { offset += index; }
				[](auto& dest, auto& src) { dest.insert(dest.end(), src.begin(), src.end()); }
				(linear_code.global_var_table.initializing_list, initializing_list);
			}
			break;
		case AstNodeType::FuncDef:
			linear_code.global_func_table.push_back(ReadGlobalFuncDef(node->As<AstNode_FuncDef>()));
			break;
		default:
			throw compile_error("expected a declaration");
		}
	}
	linear_code.global_var_table.length = var_index_stack.back();
	var_symbol_table_stack.clear();
	var_index_stack.clear();
	func_symbol_table.clear();
	return linear_code;
}