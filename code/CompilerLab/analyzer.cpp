#include "analyzer.h"
#include "reversion_wrapper.h"


const VarEntry& Analyzer::AddVar(string_view identifier, const ArraySize& array_size, bool is_global, bool is_parameter) {
	bool is_pointer = !array_size.dimension.empty() && is_parameter;
	auto [it, success] = var_symbol_table_stack.back().insert(
		std::make_pair(identifier, VarEntry(array_size, AllocateVarIndex(is_pointer ? 1 : array_size.length), is_global, is_pointer))
	);
	if (!success) { throw compile_error("variable redefinition"); }
	return it->second;
}

void Analyzer::AddConstVar(string_view identifier, const ArraySize& array_size, const InitializingList& initializing_list) {
	auto [it, success] = var_symbol_table_stack.back().insert(std::make_pair(identifier, VarEntry(array_size, initializing_list)));
	if (!success) { throw compile_error("variable redefinition"); }
}

void Analyzer::AddGlobalFunc(string_view identifier, bool is_int, ParameterTypeList&& parameter_type_list) {
	auto [it, success] = func_symbol_table.try_emplace(identifier, AllocateFuncIndex(), is_int, std::move(parameter_type_list));
	if (!success) { throw compile_error("function redefinition"); }
}

void Analyzer::EnterLocalBlock() {
	var_symbol_table_stack.push_back({});
	var_index_stack.push_back(var_index_stack.back());
}

void Analyzer::LeaveLocalBlock() {
	max_local_var_size = std::max(max_local_var_size, var_index_stack.back());
	var_symbol_table_stack.pop_back();
	var_index_stack.pop_back();
}

const VarEntry& Analyzer::FindVar(string_view identifier) {
	for (auto& symbol_table : reverse(var_symbol_table_stack)) {
		if (auto it = symbol_table.find(identifier); it != symbol_table.end()) {
			return it->second;
		}
	}
	throw compile_error("variable undefined");
}

const FuncEntry& Analyzer::FindFunc(string_view identifier) {
	if (auto it = func_symbol_table.find(identifier); it != func_symbol_table.end()) { return it->second; }
	throw compile_error("function undefined");
}

int Analyzer::EvalConstExp(const ExpTree& exp_tree) {
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

ArrayIndex Analyzer::EvalArrayIndex(const ArraySubscript& array_subscript) {
	vector<uint> index; index.reserve(array_subscript.size());
	for (auto& exp_tree : array_subscript) {
		int size = EvalConstExp(exp_tree);
		if (size < 0) { throw compile_error("can not access position before the begin of an array"); }
		index.push_back((uint)size);
	}
	return index;
}

ArraySize Analyzer::EvalArraySize(const ArrayDimension& array_dimension) {
	vector<uint> dimension; dimension.reserve(array_dimension.size());
	for (auto& exp_tree : array_dimension) {
		int size = EvalConstExp(exp_tree);
		if (size <= 0) { throw compile_error("the size of an array must be greater than zero"); }
		dimension.push_back((uint)size);
	}
	return ArraySize(std::move(dimension));
}

ArraySize Analyzer::EvalParameterArraySize(const ArrayDimension& array_dimension) {
	if (array_dimension.empty()) { return ArraySize(vector<uint>()); }
	vector<uint> dimension; dimension.reserve(array_dimension.size()); dimension.push_back(1);
	for (auto it = array_dimension.begin() + 1; it != array_dimension.end(); ++it) {
		if (*it == nullptr) { throw compile_error("the array size except for the first dimension of a parameter must be specified"); }
		int size = EvalConstExp(*it);
		if (size <= 0) { throw compile_error("the size of an array must be greater than zero"); }
		dimension.push_back((uint)size);
	}
	return ArraySize(std::move(dimension));
}

void Analyzer::FillExpTreeInitializingList(
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
		current_initializer++;
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
				current_initializer++;
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

Analyzer::ExpTreeInitializingList Analyzer::GetExpTreeInitializingList(const ArraySize& array_size, const InitializerList& initializer_list) {
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

InitializingList Analyzer::EvalExpTreeInitializingList(const ExpTreeInitializingList& exp_tree_initializing_list) {
	InitializingList initializing_list; initializing_list.reserve(exp_tree_initializing_list.size());
	for (auto& [index, exp_tree] : exp_tree_initializing_list) {
		initializing_list.push_back(std::make_pair(index, EvalConstExp(exp_tree)));
	}
	return initializing_list;
}

InitializingList Analyzer::GetInitializingList(const ArraySize& array_size, const InitializerList& initializer_list) {
	return EvalExpTreeInitializingList(GetExpTreeInitializingList(array_size, initializer_list));
}

void Analyzer::AppendLabel(uint label_index) {
	if (label_index >= current_func_label_map.size()) { current_func_label_map.resize(label_index + 1, -1); }
	assert(current_func_label_map[label_index] == -1);
	current_func_label_map[label_index] = (uint)current_func_code_block.size();
}

VarInfo Analyzer::AllocateTempVarInitializedWith(int value) {
	VarInfo var_temp = AllocateTempVar();
	AppendCodeLine(CodeLine::Assign(var_temp, VarInfo::Number(value)));
	return var_temp;
}

VarInfo Analyzer::ReadArraySubscript(const VarEntry& var_entry, const ArraySubscript& subscript) {
	const vector<uint>& array_dimension = var_entry.GetArraySize().dimension;
	if (subscript.size() > array_dimension.size()) { throw compile_error("expression must have a value type"); }
	vector<std::pair<uint, VarInfo>> index; index.reserve(subscript.size());
	uint current_size = var_entry.GetArraySize().length; int current_number = 0;
	for (uint i = 0; i < subscript.size(); ++i) {
		assert(current_size % array_dimension[i] == 0);
		current_size = current_size / array_dimension[i];
		VarInfo var_index = ReadExpTreeAsIntOrIntRef(subscript[i]);
		if (var_index.IsNumber()) {
			current_number += var_index.value * current_size;
		} else {
			index.push_back(std::make_pair(current_size, var_index));
		}
	}
	// as statically determined variable
	if (subscript.size() == array_dimension.size() && index.empty() && !var_entry.is_pointer) {
		if ((uint)current_number >= var_entry.GetArraySize().length) { throw compile_error("array subscript out of range"); }
		return VarInfo::VarRef(var_entry.is_global, var_entry.index + (uint)current_number);
	} 
	// as dynamically determined variable or address
	else {
		VarInfo var_current_offset = AllocateTempVarInitializedWith(current_number);
		if (!index.empty()) {
			VarInfo var_mul_temp = AllocateTempVar();
			for (auto& [size, var] : index) {
				AppendCodeLine(CodeLine::BinaryOperation(OperatorType::Mul, var_mul_temp, var, VarInfo::Number(size)));
				AppendCodeLine(CodeLine::BinaryOperation(OperatorType::Add, var_current_offset, var_current_offset, var_mul_temp));
			}
		}
		VarInfo var_current_addr = VarInfo::ArrayPtr(vector<uint>(array_dimension.begin() + subscript.size(), array_dimension.end()), var_current_offset.value);
		VarInfo var_base_addr = var_entry.is_pointer ? VarInfo::ArrayPtr({}, var_entry.index) : VarInfo::VarRef(var_entry.is_global, var_entry.index);
		AppendCodeLine(CodeLine::Addr(var_current_addr, var_base_addr, var_current_offset));
		// as dynamically determined variable
		if (subscript.size() == array_dimension.size()) {
			if (is_assignment) {
				assert(current_var_addr_index == -1);
				current_var_addr_index = var_current_addr.value;
				VarInfo var_temp = AllocateTempVar();
				AppendCodeLine(CodeLine::Load(var_temp, var_current_addr, VarInfo::Number(0)));
				return var_temp;
			} else {
				VarInfo var_temp = VarInfo::Temp(var_current_addr.value);  // reuse the index
				AppendCodeLine(CodeLine::Load(var_temp, var_current_addr, VarInfo::Number(0)));
				return var_temp;
			}
		}
		// as address
		else {
			return var_current_addr;
		}
	}
}

VarInfo Analyzer::ReadVarRef(const ExpNode_Var& exp_node_var) {
	auto& var_entry = FindVar(exp_node_var.identifier);
	if (var_entry.IsConst()) {
		return VarInfo::Number(var_entry.ReadAtIndex(EvalArrayIndex(exp_node_var.array_subscript)));
	} else {
		return ReadArraySubscript(var_entry, exp_node_var.array_subscript);
	}
}

VarInfo Analyzer::ReadFuncCall(const ExpNode_FuncCall& exp_node_func_call) {
	auto& func_entry = FindFunc(exp_node_func_call.identifier);
	if (exp_node_func_call.argument_list.size() != func_entry.parameter_type_list.size()) {
		throw compile_error("too few or too many arguments in function call");
	}
	vector<VarInfo> argument_list; argument_list.reserve(func_entry.parameter_type_list.size());
	for (uint i = 0; i < exp_node_func_call.argument_list.size(); ++i) {
		VarInfo var_argument = ReadExpTree(exp_node_func_call.argument_list[i]);
		if(!var_argument.IsArrayTypeSame(func_entry.parameter_type_list[i])) { 
			throw compile_error("argument type mismatch"); 
		}
		argument_list.push_back(var_argument);
	}
	if (func_entry.is_int) {
		VarInfo var_return_value = AllocateTempVar();
		AppendCodeLine(CodeLine::IntFuncCall(func_entry.index, var_return_value));
		for (auto& var_argument : argument_list) { AppendCodeLine(CodeLine::Parameter(var_argument)); }
		return var_return_value;
	} else {
		AppendCodeLine(CodeLine::VoidFuncCall(func_entry.index));
		for (auto& var_argument : argument_list) { AppendCodeLine(CodeLine::Parameter(var_argument)); }
		return VarInfo::Void();
	}
}

VarInfo Analyzer::ReadUnaryOp(const ExpNode_UnaryOp& exp_node_unary_op) {
	VarInfo var_info = ReadExpTreeAsIntOrIntRef(exp_node_unary_op.child);
	if (var_info.IsNumber()) {
		return VarInfo::Number(EvalUnaryOperator(exp_node_unary_op.op, var_info.value));
	} else {
		if (exp_node_unary_op.op == OperatorType::Add) { return var_info; }
		VarInfo var_temp = AllocateTempVar();
		AppendCodeLine(CodeLine::UnaryOperation(exp_node_unary_op.op, var_temp, var_info));
		return var_temp;
	}
}

VarInfo Analyzer::ReadBinaryOp(const ExpNode_BinaryOp& exp_node_binary_op) {
	if (exp_node_binary_op.op == OperatorType::Assign) {
		is_assignment = true;
		VarInfo var_info_left = ReadExpTreeAsIntOrIntRef(exp_node_binary_op.left);
		is_assignment = false;
		if (!var_info_left.IsLValue()) {
			if (!var_info_left.IsTemp() || current_var_addr_index == -1) { throw compile_error("expression must be a modifiable lvalue"); }
			AppendCodeLine(CodeLine::Assign(VarInfo::ArrayPtr({}, current_var_addr_index), ReadExpTreeAsIntOrIntRef(exp_node_binary_op.right)));
			current_var_addr_index = -1;
		} else {
			AppendCodeLine(CodeLine::Assign(var_info_left, ReadExpTreeAsIntOrIntRef(exp_node_binary_op.right)));
		}
		return var_info_left;
	} else {
		VarInfo var_info_left = ReadExpTreeAsIntOrIntRef(exp_node_binary_op.left);
		if (exp_node_binary_op.op == OperatorType::And) {
			if (var_info_left.IsNumber()) {
				return var_info_left.value == 0 ? VarInfo::Number(0) : ReadExpTreeAsIntOrIntRef(exp_node_binary_op.right);
			}
			uint label_next = AllocateLabel();
			VarInfo var_temp = AllocateTempVarInitializedWith(0);
			AppendCodeLine(CodeLine::JumpIfNot(label_next, var_info_left));
			VarInfo var_info_right = ReadExpTreeAsIntOrIntRef(exp_node_binary_op.right);
			AppendCodeLine(CodeLine::Assign(var_temp, var_info_right));
			AppendLabel(label_next);
			return var_temp;
		} else if (exp_node_binary_op.op == OperatorType::Or) {
			if (var_info_left.IsNumber()) {
				return var_info_left.value != 0 ? VarInfo::Number(1) : ReadExpTreeAsIntOrIntRef(exp_node_binary_op.right);
			}
			uint label_next = AllocateLabel();
			VarInfo var_temp = AllocateTempVarInitializedWith(1);
			AppendCodeLine(CodeLine::JumpIf(label_next, var_info_left));
			VarInfo var_info_right = ReadExpTreeAsIntOrIntRef(exp_node_binary_op.right);
			AppendCodeLine(CodeLine::Assign(var_temp, var_info_right));
			AppendLabel(label_next);
			return var_temp;
		} else {
			VarInfo var_info_right = ReadExpTreeAsIntOrIntRef(exp_node_binary_op.right);
			if (var_info_left.IsNumber() && var_info_right.IsNumber()) {
				return VarInfo::Number(EvalBinaryOperator(exp_node_binary_op.op, var_info_left.value, var_info_right.value));
			} else {
				VarInfo var_temp = AllocateTempVar();
				AppendCodeLine(CodeLine::BinaryOperation(exp_node_binary_op.op, var_temp, var_info_left, var_info_right));
				return var_temp;
			}
		}
	}
}

VarInfo Analyzer::ReadExpTree(const ExpTree& exp_tree) {
	assert(exp_tree != nullptr);
	switch (exp_tree->GetType()) {
	case ExpNodeType::Var: return ReadVarRef(exp_tree->As<ExpNode_Var>());
	case ExpNodeType::FuncCall: return ReadFuncCall(exp_tree->As<ExpNode_FuncCall>());
	case ExpNodeType::Integer: return VarInfo::Number(exp_tree->As<ExpNode_Integer>().number);
	case ExpNodeType::UnaryOp: return ReadUnaryOp(exp_tree->As<ExpNode_UnaryOp>());
	case ExpNodeType::BinaryOp: return ReadBinaryOp(exp_tree->As<ExpNode_BinaryOp>());
	default: assert(false); return VarInfo::Void();
	}
}

VarInfo Analyzer::ReadExpTreeAsIntOrIntRef(const ExpTree& exp_tree) {
	VarInfo var_info = ReadExpTree(exp_tree);
	if (!var_info.IsRValue()) { throw compile_error("expression must have int or int& type"); }
	return var_info;
}

void Analyzer::ReadLocalVarDef(const AstNode_VarDef& node_var_def) {
	ArraySize array_size = EvalArraySize(node_var_def.array_dimension);
	ExpTreeInitializingList exp_tree_initializing_list = GetExpTreeInitializingList(array_size, node_var_def.initializer_list);
	if (node_var_def.is_const) {
		AddConstVar(node_var_def.identifier, array_size, EvalExpTreeInitializingList(exp_tree_initializing_list));
	} else {
		const VarEntry& var_entry = AddVar(node_var_def.identifier, array_size, false, false);
		if (!node_var_def.initializer_list.empty()) {
			uint length = var_entry.GetArraySize().length;
			VarInfo dest_begin = VarInfo::VarRef(false, var_entry.index);
			if (length <= 8 || exp_tree_initializing_list.size() >= length / 2) {
				// initialize each element
				uint current_offset = 0;
				for (auto& [index, exp_tree] : exp_tree_initializing_list) {
					assert(index < length);
					while (current_offset < index) {
						AppendCodeLine(CodeLine::Store(dest_begin, VarInfo::Number(current_offset++), VarInfo::Number(0)));
					}
					VarInfo src = ReadExpTreeAsIntOrIntRef(exp_tree);
					AppendCodeLine(CodeLine::Store(dest_begin, VarInfo::Number(current_offset++), src));
				}
				while (current_offset < length) {
					AppendCodeLine(CodeLine::Store(dest_begin, VarInfo::Number(current_offset++), VarInfo::Number(0)));
				}
			} else {
				// initialize all elements to zero in a loop
				VarInfo var_temp = AllocateTempVarInitializedWith(0);
				uint label_loop = AllocateLabel();
				AppendLabel(label_loop);
				AppendCodeLine(CodeLine::Store(dest_begin, var_temp, VarInfo::Number(0)));
				AppendCodeLine(CodeLine::BinaryOperation(OperatorType::Add, var_temp, var_temp, VarInfo::Number(1)));
				AppendCodeLine(CodeLine::JumpIf(label_loop, OperatorType::Less, var_temp, VarInfo::Number((int)length)));
				// initialize with initializing list
				for (auto& [index, exp_tree] : exp_tree_initializing_list) {
					VarInfo src = ReadExpTreeAsIntOrIntRef(exp_tree);
					VarInfo dest_offset = VarInfo::Number(index);
					AppendCodeLine(CodeLine::Store(dest_begin, dest_offset, src));
				}
			}
		}
	}
}

void Analyzer::ReadExp(const AstNode_Exp& node_exp) {
	ReadExpTree(node_exp.expression);
}

void Analyzer::ReadBlock(const AstNode_Block& node_block) {
	ReadLocalBlock(node_block.block);
}

void Analyzer::ReadIf(const AstNode_If& node_if) {
	VarInfo var_expression_value = ReadExpTreeAsIntOrIntRef(node_if.expression);
	if (var_expression_value.IsNumber()) {
		return var_expression_value.value != 0 ? ReadLocalBlock(node_if.then_block) : ReadLocalBlock(node_if.else_block);
	}
	uint label_else = AllocateLabel();
	AppendCodeLine(CodeLine::JumpIfNot(label_else, var_expression_value));
	ReadLocalBlock(node_if.then_block);
	uint label_next = AllocateLabel();
	AppendCodeLine(CodeLine::Goto(label_next));
	AppendLabel(label_else);
	ReadLocalBlock(node_if.else_block);
	AppendLabel(label_next);
}

void Analyzer::ReadWhile(const AstNode_While& node_while) {
	uint old_label_continue = label_continue;
	label_continue = AllocateLabel();
	AppendLabel(label_continue);
	VarInfo var_expression_value = ReadExpTreeAsIntOrIntRef(node_while.expression);
	if (!(var_expression_value.IsNumber() && var_expression_value.value == 0)) {
		uint old_label_break = label_break;
		label_break = AllocateLabel();
		if (!var_expression_value.IsNumber()) { 
			AppendCodeLine(CodeLine::JumpIfNot(label_break, var_expression_value)); 
		}
		ReadLocalBlock(node_while.block);
		AppendCodeLine(CodeLine::Goto(label_continue));
		AppendLabel(label_break);
		label_break = old_label_break;
	}
	label_continue = old_label_continue;
}

void Analyzer::ReadBreak(const AstNode_Break& node_break) {
	if (label_break == -1) {
		throw compile_error("a break statement may only be used within a loop or switch");
	}
	AppendCodeLine(CodeLine::Goto(label_break));
}

void Analyzer::ReadContinue(const AstNode_Continue& node_continue) {
	if (label_continue == -1) {
		throw compile_error("a continue statement may only be used within a loop");
	}
	AppendCodeLine(CodeLine::Goto(label_continue));
}

void Analyzer::ReadReturn(const AstNode_Return& node_return) {
	if (node_return.expression == nullptr) {
		AppendCodeLine(CodeLine::ReturnVoid());
	} else {
		VarInfo var_ret = ReadExpTreeAsIntOrIntRef(node_return.expression);
		AppendCodeLine(CodeLine::ReturnInt(var_ret));
	}
}

void Analyzer::ReadLocalBlock(const Block& block) {
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

void Analyzer::AddParameterList(const ParameterList& parameter_list) {
	assert(var_symbol_table_stack.size() == 1);
	assert(var_index_stack.size() == 1);
	var_symbol_table_stack.push_back({});
	var_index_stack.push_back(0);
	for (auto& parameter_def : parameter_list) {
		AddVar(parameter_def.identifier, EvalParameterArraySize(parameter_def.array_dimension), false, true);
	}
}

void Analyzer::RemoveParameterList() {
	assert(var_symbol_table_stack.size() == 2);
	assert(var_index_stack.size() == 2);
	var_symbol_table_stack.pop_back();
	var_index_stack.pop_back();
}

ParameterTypeList Analyzer::GetParameterTypeList() {
	assert(var_symbol_table_stack.size() == 2);
	assert(var_index_stack.size() == 2);
	ParameterTypeList parameter_type_list(var_symbol_table_stack.back().size());
	for (const auto& [identifier, local_var_entry] : var_symbol_table_stack.back()) {
		assert(local_var_entry.index < parameter_type_list.size());
		parameter_type_list[local_var_entry.index] = local_var_entry.GetArraySize().dimension;
	}
	return parameter_type_list;
}

std::pair<uint, InitializingList> Analyzer::ReadGlobalVarDef(const AstNode_VarDef& var_def) {
	ArraySize array_size = EvalArraySize(var_def.array_dimension);
	InitializingList initializing_list = GetInitializingList(array_size, var_def.initializer_list);
	if (var_def.is_const) {
		AddConstVar(var_def.identifier, array_size, initializing_list);
		return { (uint)-1, {} };
	} else {
		auto& var_entry = AddVar(var_def.identifier, array_size, true, false);
		return { var_entry.index, std::move(initializing_list) };
	}
}

GlobalFuncDef Analyzer::ReadGlobalFuncDef(const AstNode_FuncDef& func_def) {
	assert(current_func_code_block.empty());
	is_return_type_int = func_def.is_int;
	max_local_var_size = 0;
	assert(current_func_label_map.empty());
	current_label_count = 0;
	assert(label_break == -1);
	assert(label_continue == -1);
	AddParameterList(func_def.parameter_list);
	AddGlobalFunc(func_def.identifier, func_def.is_int, GetParameterTypeList());
	ReadLocalBlock(func_def.block);
	RemoveParameterList();
	return GlobalFuncDef{
		(uint)func_def.parameter_list.size(), max_local_var_size, func_def.is_int,
		std::move(current_func_code_block), std::move(current_func_label_map)
	};
}

uint Analyzer::GetMainFuncIndex() {
	auto main_func_entry = FindFunc("main");
	if (!main_func_entry.parameter_type_list.empty()) { throw compile_error("main function must have no parameter"); }
	if (!main_func_entry.is_int) { throw compile_error("main function must return int"); }
	return main_func_entry.index;
}

LinearCode Analyzer::ReadGlobalBlock(const Block& block) {
	assert(var_symbol_table_stack.empty());
	assert(var_index_stack.empty());
	assert(func_symbol_table.size() == library_func_number);
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
	assert(var_index_stack.size() == 1);
	linear_code.global_var_table.length = var_index_stack.back();
	linear_code.main_func_index = GetMainFuncIndex();
	var_symbol_table_stack.clear();
	var_index_stack.clear();
	func_symbol_table = FuncSymbolTable();
	return linear_code;
}