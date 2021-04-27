#pragma once

#include "keyword.h"

#include <string>
#include <vector>
#include <memory>


using std::string_view;
using std::vector;
using std::unique_ptr;


enum class ExpNodeType {
	Var,
	FuncCall,
	Integer,
	UnaryOp,
	BinaryOp,
};


struct ABSTRACT_BASE ExpNode_Base {
private:
	ExpNodeType type;
protected:
	ExpNode_Base(ExpNodeType type) : type(type) {}
public:
	ExpNodeType GetType() const { return type; }
	virtual ~ExpNode_Base() pure {}
public:
	template<class DerivedClass>
	const DerivedClass& As() const { return static_cast<const DerivedClass&>(*this); }
};

using ExpTree = unique_ptr<ExpNode_Base>;


using ArraySubscript = vector<ExpTree>;

struct ExpNode_Var : public ExpNode_Base {
public:
	string_view identifier;
	ArraySubscript array_subscript;
public:
	ExpNode_Var() : ExpNode_Base(ExpNodeType::Var) {}
};


using ArgumentList = vector<ExpTree>;

struct ExpNode_FuncCall : public ExpNode_Base {
public:
	string_view identifier;
	ArgumentList argument_list;
public:
	ExpNode_FuncCall() : ExpNode_Base(ExpNodeType::FuncCall) {}
};


struct ExpNode_Integer : public ExpNode_Base {
public:
	int number = 0;
public:
	ExpNode_Integer() : ExpNode_Base(ExpNodeType::Integer) {}
};


struct ExpNode_UnaryOp : public ExpNode_Base {
public:
	OperatorType op = OperatorType::None;
	ExpTree child;
public:
	ExpNode_UnaryOp() : ExpNode_Base(ExpNodeType::UnaryOp) {}
};


struct ExpNode_BinaryOp : public ExpNode_Base {
public:
	OperatorType op = OperatorType::None;
	ExpTree left;
	ExpTree right;
public:
	ExpNode_BinaryOp() : ExpNode_Base(ExpNodeType::BinaryOp) {}
};