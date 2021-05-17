#pragma once

#include "exp_tree.h"


// statement (or AST node) type
enum class AstNodeType {
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


struct ABSTRACT_BASE AstNode_Base {
private:
	AstNodeType type;
protected:
	AstNode_Base(AstNodeType type) : type(type) {}
public:
	AstNodeType GetType() const { return type; }
	virtual ~AstNode_Base() pure {}
public:
	template<class DerivedClass>
	const DerivedClass& As() const { return static_cast<const DerivedClass&>(*this); }
};


using Block = vector<unique_ptr<AstNode_Base>>;
using SyntaxTree = Block;


struct Initializer {  // used like a union
public:
	ExpTree expression;  // as a single expression
	vector<Initializer> initializer_list;  // as a list of Initializer
public:
	bool IsExpression() const { return expression != nullptr; }
};

using ArrayDimension = vector<ExpTree>;
using InitializerList = vector<Initializer>;

struct AstNode_VarDef : public AstNode_Base {
public:
	string_view identifier;
	ArrayDimension array_dimension;
	InitializerList initializer_list;  // with zero or one element
	bool is_const = false;
public:
	AstNode_VarDef() : AstNode_Base(AstNodeType::VarDef) {}
};


struct ParameterDef {
	string_view identifier;
	ArrayDimension array_dimension;
};

using ParameterList = vector<ParameterDef>;

struct AstNode_FuncDef : public AstNode_Base {
public:
	string_view identifier;
	ParameterList parameter_list;
	Block block;
	bool is_int = false;
public:
	AstNode_FuncDef() : AstNode_Base(AstNodeType::FuncDef) {}
};


struct AstNode_Exp : public AstNode_Base {
public:
	ExpTree expression;
public:
	AstNode_Exp() : AstNode_Base(AstNodeType::Exp) {}
};


struct AstNode_Block : public AstNode_Base {
public:
	Block block;
public:
	AstNode_Block() : AstNode_Base(AstNodeType::Block) {}
};


struct AstNode_If : public AstNode_Base {
public:
	ExpTree expression;
	Block then_block;
	Block else_block;
public:
	AstNode_If() : AstNode_Base(AstNodeType::If) {}
};


struct AstNode_While : public AstNode_Base {
public:
	ExpTree expression;
	Block block;
public:
	AstNode_While() : AstNode_Base(AstNodeType::While) {}
};


struct AstNode_Break : public AstNode_Base {
public:
	AstNode_Break() : AstNode_Base(AstNodeType::Break) {}
};


struct AstNode_Continue : public AstNode_Base {
public:
	AstNode_Continue() : AstNode_Base(AstNodeType::Continue) {}
};


struct AstNode_Return : public AstNode_Base {
public:
	ExpTree expression;
public:
	AstNode_Return() : AstNode_Base(AstNodeType::Return) {}
};