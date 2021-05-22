#include "lexer.h"
#include "parser.h"
#include "analyzer.h"

#include "lexer_debug_helper.h"
#include "parser_debug_helper.h"
#include "analyzer_debug_helper.h"

#include "linear_code_interpreter.h"

#include <iostream>
#include <fstream>
#include <sstream>


const std::string ReadFileToString(const char input_file[]) {
	std::ifstream file(input_file);
	if (!file) { throw std::invalid_argument("invalid input file"); }
	std::stringstream buffer; buffer << file.rdbuf();
	return buffer.str();
}


int debug_main() {
	while (true) {
		string file; std::getline(std::cin, file);
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
		//LexerDebugHelper().PrintLexTree(lex_tree);


		Parser parser; SyntaxTree syntax_tree;
		try {
			syntax_tree = parser.ReadLexTree(lex_tree);
		} catch (compile_error& error) {
			std::cerr << "syntax error: " << error.what() << std::endl;
			continue;
		}
		//ParserDebugHelper().PrintSyntaxTree(syntax_tree);


		Analyzer analyzer; LinearCode linear_code;
		try {
			linear_code = analyzer.ReadSyntaxTree(syntax_tree);
		} catch (compile_error& error) {
			std::cerr << "semantic error: " << error.what() << std::endl;
			continue;
		}
		//AnalyzerDebugHelper().PrintLinearCode(linear_code);


		LinearCodeInterpreter interpreter; int return_value;
		try {
			return_value = interpreter.ExecuteLinearCode(linear_code);
		} catch (std::runtime_error& error) {
			std::cerr << "runtime error: " << error.what() << std::endl;
			continue;
		}
		cout << return_value << endl;
		

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