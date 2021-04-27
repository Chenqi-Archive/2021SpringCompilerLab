#pragma once

#include "core.h"

#include <string>


using std::string_view;


enum class OperatorType : uchar {
	Add,			// +  
	Sub,			// -  
	Mul,			// *  
	Div,			// /  
	Mod,			// %  

	Assign,			// =  

	And,			// && 
	Or,				// || 
	Not,			// !  

	Equal,			// == 
	NotEqual,		// != 
	Less,			// <  
	Greater,		// >  
	LessEqual,		// <= 
	GreaterEuqal,	// >= 

	_Count,
	None = _Count,
};

const char* GetOperatorString(OperatorType op);
bool IsUnaryOperator(OperatorType op);
bool IsBinaryOperator(OperatorType op);
int GetBinaryOperatorPriority(OperatorType op);  // (highest) 0 --- 8 (lowest)


enum class BracketType : uchar {
	Round,	 // ()
	Curly,	 // {}
	Square,	 // []

	_Count,
	None = _Count,
};

char GetLeftBracket(BracketType bracket_type);
char GetRightBracket(BracketType bracket_type);


enum class KeywordType : uchar {
	Int,
	Void,
	Const,
	If,
	Else,
	While,
	Break,
	Continue,
	Return,

	_Count,
	None = _Count,
};

const char* GetKeywordString(KeywordType keyword);


enum class SymbolType : uchar {
	Keyword,
	Identifier,
	Integer,
};

struct SymbolInfo {
	SymbolType type;
	KeywordType keyword_type; // as Keyword
	int integer; // as Integer
};

const SymbolInfo GetSymbolInfo(string_view name);