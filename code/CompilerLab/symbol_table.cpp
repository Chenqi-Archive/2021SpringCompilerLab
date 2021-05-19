#include "symbol_table.h"


uint ArraySize::CalculateArrayLength() {
	uint length = 1;
	for (uint length_i : dimension) {
		assert(length_i > 0);
		uint64 current_length = (uint64)length * (uint64)length_i;
		length = (uint)current_length;
		if ((uint64)length != current_length) { throw compile_error("array size too large"); }
	}
	return length;
}

int VarEntry::ReadAtIndex(const ArrayIndex& index) const {
	assert(IsConst());
	if (index.size() != dimension.size()) { throw compile_error("expression must have a value type"); }
	if (index.empty()) { return content[0]; }  // a single variable
	uint current_size = length; uint current_index = 0;
	for (uint i = 0; i < index.size(); ++i) {
		if (index[i] >= dimension[i]) { throw compile_error("can not access position past the end of an array"); }
		assert(current_size % dimension[i] == 0);
		current_size = current_size / dimension[i];
		current_index += current_size * index[i];
	}
	assert(current_index < length);
	return content[current_index];
}

vector<int> VarEntry::GetInitialContent(uint length, const InitializingList& initializing_list) {
	if (length > max_constexpr_array_length) { throw compile_error("array size too large for constexpr evaluation"); }
	vector<int> content(length);
	for (auto& [index, value] : initializing_list) {
		assert(index < length);
		content[index] = value;
	}
	return content;
}


inline std::pair<string_view, FuncEntry> GetLibraryFuncEntry(uint index) {
	using std::make_pair;
	static const ParameterArraySize array_size_0({});
	static const ParameterArraySize array_size_1({ 1 });
	static const std::pair<string_view, FuncEntry> library_func_table[library_func_number] = {
		make_pair("getint", FuncEntry{ 0, true, {} }),
		make_pair("getch", FuncEntry{ 1, true, {} }),
		make_pair("getarray", FuncEntry{ 2, true, { array_size_1 } }),
		make_pair("putint", FuncEntry{ 3, false, { array_size_0 } }),
		make_pair("putch", FuncEntry{ 4, false, { array_size_0 } }),
		make_pair("putarray", FuncEntry{ 5, false, { array_size_0, array_size_1 } }),
		make_pair("_sysy_starttime", FuncEntry{ 7, false, { array_size_0 } }),
		make_pair("_sysy_stoptime", FuncEntry{ 8, false, { array_size_0 } }),
	};
	if (!IsLibraryFunc(index)) { throw std::invalid_argument("invalid library function index"); }
	return library_func_table[index];
}

string_view GetLibraryFuncString(uint library_func_index) {
	return GetLibraryFuncEntry(library_func_index).first;
}

void InitializeLibraryFuncEntries(FuncSymbolTable& func_symbol_table) {
	assert(func_symbol_table.empty());
	for (uint i = 0; i < library_func_number; ++i) {
		func_symbol_table.insert(GetLibraryFuncEntry(i));
	}
}