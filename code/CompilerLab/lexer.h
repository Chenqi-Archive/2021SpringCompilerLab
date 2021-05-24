#pragma once

#include "lex_tree.h"

#include <stack>


using std::string;
using std::stack;


class Lexer {
private:
    LexTree lex_tree;
    stack<ref_ptr<Item_Block>, vector<ref_ptr<Item_Block>>> block_stack;
    ref_ptr<Item_Block> current_block = nullptr;

private:
    void InitializeBlockStack();
    void ClearBlockStack();
    LexTree GetLexTree();

private:
    void BeginBlock(char left_bracket);
    void EndBlock(char right_bracket);

private:
    void AppendItem(unique_ptr<Item_Base> item) { current_block->item_list.push_back(std::move(item)); }

private:
    void AppendWord(string_view word);
    void AppendOperator(OperatorType op);
    void AppendOperator(char op);

private:
    struct string_const_iterator {
    public:
        ref_ptr<const char> it;
        uint line_no;
    public:
        string_const_iterator(ref_ptr<const char> it) : it(it), line_no(1) {}
        char operator*() const { return *it; }
        void operator++() { if (*it == '\n') { line_no++; } it++; }
        void operator++(int) { if (*it == '\n') { line_no++; } it++; }
        operator ref_ptr<const char>() { return it; }
    };

private:
    void ReadRoundBracket(string_const_iterator& it);
    void ReadMacro(string_view str, string_const_iterator& it);

private:
    void ReadWord(string_const_iterator& it);
    void ReadOperator(string_const_iterator& it);
    void ReadCommentSingleLine(string_const_iterator& it);
    void ReadCommentMultiLine(string_const_iterator& it);
    void ReadWhiteSpace(string_const_iterator& it);

public:
    LexTree ReadString(const string& input);
};