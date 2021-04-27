#include "lexer.h"
#include "parser.h"
#include "semantic_checker.h"

#include "lexer_debug_helper.h"
#include "parser_debug_helper.h"

#include <iostream>
#include <fstream>
#include <sstream>


const std::string ReadFileToString(const char input_file[]) {
	std::ifstream file(input_file);
	if (!file) { throw std::invalid_argument("invalid input file"); }
	// inefficient here
	std::stringstream buffer; buffer << file.rdbuf();
	return buffer.str();
}

void WriteFileFromString(const char output_file[], const std::string& str) {
	std::ofstream file(output_file);
	if (!file) { throw std::invalid_argument("invalid output file"); }
	file << str;
}


int debug_main() {
	while (true) {
		string file; std::cin >> file;
		string input;
		try {
			input = ReadFileToString(file.c_str());
		} catch (std::invalid_argument& error) {
			std::cerr << error.what() << std::endl;
			continue;
		}


		Lexer lexer; LexTree lex_tree;
		try {
			lex_tree = lexer.ReadString(input);
		} catch (compile_error& error) {
			std::cerr << "lex error: " << error.what() << std::endl;
			continue;
		}
		//LexerDebugHelper lexer_debug_helper;
		//lexer_debug_helper.PrintLexTree(lex_tree);


		Parser parser; SyntaxTree syntax_tree;
		try {
			syntax_tree = parser.ReadLexTree(lex_tree);
		} catch (compile_error& error) {
			std::cerr << "syntax error: " << error.what() << std::endl;
			continue;
		}
		ParserDebugHelper parser_debug_helper;
		parser_debug_helper.PrintSyntaxTree(syntax_tree);


		SemanticChecker semantic_checker;
		try {
			semantic_checker.CheckSyntaxTree(syntax_tree);
		} catch (compile_error& error) {
			std::cerr << "semantic error: " << error.what() << std::endl;
			continue;
		}


	}
}


// Usage: 
// $ compiler -S testcase.c -o testcase.S
int main(int argc, const char* argv[]) {

	// debug
	return debug_main();
	// debug end

	try {
		if (argc != 5) { throw std::invalid_argument("invalid argument count"); }

		const char* input_file = argv[2];
		const char* output_file = argv[4];


	} catch (std::invalid_argument& error) {
		std::cerr << "Error: " << error.what();
		return 0;
	}

	return 0;
}