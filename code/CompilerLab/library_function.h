#pragma once

#include "core.h"

#include <string>


using std::string_view;


constexpr uint library_func_number = 8;

constexpr bool IsLibraryFunc(uint func_index) { return func_index < library_func_number; }

string_view GetLibraryFuncString(uint library_func_index);
uint GetLibraryFuncParameterCount(uint library_func_index);


struct Argument {
public:
	const ref_ptr<int> array_addr;
	union {
		const int value;
		const uint array_size;
	};
public:
	bool IsEmpty() const { return array_addr == (int*)-1; }
	bool IsInt() const { return array_addr == nullptr; }
	bool IsArray() const { return array_addr != nullptr && array_addr != (int*)-1; }
public:
	Argument() : array_addr((int*)-1), value(0) {}
	Argument(int value) : array_addr(nullptr), value(value) {}
	Argument(ref_ptr<int> array_addr, uint array_size) : array_addr(array_addr), array_size(array_size) {}
};

void CallLibraryFunc(uint library_func_index, const Argument& arg0, const Argument& arg1, int& return_value);
void LibraryInitialize();
void LibraryUninitialize();