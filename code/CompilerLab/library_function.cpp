#include "symbol_table.h"

#include <iostream>


using FuncPtr = void(*)(const Argument& arg0, const Argument& arg1, int& return_value);


void GetInt(const Argument& arg0, const Argument& arg1, int& return_value) {
	assert(arg0.IsEmpty() && arg1.IsEmpty());
	std::cin >> return_value;
}
void GetCh(const Argument& arg0, const Argument& arg1, int& return_value) {
	assert(arg0.IsEmpty() && arg1.IsEmpty());
	char c;
	std::cin >> c; 
	return_value = (int)c;
}
void GetArray(const Argument& arg0, const Argument& arg1, int& return_value) {
	assert(arg0.IsArray() && arg1.IsEmpty());
	int n; std::cin >> n;
	for (int i = 0; i < n; i++) {
		if (i >= (int)arg0.array_size) { throw std::runtime_error("array subscript out of range"); }
		std::cin >> arg0.array_addr[i];
	}
	return_value = n;
}
void PutInt(const Argument& arg0, const Argument& arg1, int& return_value) {
	assert(arg0.IsInt() && arg1.IsEmpty());
	std::cout << arg0.value;
}
void PutCh(const Argument& arg0, const Argument& arg1, int& return_value) {
	assert(arg0.IsInt() && arg1.IsEmpty());
	std::cout << (char)arg0.value;
}
void PutArray(const Argument& arg0, const Argument& arg1, int& return_value) {
	assert(arg0.IsInt() && arg1.IsArray());
	std::cout << arg0.value;
	for (int i = 0; i < arg0.value; i++) { 
		if (i >= (int)arg1.array_size) { throw std::runtime_error("array subscript out of range"); }
		std::cout << arg1.array_addr[i];
	}
	std::cout << std::endl;
}




void StartTime(const Argument& arg0, const Argument& arg1, int& return_value) {
	assert(arg0.IsInt() && arg1.IsEmpty());

}
void StopTime(const Argument& arg0, const Argument& arg1, int& return_value) {
	assert(arg0.IsInt() && arg1.IsEmpty());

}


void LibraryInitialize() {

}

void LibraryUninitialize() {

}


struct LibraryFuncEntry {
	string_view str;
	FuncEntry entry;
	FuncPtr ptr;
};


inline const LibraryFuncEntry& GetLibraryFuncEntry(uint index) {
	static const ParameterArraySize array_size_0({});
	static const ParameterArraySize array_size_1({ 1 });
	static const LibraryFuncEntry library_func_table[library_func_number] = {
		{"getint", FuncEntry{ 0, true, {} }, GetInt},
		{"getch", FuncEntry{ 1, true, {} }, GetCh},
		{"getarray", FuncEntry{ 2, true, { array_size_1 } }, GetArray},
		{"putint", FuncEntry{ 3, false, { array_size_0 } }, PutInt},
		{"putch", FuncEntry{ 4, false, { array_size_0 } }, PutCh},
		{"putarray", FuncEntry{ 5, false, { array_size_0, array_size_1 } }, PutArray},
		{"_sysy_starttime", FuncEntry{ 7, false, { array_size_0 } }, StartTime},
		{"_sysy_stoptime", FuncEntry{ 8, false, { array_size_0 } }, StopTime},
	};
	if (!IsLibraryFunc(index)) { throw std::invalid_argument("invalid library function index"); }
	return library_func_table[index];
}

string_view GetLibraryFuncString(uint library_func_index) {
	return GetLibraryFuncEntry(library_func_index).str;
}

void InitializeLibraryFuncEntries(FuncSymbolTable& func_symbol_table) {
	assert(func_symbol_table.empty());
	for (uint i = 0; i < library_func_number; ++i) {
		func_symbol_table.insert(std::make_pair(GetLibraryFuncEntry(i).str, GetLibraryFuncEntry(i).entry));
	}
}

uint GetLibraryFuncParameterCount(uint library_func_index) {
	return GetLibraryFuncEntry(library_func_index).entry.parameter_type_list.size();
}

void CallLibraryFunc(uint library_func_index, const Argument& arg0, const Argument& arg1, int& return_value) {
	GetLibraryFuncEntry(library_func_index).ptr(arg0, arg1, return_value);
}