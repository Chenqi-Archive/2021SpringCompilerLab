#pragma once

#include "syntax_tree.h"

#include <iostream>


using std::string;
using std::cout;
using std::endl;


class ParserDebugHelper {
private:
	static constexpr uint max_level = 127;
	const string tab_padding = string(max_level, '\t');
	const string_view tab_padding_view = tab_padding;
	string_view indent(uint level) const { return tab_padding_view.substr(0, level); }

private:
	void PrintBlock(const Block& block, uint level) {
		cout << indent(level) << '{' << endl;
		for (auto& node : block) { PrintNode(*node, level + 1); }
		cout << indent(level) << '}' << endl;
	}

	void PrintExpTree(const ExpTree& exp_tree, uint level) {
		if (exp_tree == nullptr) { return; }
		const ExpNode_Base& exp_node = *exp_tree;
		switch (exp_node.GetType()) {
		case ExpNodeType::Var:
			{
				auto& exp_node_var = exp_node.As<ExpNode_Var>();
				cout << indent(level) << exp_node_var.identifier << endl;
				PrintArrayDimension(exp_node_var.array_subscript, level);
			}
			break;
		case ExpNodeType::FuncCall:
			{
				auto& exp_node_func_call = exp_node.As<ExpNode_FuncCall>();
				cout << indent(level) << exp_node_func_call.identifier << endl;
				PrintArgumentList(exp_node_func_call.argument_list, level);
			}
			break;
		case ExpNodeType::Integer:
			{
				auto& exp_node_integer = exp_node.As<ExpNode_Integer>();
				cout << indent(level) << exp_node_integer.number << endl;
			}
			break;
		case ExpNodeType::UnaryOp:
			{
				auto& exp_node_unary_op = exp_node.As<ExpNode_UnaryOp>();
				cout << indent(level) << GetOperatorString(exp_node_unary_op.op) << endl;
				PrintExpTree(exp_node_unary_op.child, level + 1);
			}
			break;
		case ExpNodeType::BinaryOp:
			{
				auto& exp_node_binary_op = exp_node.As<ExpNode_BinaryOp>();
				cout << indent(level) << GetOperatorString(exp_node_binary_op.op) << endl;
				PrintExpTree(exp_node_binary_op.left, level + 1);
				PrintExpTree(exp_node_binary_op.right, level + 1);
			}
			break;
		default: assert(false); break;
		}
	}

	void PrintArrayDimension(const ArrayDimension& array_dimension, uint level) {
		if (array_dimension.empty()) { return; }
		cout << indent(level) << '[' << endl;
		for (auto& exp_tree : array_dimension) { PrintExpTree(exp_tree, level + 1); }
		cout << indent(level) << ']' << endl;
	}

	void PrintInitializerList(const InitializerList& initializer_list, uint level) {
		if (initializer_list.IsExpression()) {
			PrintExpTree(initializer_list.expression, level);
		} else {
			cout << indent(level) << '{' << endl;
			for (auto& child_initializer_list : initializer_list.list_of_initializer_list) {
				PrintInitializerList(child_initializer_list, level + 1);
			}
			cout << indent(level) << '}' << endl;
		}
	}

	void PrintParameterDef(const ParameterDef& parameter_def, uint level) {
		cout << indent(level) << parameter_def.identifier << endl;
		PrintArrayDimension(parameter_def.array_dimension, level);
	}

	void PrintParameterList(const ParameterList& parameter_list, uint level) {
		cout << indent(level) << '(' << endl;
		for (auto& parameter_def : parameter_list) { PrintParameterDef(parameter_def, level + 1); }
		cout << indent(level) << ')' << endl;
	}

	void PrintArgumentList(const ArgumentList& argument_list, uint level) {
		cout << indent(level) << '(' << endl;
		for (auto& exp_tree : argument_list) { PrintExpTree(exp_tree, level + 1); }
		cout << indent(level) << ')' << endl;
	}

	void PrintNode(const Node_Base& node, uint level) {
		switch (node.GetType()) {
		case NodeType::VarDef: 
			{
				auto& node_var_def = node.As<Node_VarDef>();
				cout << indent(level) << (node_var_def.is_const ? "const int" : "int") << endl;
				cout << indent(level) << node_var_def.identifier << endl;
				PrintArrayDimension(node_var_def.array_dimension, level);
				if (node_var_def.initializer_list != nullptr) { PrintInitializerList(*node_var_def.initializer_list, level); }
			}
			break;
		case NodeType::FuncDef:
			{
				auto& node_func_def = node.As<Node_FuncDef>();
				cout << indent(level) << (node_func_def.is_int ? "int" : "void") << endl;
				cout << indent(level) << node_func_def.identifier << endl;
				PrintParameterList(node_func_def.parameter_list, level);
				PrintBlock(node_func_def.block, level);
			}
			break;
		case NodeType::Exp:
			{
				auto& node_exp = node.As<Node_Exp>();
				PrintExpTree(node_exp.expression, level);
			}
			break;
		case NodeType::Block:
			{
				auto& node_block = node.As<Node_Block>();
				PrintBlock(node_block.block, level);
			}
			break;
		case NodeType::If:
			{
				auto& node_if = node.As<Node_If>();
				cout << indent(level) << "if" << endl;
				PrintExpTree(node_if.expression, level);
				PrintBlock(node_if.then_block, level);
				PrintBlock(node_if.else_block, level);
			}
			break;
		case NodeType::While:
			{
				auto& node_while = node.As<Node_While>();
				cout << indent(level) << "while" << endl;
				PrintExpTree(node_while.expression, level);
				PrintBlock(node_while.block, level);
			}
			break;
		case NodeType::Break:
			{
				cout << indent(level) << "break" << endl;
			}
			break;
		case NodeType::Continue:
			{
				cout << indent(level) << "continue" << endl;
			}
			break;
		case NodeType::Return:
			{
				auto& node_return = node.As<Node_Return>();
				cout << indent(level) << "return" << endl;
				PrintExpTree(node_return.expression, level);
			}
			break;
		default: assert(false); break;
		}
	}

public:
	void PrintSyntaxTree(const SyntaxTree& syntax_tree) {
		PrintBlock(syntax_tree, 0);
	}
};