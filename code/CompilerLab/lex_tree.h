#pragma once

#include "keyword.h"

#include <string>
#include <vector>
#include <memory>


using std::string_view;
using std::vector;
using std::unique_ptr;


enum class ItemType : uchar {
	Keyword,
	Identifier,
	Integer,
	Operator,
	Comma,
	Semicolon,
	Block,
};


struct ABSTRACT_BASE Item_Base {
private:
	ItemType type;
protected:
	Item_Base(ItemType type) : type(type) {}
public:
	ItemType GetType() const { return type; }
	virtual ~Item_Base() pure {}
public:
	template<class DerivedClass>
	const DerivedClass& As() const { return static_cast<const DerivedClass&>(*this); }
};


struct Item_Keyword : public Item_Base {
public:
	KeywordType keyword_type;
public:
	Item_Keyword(KeywordType keyword_type) : Item_Base(ItemType::Keyword), keyword_type(keyword_type) {}
};


struct Item_Identifier : public Item_Base {
public:
	string_view identifier;
public:
	Item_Identifier(string_view identifier) : Item_Base(ItemType::Identifier), identifier(identifier) {}
};


struct Item_Integer : public Item_Base {
public:
	int number;
public:
	Item_Integer(int number) : Item_Base(ItemType::Integer), number(number) {}
};


struct Item_Operator : public Item_Base {
public:
	OperatorType op;
public:
	Item_Operator(OperatorType op) : Item_Base(ItemType::Operator), op(op) {}
};


struct Item_Comma : public Item_Base {
public:
	Item_Comma() : Item_Base(ItemType::Comma) {}
};


struct Item_Semicolon : public Item_Base {
public:
	Item_Semicolon() : Item_Base(ItemType::Semicolon) {}
};


struct Item_Block : public Item_Base {
public:
	BracketType bracket_type;
	vector<unique_ptr<Item_Base>> item_list;
public:
	Item_Block(BracketType bracket_type) : Item_Base(ItemType::Block), bracket_type(bracket_type) {}
};


struct LexTree : public Item_Block {
	LexTree() : Item_Block(BracketType::Curly) {}
};