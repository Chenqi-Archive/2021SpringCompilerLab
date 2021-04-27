#pragma once

#include "lex_tree.h"

#include <iostream>


using std::string;
using std::cout;
using std::endl;


class LexerDebugHelper {
private:
	static constexpr uint max_level = 63;
	const string tab_padding = string(max_level, '\t');
	const string_view tab_padding_view = tab_padding;

private:
	void PrintLexBlock(const Item_Block& block, uint level) {
		std::cout << tab_padding_view.substr(0, level) << GetLeftBracket(block.bracket_type) << std::endl;
		for (auto& item : block.item_list) {
			if (item->GetType() == ItemType::Block) {
				PrintLexBlock(static_cast<const Item_Block&>(*item), level);
				continue;
			}

			std::cout << tab_padding_view.substr(0, level);

			switch (item->GetType()) {
			case ItemType::Keyword: std::cout << GetKeywordString(static_cast<const Item_Keyword&>(*item).keyword_type); break;
			case ItemType::Identifier: std::cout << static_cast<const Item_Identifier&>(*item).identifier; break;
			case ItemType::Integer: std::cout << static_cast<const Item_Integer&>(*item).number; break;
			case ItemType::Operator: std::cout << GetOperatorString(static_cast<const Item_Operator&>(*item).op); break;
			case ItemType::Comma: std::cout << ','; break;
			case ItemType::Semicolon: std::cout << ';'; break;
			default: assert(false); return;
			}

			std::cout << std::endl;
		}
		std::cout << tab_padding_view.substr(0, level) << GetRightBracket(block.bracket_type) << std::endl;
	}

public:
	void PrintLexTree(const LexTree& lex_tree) {
		PrintLexBlock(lex_tree, 0);
	}
};