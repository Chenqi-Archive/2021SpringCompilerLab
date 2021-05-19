#include "parser.h"


void Parser::ExpParser::ApplyTopBinaryOp() {
	assert(!binary_op_stack.empty());
	assert(exp_stack.size() >= 2);
	ExpNode_BinaryOp exp_node_binary_op;
	exp_node_binary_op.op = binary_op_stack.top(); binary_op_stack.pop();
	exp_node_binary_op.right = std::move(exp_stack.top()); exp_stack.pop();
	exp_node_binary_op.left = std::move(exp_stack.top());
	exp_stack.top() = std::make_unique<ExpNode_BinaryOp>(std::move(exp_node_binary_op));
}

void Parser::ExpParser::ReadOperator(OperatorType op) {
	if (expected_exp) {
		if (!IsUnaryOperator(op)) { throw compile_error("expected an expression"); }
		unary_op_stack.push(op); return;
	} else {
		if(!IsBinaryOperator(op)) { throw compile_error("expected a binary operator"); }
		while (!binary_op_stack.empty()) {
			if (GetBinaryOperatorPriority(op) < GetBinaryOperatorPriority(binary_op_stack.top()) || 
				op == OperatorType::Assign && binary_op_stack.top() == OperatorType::Assign) {  // right-associative
				break;
			} else {
				ApplyTopBinaryOp();
			}
		}
		binary_op_stack.push(op);
		expected_exp = true;
	}
}

void Parser::ExpParser::ReadExp(ExpTree exp_tree) {
	if (!expected_exp) { throw compile_error("expected an operator"); }
	ExpTree current_exp_tree = std::move(exp_tree);
	while (!unary_op_stack.empty()) {
		ExpNode_UnaryOp exp_node_unary_op;
		exp_node_unary_op.op = unary_op_stack.top(); unary_op_stack.pop();
		exp_node_unary_op.child = std::move(current_exp_tree);
		current_exp_tree = std::make_unique<ExpNode_UnaryOp>(std::move(exp_node_unary_op));
	}
	exp_stack.push(std::move(current_exp_tree));
	expected_exp = false;
}

ExpTree Parser::ExpParser::GetExpTree() {
	if (expected_exp) { throw compile_error("expected an expression"); }
	assert(unary_op_stack.empty());
	while (!binary_op_stack.empty()) { ApplyTopBinaryOp(); }
	assert(exp_stack.size() == 1);
	ExpTree exp_tree = std::move(exp_stack.top()); exp_stack.pop();
	expected_exp = true;
	return exp_tree;
}

void Parser::ExpReadVar(item_const_iterator& it) {
	// identifier
	assert(it != it_end);
	assert(it->GetType() == ItemType::Identifier);
	ExpNode_Var exp_node_var;
	exp_node_var.identifier = it.As<Item_Identifier>().identifier;
	it++;

	// [][]...
	exp_node_var.array_subscript = ReadArrayDimension(it);

	exp_parser->ReadExp(std::make_unique<ExpNode_Var>(std::move(exp_node_var)));
}

void Parser::ExpReadFuncCall(item_const_iterator& it) {
	// identifier
	assert(it != it_end);
	assert(it->GetType() == ItemType::Identifier);
	ExpNode_FuncCall exp_node_func_call;
	exp_node_func_call.identifier = it.As<Item_Identifier>().identifier;
	it++;

	// argument-list
	assert(it != it_end);
	assert(it->GetType() == ItemType::Block);
	assert(it.As<Item_Block>().bracket_type == BracketType::Round);
	exp_node_func_call.argument_list = ReadArgumentList(it.As<Item_Block>());
	it++;

	exp_parser->ReadExp(std::make_unique<ExpNode_FuncCall>(std::move(exp_node_func_call)));
}

void Parser::ExpReadIdentifier(item_const_iterator& it) {
	if (it_end - it >= 2) {
		if (item_const_iterator it_forward = it + 1;
			it_forward->GetType() == ItemType::Block && it_forward.As<Item_Block>().bracket_type == BracketType::Round) {
			return ExpReadFuncCall(it);
		}
	}
	return ExpReadVar(it);
}

void Parser::ExpReadInteger(item_const_iterator& it) {
	// integer
	assert(it != it_end);
	assert(it->GetType() == ItemType::Integer);
	ExpNode_Integer exp_node_integer; 
	exp_node_integer.number = it.As<Item_Integer>().number;
	it++;

	exp_parser->ReadExp(std::make_unique<ExpNode_Integer>(std::move(exp_node_integer)));
}

ExpTree Parser::ReadExpTree(item_const_iterator& it) {
	assert(it != it_end);
	ref_ptr<ExpParser> old_exp_parser = this->exp_parser;
	ExpParser exp_parser; 
	this->exp_parser = &exp_parser;

	for (; it != it_end;) {
		switch (it->GetType()) {
		case ItemType::Identifier: ExpReadIdentifier(it); break;
		case ItemType::Integer: ExpReadInteger(it); break;
		case ItemType::Operator: exp_parser.ReadOperator(it.As<Item_Operator>().op); it++; break;
		case ItemType::Block:
			if (it.As<Item_Block>().bracket_type == BracketType::Round) {
				exp_parser.ReadExp(ReadExpTreeRoundBracket(it.As<Item_Block>())); it++; break;
			}
			[[fallthrough]];
		case ItemType::Keyword:
		case ItemType::Comma:
		case ItemType::Semicolon: goto Finished;
		default: assert(false); goto Finished;
		}
	}

Finished:
	this->exp_parser = old_exp_parser;
	return exp_parser.GetExpTree();
}

ExpTree Parser::ReadExpTree(const Item_Block& lex_block) {
	if (lex_block.item_list.empty()) { return nullptr; }
	item_const_iterator old_it_end = it_end;
	it_end = lex_block.item_list.end();
	item_const_iterator it = lex_block.item_list.begin();
	ExpTree exp_tree = ReadExpTree(it);
	if(it != it_end) { throw compile_error("expected an expression"); }
	it_end = old_it_end;
	return exp_tree;
}

ExpTree Parser::ReadExpTreeRoundBracket(const Item_Block& lex_block) {
	assert(lex_block.bracket_type == BracketType::Round);
	if (lex_block.item_list.empty()) { throw compile_error("expected an expression"); }
	return ReadExpTree(lex_block);
}

ExpTree Parser::ReadExpTreeSquareBracket(const Item_Block& lex_block) {
	assert(lex_block.bracket_type == BracketType::Square);
	return ReadExpTree(lex_block);
}

Block Parser::ReadBlock(const Item_Block& lex_block) {
	if (lex_block.bracket_type != BracketType::Curly) { throw compile_error("expected a '{'"); }

	if (lex_block.item_list.empty()) { return {}; }

	ref_ptr<Block> old_block = current_block;
	item_const_iterator old_it_end = it_end;

	Block block; current_block = &block;
	it_end = lex_block.item_list.end();
	for (item_const_iterator it = lex_block.item_list.begin(); it != it_end;) { ReadNode(it); }

	current_block = old_block;
	it_end = old_it_end;

	return block;
}

Block Parser::ReadSingleNodeOrBlock(item_const_iterator& it) {
	if (it->GetType() == ItemType::Block && it.As<Item_Block>().bracket_type == BracketType::Curly) {
		item_const_iterator old_it = it; it++;
		return ReadBlock(old_it.As<Item_Block>());
	} else {
		ref_ptr<Block> old_block = current_block;
		Block block; current_block = &block;
		ReadNode(it);
		current_block = old_block;
		return block;
	}
}

ArrayDimension Parser::ReadArrayDimension(item_const_iterator& it) {
	ArrayDimension array_dimension;
	for (; it != it_end && it->GetType() == ItemType::Block && it.As<Item_Block>().bracket_type == BracketType::Square;) {
		array_dimension.push_back(ReadExpTreeSquareBracket(it.As<Item_Block>())); it++;
	}
	return array_dimension;
}

InitializerList Parser::ReadInitializerList(const Item_Block& lex_block) {
	assert(lex_block.bracket_type == BracketType::Curly);

	if (lex_block.item_list.empty()) { return {}; }

	InitializerList initializer_list;

	item_const_iterator old_it_end = it_end;
	it_end = lex_block.item_list.end();

	// initializer-list, initializer-list, ...
	for (item_const_iterator it = lex_block.item_list.begin(); it != it_end;) {

		// initializer-list
		initializer_list.push_back(ReadInitializer(it));

		if (it == it_end) { break; }

		// ,
		if (it->GetType() != ItemType::Comma) { throw compile_error("expected a ','"); }
		it++;
	}

	it_end = old_it_end;
	return initializer_list;
}

Initializer Parser::ReadInitializer(item_const_iterator& it) {
	Initializer initializer;
	if (it == it_end) { throw compile_error("expected an initializer"); }
	if (it->GetType() == ItemType::Block && it.As<Item_Block>().bracket_type == BracketType::Curly) {
		initializer.initializer_list = ReadInitializerList(it.As<Item_Block>()); it++;
	} else {
		initializer.expression = ReadExpTree(it);
		if (initializer.expression == nullptr) { throw compile_error("expected an expression"); }
	}
	return initializer;
}

ParameterList Parser::ReadParameterList(const Item_Block& lex_block) {
	assert(lex_block.bracket_type == BracketType::Round);

	if (lex_block.item_list.empty()) { return {}; }

	ParameterList parameter_list;

	item_const_iterator old_it_end = it_end;
	it_end = lex_block.item_list.end();

	// int identifier[][]..., ...
	for (item_const_iterator it = lex_block.item_list.begin();;) {
		ParameterDef parameter_def;

		// int
		ReadInt(it);

		// identifier
		parameter_def.identifier = ReadIdentifier(it);

		// array dimension
		parameter_def.array_dimension = ReadArrayDimension(it);

		parameter_list.push_back(std::move(parameter_def));

		if (it == it_end) { break; }

		// ,
		if (it->GetType() != ItemType::Comma) { throw compile_error("expected a ','"); }
		it++;
	}

	it_end = old_it_end;
	return parameter_list;
}

ArgumentList Parser::ReadArgumentList(const Item_Block& lex_block) {
	assert(lex_block.bracket_type == BracketType::Round);

	if (lex_block.item_list.empty()) { return {}; }

	ArgumentList argument_list;

	item_const_iterator old_it_end = it_end;
	it_end = lex_block.item_list.end();

	// exp, exp, ...
	for (item_const_iterator it = lex_block.item_list.begin();;) {
		// exp
		argument_list.push_back(ReadExpTree(it));

		if (it == it_end) { break; }

		// ,
		if (it->GetType() != ItemType::Comma) { throw compile_error("expected a ','"); }
		it++;
	}

	it_end = old_it_end;
	return argument_list;
}

void Parser::ReadSemicolon(item_const_iterator& it) {
	if (it == it_end || it->GetType() != ItemType::Semicolon) { 
		throw compile_error("expected a ';'"); 
	}
	it++;
}

void Parser::ReadInt(item_const_iterator& it) {
	if (it == it_end || it->GetType() != ItemType::Keyword || it.As<Item_Keyword>().keyword_type != KeywordType::Int) {
		throw compile_error("expected \"int\"");
	}
	it++;
}

string_view Parser::ReadIdentifier(item_const_iterator& it) {
	if (it == it_end || it->GetType() != ItemType::Identifier) { throw compile_error("expected an identifier"); }
	string_view identifier = it.As<Item_Identifier>().identifier; it++;
	return identifier;
}

void Parser::ReadNodeVarDef(item_const_iterator& it) {
	// const
	assert(it != it_end);
	assert(it->GetType() == ItemType::Keyword);
	bool is_const = false;
	if (it.As<Item_Keyword>().keyword_type == KeywordType::Const) { is_const = true; it++; }

	// int
	ReadInt(it);

	// identifier[][]... = initialize-list, ... ;
	do {
		AstNode_VarDef node_var_def;
		node_var_def.is_const = is_const;

		// identifier
		node_var_def.identifier = ReadIdentifier(it);

		// array dimension
		node_var_def.array_dimension = ReadArrayDimension(it);

		// = initializer
		if (it != it_end && it->GetType() == ItemType::Operator && it.As<Item_Operator>().op == OperatorType::Assign) {
			it++;

			// initializer
			node_var_def.initializer_list.push_back(ReadInitializer(it));
		}

		AppendNode(std::make_unique<AstNode_VarDef>(std::move(node_var_def)));

		// ;
		if (it == it_end) { throw compile_error("expected a ';'"); }
		if (it->GetType() == ItemType::Semicolon) { it++; break; }

		// ,
		if (it->GetType() != ItemType::Comma) { throw compile_error("expected a ','"); }
		it++;

	} while (true);
}

void Parser::ReadNodeFuncDef(item_const_iterator& it) {
	AstNode_FuncDef node_func_def;

	// void or int
	assert(it != it_end);
	assert(it->GetType() == ItemType::Keyword);
	if (it.As<Item_Keyword>().keyword_type == KeywordType::Void) {
		node_func_def.is_int = false;
	} else if (it.As<Item_Keyword>().keyword_type == KeywordType::Int) {
		node_func_def.is_int = true;
	} else { 
		assert(false); return;
	}
	it++;

	// identifier
	node_func_def.identifier = ReadIdentifier(it);

	// (parameter-list)
	if (it == it_end || it->GetType() != ItemType::Block || it.As<Item_Block>().bracket_type != BracketType::Round) {
		throw compile_error("expected a '('");
	}
	node_func_def.parameter_list = ReadParameterList(it.As<Item_Block>());
	it++;

	// {}
	if (it == it_end || it->GetType() != ItemType::Block || it.As<Item_Block>().bracket_type != BracketType::Curly) {
		throw compile_error("expected a '{'");
	}
	node_func_def.block = ReadBlock(it.As<Item_Block>());
	it++;

	AppendNode(std::make_unique<AstNode_FuncDef>(std::move(node_func_def)));
}

void Parser::ReadNodeExp(item_const_iterator& it) {
	AstNode_Exp node_exp;
	node_exp.expression = ReadExpTree(it); // expression
	ReadSemicolon(it); // ';'
	AppendNode(std::make_unique<AstNode_Exp>(std::move(node_exp)));
}

void Parser::ReadNodeBlock(item_const_iterator& it) {
	// {}
	assert(it != it_end);
	assert(it->GetType() == ItemType::Block);
	assert(it.As<Item_Block>().bracket_type == BracketType::Curly);
	AstNode_Block node_block;
	node_block.block = ReadBlock(it.As<Item_Block>());
	it++;
	AppendNode(std::make_unique<AstNode_Block>(std::move(node_block)));
}

void Parser::ReadNodeIf(item_const_iterator& it) {
	// 'if'
	assert(it != it_end);
	assert(it->GetType() == ItemType::Keyword);
	assert(it.As<Item_Keyword>().keyword_type == KeywordType::If);
	it++;

	AstNode_If node_if;

	// ()
	if (it == it_end || it->GetType() != ItemType::Block || it.As<Item_Block>().bracket_type != BracketType::Round) {
		throw compile_error("expected a '('");
	}
	node_if.expression = ReadExpTreeRoundBracket(it.As<Item_Block>());
	it++;

	// {} or Node
	if (it == it_end) { throw compile_error("expected a statement"); }
	node_if.then_block = ReadSingleNodeOrBlock(it);

	// else and else block
	if (it != it_end && it->GetType() == ItemType::Keyword && it.As<Item_Keyword>().keyword_type == KeywordType::Else) {
		it++;

		// {} or Node
		if (it == it_end) { throw compile_error("expected a statement"); }
		node_if.else_block = ReadSingleNodeOrBlock(it);
	}

	AppendNode(std::make_unique<AstNode_If>(std::move(node_if)));
}

void Parser::ReadNodeWhile(item_const_iterator& it) {
	// 'while'
	assert(it != it_end);
	assert(it->GetType() == ItemType::Keyword);
	assert(it.As<Item_Keyword>().keyword_type == KeywordType::While);
	it++;

	AstNode_While node_while;

	// ()
	if (it == it_end || it->GetType() != ItemType::Block || it.As<Item_Block>().bracket_type != BracketType::Round) {
		throw compile_error("expected a '('");
	}
	node_while.expression = ReadExpTreeRoundBracket(it.As<Item_Block>());
	it++;

	// {} or Node
	if (it == it_end) { throw compile_error("expected a statement"); }
	node_while.block = ReadSingleNodeOrBlock(it);

	AppendNode(std::make_unique<AstNode_While>(std::move(node_while)));
}

void Parser::ReadNodeBreak(item_const_iterator& it) {
	// 'break'
	assert(it != it_end);
	assert(it->GetType() == ItemType::Keyword);
	assert(it.As<Item_Keyword>().keyword_type == KeywordType::Break);
	it++;

	// ';'
	ReadSemicolon(it);

	AppendNode(std::make_unique<AstNode_Break>());
}

void Parser::ReadNodeContinue(item_const_iterator& it) {
	// 'continue'
	assert(it != it_end);
	assert(it->GetType() == ItemType::Keyword);
	assert(it.As<Item_Keyword>().keyword_type == KeywordType::Continue);
	it++;

	// ';'
	ReadSemicolon(it);

	AppendNode(std::make_unique<AstNode_Continue>());
}

void Parser::ReadNodeReturn(item_const_iterator& it) {
	// 'return'
	assert(it != it_end);
	assert(it->GetType() == ItemType::Keyword);
	assert(it.As<Item_Keyword>().keyword_type == KeywordType::Return);
	it++;

	AstNode_Return node_return;

	// expression
	node_return.expression = ReadExpTree(it);

	// ';'
	ReadSemicolon(it);

	AppendNode(std::make_unique<AstNode_Return>(std::move(node_return)));
}

void Parser::ReadNode(item_const_iterator& it) {
	assert(it != it_end);
	switch (it->GetType()) {
	case ItemType::Keyword:
		switch (static_cast<const Item_Keyword&>(*it).keyword_type) {
		case KeywordType::Int:
			if (it_end - it < 3) { throw compile_error("expected a statement"); }
			if (item_const_iterator it_forward = it + 2; 
				it_forward->GetType() == ItemType::Block && it_forward.As<Item_Block>().bracket_type == BracketType::Round) {
				return ReadNodeFuncDef(it);
			} else {
				return ReadNodeVarDef(it);
			}
		case KeywordType::Void: return ReadNodeFuncDef(it);
		case KeywordType::Const: return ReadNodeVarDef(it);
		case KeywordType::If: return ReadNodeIf(it);
		case KeywordType::Else: throw compile_error("\"else\" mismatch");
		case KeywordType::While: return ReadNodeWhile(it);
		case KeywordType::Break: return ReadNodeContinue(it);
		case KeywordType::Continue: return ReadNodeBreak(it);
		case KeywordType::Return: return ReadNodeReturn(it);
		}
		assert(false); return;
	case ItemType::Identifier:
	case ItemType::Integer:
	case ItemType::Operator: return ReadNodeExp(it);
	case ItemType::Comma: throw compile_error("expected an expression");
	case ItemType::Semicolon: it++; return;
	case ItemType::Block: return ReadNodeBlock(it);
	default: assert(false); return;
	}
}