#pragma once

#include "core.h"
#include "library_function.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <list>


using std::vector;
using std::string_view;
using std::unordered_map;
using std::list;


using InitializingList = vector<std::pair<uint, int>>;


struct ArraySize {
public:
	const vector<uint> dimension;
	const uint length;
public:
	ArraySize(vector<uint>&& dimension) : dimension(std::move(dimension)), length(CalculateArrayLength()) {}
private:
	uint CalculateArrayLength();
};


using ArrayIndex = vector<uint>;


struct VarEntry : private ArraySize {
public:
	const uint index;
	const bool is_global;
	const bool is_pointer;
public:
	VarEntry(const ArraySize& array_size, uint index, bool is_global, bool is_pointer) :
		ArraySize(array_size), index(index), is_global(is_global), is_pointer(is_pointer) {
	}
public:
	bool IsArray() const { return !dimension.empty(); }
	const ArraySize& GetArraySize() const { return *this; }

	// for const variables
public:  
	static constexpr uint max_constexpr_array_length = 65536;
	const vector<int> content;
	VarEntry(const ArraySize& array_size, const InitializingList& initializing_list) :
		ArraySize(array_size), index(-1), is_global(false), is_pointer(false),
		content(GetInitialContent(length, initializing_list)) {
	}
	bool IsConst() const { return !content.empty(); }
	int ReadAtIndex(const ArrayIndex& index) const;
private:
	static vector<int> GetInitialContent(uint length, const InitializingList& initializing_list);
};


using VarSymbolTable = unordered_map<string_view, VarEntry>;
using VarSymbolTableStack = list<VarSymbolTable>;
using LocalVarIndexStack = vector<uint>;


using ParameterArraySize = ArrayIndex;
using ParameterTypeList = vector<ParameterArraySize>;

struct FuncEntry {
public:
	const uint index;
	const bool is_int;
	const ParameterTypeList parameter_type_list;
public:
	FuncEntry(uint index, bool is_int, ParameterTypeList&& parameter_type_list) :
		index(index), is_int(is_int), parameter_type_list(std::move(parameter_type_list)) {
	}
};


class FuncSymbolTable;
void InitializeLibraryFuncEntries(FuncSymbolTable& func_symbol_table);  // defined in library_function.cpp

class FuncSymbolTable : public unordered_map<string_view, FuncEntry> {
public:
	FuncSymbolTable() { InitializeLibraryFuncEntries(*this); }
};