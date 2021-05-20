#pragma once

#include "core.h"

#include <string>


using std::string_view;


constexpr uint library_func_number = 8;

constexpr bool IsLibraryFunc(uint func_index) { 
	return func_index < library_func_number; 
}

string_view GetLibraryFuncString(uint library_func_index);  // defined in symbol_table.cpp