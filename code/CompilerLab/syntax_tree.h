#pragma once

#include "exp_tree.h"


// statement (or AST node) type
enum class NodeType {
	VarDef,
	FuncDef,
	Exp,
	Block,
	If,
	While,
	Break,
	Continue,
	Return,
};


struct ABSTRACT_BASE Node_Base {
private:
	NodeType type;
protected:
	Node_Base(NodeType type) : type(type) {}
public:
	NodeType GetType() const { return type; }
	virtual ~Node_Base() pure {}
public:
	template<class DerivedClass>
	const DerivedClass& As() const { return static_cast<const DerivedClass&>(*this); }
};


using Block = vector<unique_ptr<Node_Base>>;
using SyntaxTree = Block;


struct InitializerList {  // used like a union
public:
	ExpTree expression;  // as a single expression
	vector<InitializerList> list_of_initializer_list;  // as a list of InitializerList
public:
	bool IsExpression() const { return expression != nullptr; }
};

using ArrayDimension = vector<ExpTree>;
using ListOfInitializerList = vector<InitializerList>;

struct Node_VarDef : public Node_Base {
public:
	string_view identifier;
	ArrayDimension array_dimension;
	unique_ptr<InitializerList> initializer_list;
	bool is_const = false;
public:
	Node_VarDef() : Node_Base(NodeType::VarDef) {}
};


struct ParameterDef {
	string_view identifier;
	ArrayDimension array_dimension;
};

using ParameterList = vector<ParameterDef>;

struct Node_FuncDef : public Node_Base {
public:
	string_view identifier;
	ParameterList parameter_list;
	Block block;
	bool is_int = false;
public:
	Node_FuncDef() : Node_Base(NodeType::FuncDef) {}
};


struct Node_Exp : public Node_Base {
public:
	ExpTree expression;
public:
	Node_Exp() : Node_Base(NodeType::Exp) {}
};


struct Node_Block : public Node_Base {
public:
	Block block;
public:
	Node_Block() : Node_Base(NodeType::Block) {}
};


struct Node_If : public Node_Base {
public:
	ExpTree expression;
	Block then_block;
	Block else_block;
public:
	Node_If() : Node_Base(NodeType::If) {}
};


struct Node_While : public Node_Base {
public:
	ExpTree expression;
	Block block;
public:
	Node_While() : Node_Base(NodeType::While) {}
};


struct Node_Break : public Node_Base {
public:
	Node_Break() : Node_Base(NodeType::Break) {}
};


struct Node_Continue : public Node_Base {
public:
	Node_Continue() : Node_Base(NodeType::Continue) {}
};


struct Node_Return : public Node_Base {
public:
	ExpTree expression;
public:
	Node_Return() : Node_Base(NodeType::Return) {}
};