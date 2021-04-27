#pragma once

#include <vector>
#include <string>
#include <unordered_map>


using std::vector;
using std::string_view;
using std::unordered_map;


struct VarEntry;

struct VarRef {
	const bool is_const;
	const vector<uint> array_dimension;
	//ref_ptr<int> content; uint content_length;
	ref_ptr<VarEntry> entry; uint content_offset;
};

struct VarEntry {
private:
	const bool is_const;
	const vector<uint> array_dimension;
	vector<int> content;

public:
	VarEntry(bool is_const, bool is_global, vector<uint>&& array_dimension) :
		is_const(is_const), array_dimension(array_dimension) {
		try {
			content.assign(CalculateArrayLength(array_dimension), is_global ? 0 : 0xCCCCCCCC);
		} catch (std::bad_alloc&) { throw compile_error("array is too large"); }
	}
	bool IsConst() const { return is_const; }
	VarRef ReadAt(const vector<uint>& subscript) {
		if (subscript.size() > array_dimension.size()) { throw compile_error("invalid array subscript"); }
		for()

		int a[10][20][30][40];
		a[9][22];

	}

private:
	static uint CalculateArrayLength(const vector<uint>& array_dimension) {
		uint length = 1;
		for (uint length_i : array_dimension) {
			assert(length_i > 0);
			uint64 current_length = (uint64)length * (uint64)length_i;
			length = (uint)current_length;
			if ((uint64)length != current_length) { throw compile_error("array is too large"); }
		}
		return length;
	}
};

using VarTable = unordered_map<string_view, VarEntry>;


struct FuncEntry {
private:
	bool is_int;
	ParameterList& parameter_list;
	Block& block;

public:
	FuncEntry(bool is_int, ParameterList& parameter_list, Block& block) :
		is_int(is_int), parameter_list(parameter_list), block(block) {
	}
};

using FuncTable = unordered_map<string_view, FuncEntry>;