#include "lexer.h"


enum class CharType : uchar {
    None = 0,         // others
    Word = 1,         // "A-Za-z0-9_"
    Operator = 2,     // "%&*+-/<=>|"
    Comma = 3,        // ","
    Semicolon = 4,    // ";"
    WhiteSpace = 5,   // " \t\r\n"
    LeftBracket = 6,  // "([{"
    RightBracket = 7, // ")]}"
    End = 8,          // "\0"
};

inline CharType GetCharType(uchar ch) {
    static constexpr uchar character_map_table[256] = {
        /*       X0 X1 X2 X3 X4 X5 X6 X7 X8 X9 XA XB XC XD XE XF */

        //                                  \t \n       \r
        /* 0X */ 8, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 5, 0, 0,

        //       
        /* 1X */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

        //      sps !  "  #  $  %  &  '  (  )  *  +  ,  -  .  / 
        /* 2X */ 5, 2, 0, 0, 0, 2, 2, 0, 6, 7, 2, 2, 3, 2, 0, 2,

        //       0                          9  :  ;  <  =  >  ?
        /* 3X */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 4, 2, 2, 2, 0,

        //          A                                         O
        /* 4X */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

        //       P                             Z  [  \  ]  ^  _
        /* 5X */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 0, 7, 0, 1,

        //          a                                         o
        /* 6X */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

        //       p                             z  {  |  }  ~
        /* 7X */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 2, 7, 0, 0,

        // non-ASCII values initialized to 0
    };
    return static_cast<CharType>(character_map_table[ch]);
}


void Lexer::InitializeBlockStack() {
    assert(block_stack.empty());
    block_stack.push(&lex_tree);
    current_block = block_stack.top();
}

void Lexer::ClearBlockStack() {
    assert(block_stack.size() == 1);
    block_stack.pop();
    current_block = nullptr;
}

LexTree Lexer::GetLexTree() {
    if (block_stack.size() != 1) { throw compile_error("block unclosed at end of file"); }
    ClearBlockStack();
    return std::move(lex_tree);
}

void Lexer::BeginBlock(char left_bracket) {
    BracketType bracket_type;
    switch (left_bracket) {
    case '(': bracket_type = BracketType::Round; break;
    case '{': bracket_type = BracketType::Curly; break;
    case '[': bracket_type = BracketType::Square; break;
    default: assert(false); break;
    }
    AppendItem(std::make_unique<Item_Block>(bracket_type));
    block_stack.push(static_cast<Item_Block*>(current_block->item_list.back().get()));
    current_block = block_stack.top();
}

void Lexer::EndBlock(char right_bracket) {
    if (block_stack.size() == 1) { throw compile_error("bracket mismatch"); }
    BracketType bracket_type;
    switch (right_bracket) {
    case ')': bracket_type = BracketType::Round; break;
    case '}': bracket_type = BracketType::Curly; break;
    case ']': bracket_type = BracketType::Square; break;
    default: assert(false); break;
    }
    if (current_block->bracket_type != bracket_type) { throw compile_error("bracket mismatch"); }
    block_stack.pop();
    current_block = block_stack.top();
}

void Lexer::AppendWord(string_view word) {
    SymbolInfo symbol = GetSymbolInfo(word);
    switch (symbol.type) {
    case SymbolType::Keyword: AppendItem(std::make_unique<Item_Keyword>(symbol.keyword_type)); break;
    case SymbolType::Identifier: AppendItem(std::make_unique<Item_Identifier>(word)); break;
    case SymbolType::Integer: AppendItem(std::make_unique<Item_Integer>(symbol.integer)); break;
    default: assert(false); return;
    }
}

void Lexer::AppendOperator(OperatorType op) {
    AppendItem(std::make_unique<Item_Operator>(op));
}

void Lexer::AppendOperator(char op_char) {
    OperatorType op = OperatorType::None;
    switch (op_char) {
    case '+': op = OperatorType::Add; break;
    case '-': op = OperatorType::Sub; break;
    case '*': op = OperatorType::Mul; break;
    case '/': op = OperatorType::Div; break;
    case '%': op = OperatorType::Mod; break;

    case '=': op = OperatorType::Assign; break;

    case '&': throw compile_error("invalid operator &"); break;
    case '|': throw compile_error("invalid operator |"); break;
    case '!':  op = OperatorType::Not; break;

    case '<': op = OperatorType::Less; break;
    case '>': op = OperatorType::Greater; break;

    default: assert(false);
    }

    AppendOperator(op);
}

void Lexer::ReadRoundBracket(string_const_iterator& it) {
    if (GetCharType(*it) == CharType::WhiteSpace) { ReadWhiteSpace(it); }
    if (*it != '(') { throw compile_error("expected a left bracket"); } it++;
    if (GetCharType(*it) == CharType::WhiteSpace) { ReadWhiteSpace(it); }
    if (*it != ')') { throw compile_error("expected a right bracket"); } it++;
}

void Lexer::ReadMacro(string_view str, string_const_iterator& it) {
    if (str == "starttime") {
        AppendItem(std::make_unique<Item_Identifier>("_sysy_starttime"));
        auto argument_block = std::make_unique<Item_Block>(BracketType::Round);
        argument_block->item_list.push_back(std::make_unique<Item_Integer>(it.line_no));
        AppendItem(std::move(argument_block));
        ReadRoundBracket(it);
    } else if (str == "stoptime") {
        AppendItem(std::make_unique<Item_Identifier>("_sysy_stoptime"));
        auto argument_block = std::make_unique<Item_Block>(BracketType::Round);
        argument_block->item_list.push_back(std::make_unique<Item_Integer>(it.line_no));
        AppendItem(std::move(argument_block));
        ReadRoundBracket(it);
    } else {
        return AppendWord(str);
    }
}

void Lexer::ReadWord(string_const_iterator& it) {
    const char* begin = it; uint count = 0;
    for (; GetCharType(*it) == CharType::Word; it++, count++) {}
    assert(count > 0);
    ReadMacro(string_view(begin, count), it);
}

void Lexer::ReadOperator(string_const_iterator& it) {
    char current_op = *it;
    for (;;) {
        if (GetCharType(current_op) != CharType::Operator) { break; }
        it++; 
        char next_ch = *it;
        switch (current_op) {
        case '&': if (next_ch == '&') { AppendOperator(OperatorType::And); it++; return ReadOperator(it); } break;
        case '|': if (next_ch == '|') { AppendOperator(OperatorType::Or); it++; return ReadOperator(it); } break;
        case '=': if (next_ch == '=') { AppendOperator(OperatorType::Equal); it++; return ReadOperator(it); } break;
        case '!': if (next_ch == '=') { AppendOperator(OperatorType::NotEqual); it++; return ReadOperator(it); } break;
        case '<': if (next_ch == '=') { AppendOperator(OperatorType::LessEqual); it++; return ReadOperator(it); } break;
        case '>': if (next_ch == '=') { AppendOperator(OperatorType::GreaterEuqal); it++; return ReadOperator(it); } break;
        case '/':
            if (next_ch == '/') {
                it++; return ReadCommentSingleLine(it);
            } else if (next_ch == '*') {
                it++; return ReadCommentMultiLine(it);
            }
            break;
        default: break;
        }
        AppendOperator(current_op);
        current_op = next_ch;
    }
}

void Lexer::ReadCommentSingleLine(string_const_iterator& it) {
    for (; *it != '\n' && *it != '\0'; it++) {}
}

void Lexer::ReadCommentMultiLine(string_const_iterator& it) {
    bool asterisk = false;
    for (; *it != '\0'; it++) {
        if (*it == '*') {
            asterisk = true;
        } else if (*it == '/' && asterisk) {
            it++; return;
        } else {
            asterisk = false;
        }
    }
    throw compile_error("comment unclosed at end of file");
}

void Lexer::ReadWhiteSpace(string_const_iterator& it) {
    do { it++; } while (GetCharType(*it) == CharType::WhiteSpace);
}

LexTree Lexer::ReadString(const string& input) {
    InitializeBlockStack();
    string_const_iterator it = input.data();
    for (;;) {
        switch (GetCharType(*it)) {
        case CharType::Word: ReadWord(it); break;
        case CharType::Operator: ReadOperator(it); break;
        case CharType::WhiteSpace: ReadWhiteSpace(it); break;
        case CharType::Comma: AppendItem(std::make_unique<Item_Comma>()); it++; break;
        case CharType::Semicolon: AppendItem(std::make_unique<Item_Semicolon>()); it++; break;
        case CharType::LeftBracket: BeginBlock(*it); it++; break;
        case CharType::RightBracket: EndBlock(*it); it++; break;
        case CharType::End: return GetLexTree();
        default: throw compile_error(string("invalid characher: '") + *it + "'");
        }
    }
    assert(false);
}