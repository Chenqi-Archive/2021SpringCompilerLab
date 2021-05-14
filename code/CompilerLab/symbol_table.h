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
	static constexpr uint max_constexpr_array_length = 65536;
	static constexpr int local_variable_initial_value = 0xCCCCCCCC;

private:
	const bool is_const;
	const vector<uint> array_dimension;
	const vector<int> content;

private:
	static uint CalculateArrayLength(const vector<uint>& array_dimension) {
		uint length = 1;
		for (uint length_i : array_dimension) {
			assert(length_i > 0);
			uint64 current_length = (uint64)length * (uint64)length_i;
			length = (uint)current_length;
			if ((uint64)length != current_length) { throw compile_error("array too large"); }
		}
		return length;
	}

	vector<int> GetInitialContent(bool is_global) {
		if (is_const) {
			uint length = CalculateArrayLength(array_dimension);
			if (length > max_constexpr_array_length) { throw compile_error("array too large for constexpr evaluation"); }
			return vector<int>(length, is_global ? 0 : local_variable_initial_value);
		} else {
			return vector<int>();
		}
	}
	
public:
	VarEntry(bool is_const, bool is_global, vector<uint>&& array_dimension) :
		is_const(is_const), array_dimension(array_dimension), content(GetInitialContent(is_global)) {
	}
	bool IsConst() const { return is_const; }
	VarRef ReadAt(const vector<uint>& subscript) {
		if (subscript.size() > array_dimension.size()) { throw compile_error("invalid array subscript"); }
		for()

		int a[10][20][30][40];
		a[9][22];

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


using LocalVarInitializerList = vector<std::pair<uint, const ExpTree&>>;  // (index, exp_tree)