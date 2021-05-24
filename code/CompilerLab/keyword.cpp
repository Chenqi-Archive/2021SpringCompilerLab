#include "keyword.h"

#include <array>
#include <algorithm>


struct OperatorInfo {
	const char* str;
	bool is_unary_op;
	bool is_binary_op;
	uchar priority;
	// bool is_left_to_right;  // all but '='
};

inline const OperatorInfo& GetOperatorInfo(OperatorType op) {
	static constexpr OperatorInfo operator_info_table[static_cast<uchar>(OperatorType::_Count) + 1] = {
		{"+", true, true, 2},
		{"-", true, true, 2},
		{"*", false, true, 1},
		{"/", false, true, 1},
		{"%", false, true, 1},

		{"=", false, true, 7},

		{"&&", false, true, 5},
		{"||", false, true, 6},
		{"!", true, false, 0},

		{"==", false, true, 4},
		{"!=", false, true, 4},
		{"<", false, true, 3},
		{">", false, true, 3},
		{"<=", false, true, 3},
		{">=", false, true, 3},

		{ "", false, false, 0 },
	};

	if (op < OperatorType::_Count) {
		return operator_info_table[static_cast<uchar>(op)];
	} else {
		assert(false);
		return operator_info_table[static_cast<uchar>(OperatorType::None)];
	}
}

const char* GetOperatorString(OperatorType op) {
	return GetOperatorInfo(op).str;
}

bool IsUnaryOperator(OperatorType op) {
	return GetOperatorInfo(op).is_unary_op;
}

bool IsBinaryOperator(OperatorType op) {
	return GetOperatorInfo(op).is_binary_op;
}

int GetBinaryOperatorPriority(OperatorType op) {
	return GetOperatorInfo(op).priority;
}

int EvalUnaryOperator(OperatorType op, int value) {
	switch (op) {
	case OperatorType::Add: return value;
	case OperatorType::Sub: return -value;
	case OperatorType::Not: return (int)!(bool)value;
	default: assert(false); return 0;
	}
}

int EvalBinaryOperator(OperatorType op, int value_left, int value_right) {
	switch (op) {
	case OperatorType::Add: return value_left + value_right;
	case OperatorType::Sub: return value_left - value_right;
	case OperatorType::Mul: return value_left * value_right;
	case OperatorType::Div: return value_left / value_right;
	case OperatorType::Mod: return value_left % value_right;
	case OperatorType::Assign: throw compile_error("expression must be a modifiable lvalue");
	case OperatorType::And: return (int)((bool)value_left && (bool)value_right);
	case OperatorType::Or: return (int)((bool)value_left || (bool)value_right);
	case OperatorType::Not: assert(false); return 0;
	case OperatorType::Equal: return (int)(value_left == value_right);
	case OperatorType::NotEqual: return (int)(value_left != value_right);
	case OperatorType::Less: return (int)(value_left < value_right);
	case OperatorType::Greater: return (int)(value_left > value_right);
	case OperatorType::LessEqual: return (int)(value_left <= value_right);
	case OperatorType::GreaterEuqal: return (int)(value_left >= value_right);
	default: assert(false); return 0;
	}
}


struct BracketInfo {
	char left;
	char right;
};

inline BracketInfo GetBracketInfo(BracketType bracket_type) {
	static constexpr BracketInfo bracket_info_table[static_cast<uchar>(BracketType::_Count)] = {
		{'(',')'},
		{'{','}'},
		{'[',']'},
	};
	if (bracket_type < BracketType::_Count) {
		return bracket_info_table[static_cast<uchar>(bracket_type)];
	} else {
		assert(false);
		return { '\n' , '\n' };
	}
}

char GetLeftBracket(BracketType bracket_type) {
	return GetBracketInfo(bracket_type).left;
}

char GetRightBracket(BracketType bracket_type) {
	return GetBracketInfo(bracket_type).right;
}


inline KeywordType GetKeywordType(string_view name) {
	static constexpr std::array<std::pair<string_view, KeywordType>, static_cast<uchar>(KeywordType::_Count)> keyword_map = {
		std::make_pair("break", KeywordType::Break),
		std::make_pair("const", KeywordType::Const),
		std::make_pair("continue", KeywordType::Continue),
		std::make_pair("else", KeywordType::Else),
		std::make_pair("if", KeywordType::If),
		std::make_pair("int", KeywordType::Int),
		std::make_pair("return", KeywordType::Return),
		std::make_pair("void", KeywordType::Void),
		std::make_pair("while", KeywordType::While),
	};
	auto it = std::lower_bound(keyword_map.begin(), keyword_map.end(), name, [](auto& pair, auto& name) { return pair.first < name; });
	if (it == keyword_map.end() || it->first != name) { return KeywordType::None; }
	return it->second;
}

const char* GetKeywordString(KeywordType keyword) {
	switch (keyword) {
	case KeywordType::Int: return "int";
	case KeywordType::Void: return "void";
	case KeywordType::Const: return "const";
	case KeywordType::If: return "if";
	case KeywordType::Else: return "else";
	case KeywordType::While: return "while";
	case KeywordType::Break: return "break";
	case KeywordType::Continue: return "continue";
	case KeywordType::Return: return "return";
	default: assert(false); return "\n";
	}
	assert(false);
}


inline bool IsNumber(char ch) { return ch >= '0' && ch <= '9'; }
inline bool IsOctDigit(char ch) { return ch >= '0' && ch <= '7'; }
inline bool IsHexLetterLowercase(char ch) { return ch >= 'a' && ch <= 'f'; }
inline bool IsHexLetterUppercase(char ch) { return ch >= 'A' && ch <= 'F'; }

int ReadIntegerOct(string_view str) {
	int res = 0;
	for (auto ch : str) {
		if (!IsOctDigit(ch)) { throw compile_error("invalid integer literal"); }
		res = res * 8 + (ch - '0');
	}
	return res;
}

int ReadIntegerDec(string_view str) {
	int res = 0;
	for (auto ch : str) {
		if (!IsNumber(ch)) { throw compile_error("invalid integer literal"); }
		res = res * 10 + (ch - '0');
	}
	return res;
}

int ReadIntegerHex(string_view str) {
	if (str.size() == 0) { throw compile_error("invalid hexadecimal number"); }
	int res = 0;
	for (auto ch : str) {
		if (IsNumber(ch)) {
			res = res * 16 + (ch - '0');
		} else if (IsHexLetterLowercase(ch)) {
			res = res * 16 + (ch - 'a') + 10;
		} else if (IsHexLetterUppercase(ch)) {
			res = res * 16 + (ch - 'A') + 10;
		} else {
			throw compile_error("invalid integer literal");
		}
	}
	return res;
}

int ReadInteger(string_view str) {
	assert(str.size() > 0);
	assert(IsNumber(str[0]));
	if (str[0] == '0') {
		if (str.size() > 1) {
			if (str[1] == 'x' || str[1] == 'X') {
				return ReadIntegerHex(str.substr(2));
			} else {
				return ReadIntegerOct(str.substr(1));
			}
		} else {
			return 0;
		}
	} else {
		return ReadIntegerDec(str);
	}
}

const SymbolInfo GetSymbolInfo(string_view name) {
	assert(name.size() > 0);
	if (IsNumber(name[0])) { return { SymbolType::Integer, KeywordType::None, ReadInteger(name) }; }
	if (KeywordType type = GetKeywordType(name); type != KeywordType::None) { return { SymbolType::Keyword, type, 0 }; }
	return { SymbolType::Identifier, KeywordType::None, 0 };
}