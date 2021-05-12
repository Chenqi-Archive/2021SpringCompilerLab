#pragma once

#include "lex_tree.h"
#include "syntax_tree.h"

#include <stack>


using std::string;
using std::stack;


class Parser {
private:
	struct item_const_iterator : public vector<unique_ptr<Item_Base>>::const_iterator {
	private:
		using _MyBase = vector<unique_ptr<Item_Base>>::const_iterator;
	public:
		item_const_iterator() : _MyBase() {}
		item_const_iterator(_MyBase it) : _MyBase(it) {}
		const Item_Base& operator*() const { return *_MyBase::operator*(); }
		const Item_Base* operator->() const { return _MyBase::operator*().get(); }
	public:
		template<class DerivedClass>
		const DerivedClass& As() const { return static_cast<const DerivedClass&>(operator*()); }
	};

private:
	ref_ptr<Block> current_block = nullptr;
	item_const_iterator it_end = {};

private:
	class ExpParser {
	private:
		stack<OperatorType> unary_op_stack;
		stack<OperatorType> binary_op_stack;
		stack<ExpTree> exp_stack;
		bool expected_exp = true;
	private:
		void ApplyTopBinaryOp();
	public:
		void ReadOperator(OperatorType op);
		void ReadExp(ExpTree exp_tree);
		ExpTree GetExpTree();
	};
	ref_ptr<ExpParser> exp_parser = nullptr;

private:
	void ExpReadVar(item_const_iterator& it);
	void ExpReadFuncCall(item_const_iterator& it);
	void ExpReadIdentifier(item_const_iterator& it);
	void ExpReadInteger(item_const_iterator& it);

private:
	ExpTree ReadExpTree(item_const_iterator& it);
	ExpTree ReadExpTree(const Item_Block& lex_block);
	ExpTree ReadExpTreeRoundBracket(const Item_Block& lex_block);
	ExpTree ReadExpTreeSquareBracket(const Item_Block& lex_block);
	Block ReadBlock(const Item_Block& lex_block);
	Block ReadSingleNodeOrBlock(item_const_iterator& it);
	ArrayDimension ReadArrayDimension(item_const_iterator& it);
	ListOfInitializerList ReadListOfInitializerList(const Item_Block& lex_block);
	InitializerList ReadInitializerList(item_const_iterator& it);
	ParameterList ReadParameterList(const Item_Block& lex_block);
	ArgumentList ReadArgumentList(const Item_Block& lex_block);

private:
	void AppendNode(unique_ptr<AstNode_Base> node) { current_block->push_back(std::move(node)); }

private:
	void ReadSemicolon(item_const_iterator& it);
	void ReadInt(item_const_iterator& it);
	string_view ReadIdentifier(item_const_iterator& it);

private:
	void ReadNodeVarDef(item_const_iterator& it);
	void ReadNodeFuncDef(item_const_iterator& it);
	void ReadNodeExp(item_const_iterator& it);
	void ReadNodeBlock(item_const_iterator& it);
	void ReadNodeIf(item_const_iterator& it);
	void ReadNodeWhile(item_const_iterator& it);
	void ReadNodeBreak(item_const_iterator& it);
	void ReadNodeContinue(item_const_iterator& it);
	void ReadNodeReturn(item_const_iterator& it);
	void ReadNode(item_const_iterator& it);

public:
	SyntaxTree ReadLexTree(const LexTree& lex_tree) { return ReadBlock(lex_tree); }
};