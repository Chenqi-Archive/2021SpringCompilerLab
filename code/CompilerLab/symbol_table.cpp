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