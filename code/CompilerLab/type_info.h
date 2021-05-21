#pragma once

#include <vector>


using std::vector;


enum class VarType : uchar {
	Void,
	Int,		 // number | temp variable
	IntRef,
	ArrayPtr,
};


struct VarInfo {
public:
	const VarType type;
	const bool is_number;
	const bool is_global;
	const vector<uint> array_dimension;
	const int value;
private:
	VarInfo(VarType type, bool is_number, bool is_global, vector<uint> array_dimension, int value) :
		type(type), is_number(is_number), is_global(is_global), 
		array_dimension(NormalizeArrayDimension(array_dimension)), value(value) {
	}
public:
	static VarInfo Void() { return VarInfo(VarType::Void, false, false, {}, {}); }
	static VarInfo Number(int value) { return VarInfo(VarType::Int, true, false, {}, value); }
	static VarInfo Temp(uint index) { return VarInfo(VarType::Int, false, false, {}, (int)index); }
	static VarInfo VarRef(bool is_global, uint index) { return VarInfo(VarType::IntRef, false, is_global, {}, (int)index); }
	static VarInfo ArrayPtr(vector<uint> array_dimension, uint index) { return VarInfo(VarType::ArrayPtr, false, false, array_dimension, (int)index); }
public:
	bool IsNumber() const { return is_number; }
	bool IsTemp() const { return type == VarType::Int && !is_number; }
	bool IsRValue() const { return type == VarType::Int || type == VarType::IntRef; }
	bool IsLValue() const { return type == VarType::IntRef; }
	bool IsArrayTypeSame(const vector<uint>& para) const { return type != VarType::Void && array_dimension == para; }
private:
	vector<uint> NormalizeArrayDimension(vector<uint> dimension) {
		if (!dimension.empty()) { dimension[0] = 1; }
		return dimension;
	}
};