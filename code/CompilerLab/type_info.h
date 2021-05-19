#pragma once

#include <vector>


using std::vector;


enum class VarType : uchar {
	Void,
	Number,
	LocalTemp,
#error VarRef
	Local,
	Global
};


struct VarInfo {
public:
	const VarType type;
	const vector<uint> dimension;
	const int value;
private:
	explicit VarInfo(VarType type, vector<uint> dimension, int value) :
		type(type), dimension(NormalizeArrayDimension(dimension)), value(value) {
	}
public:
	static VarInfo Void() { return VarInfo(VarType::Void, {}, {}); }
	static VarInfo Number(int value) { return VarInfo(VarType::Number, {}, value); }
	static VarInfo LocalTemp(vector<uint> dimension, uint index) { return VarInfo(VarType::LocalTemp, dimension, (int)index); }
	static VarInfo Local(vector<uint> dimension, uint index) { return VarInfo(VarType::Local, dimension, (int)index); }
	static VarInfo Global(vector<uint> dimension, uint index) { return VarInfo(VarType::Global, dimension, (int)index); }
	static VarInfo Select(bool is_global, vector<uint> dimension, uint index) {
		return is_global ? Global(dimension, index) : Local(dimension, index);
	}
public:
	bool IsNumber() const { return type == VarType::Number; }
	bool IsInt() const { return type != VarType::Void && dimension.empty(); }
	bool IsLValue() const { return (type == VarType::Local || type == VarType::Global) && dimension.empty(); }
	bool IsArrayTypeSame(const vector<uint>& para) const { return type != VarType::Void && dimension == para; }
private:
	vector<uint> NormalizeArrayDimension(vector<uint> dimension) {
		if (!dimension.empty()) { dimension[0] = 1; }
		return dimension;
	}
};