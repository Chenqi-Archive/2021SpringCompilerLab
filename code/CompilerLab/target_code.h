#pragma once

#include "core.h"


enum class InstType : uchar {
	/// pseudo code
	Label,		// l0:
	Zero,		// .zero 8
	Word,		// .word 3

	/// pseudo instruction
	LoadImm,	//	li	rd, imm32	(SetImm+OperImm)
	Jmp,		//  j	imm32		(JmpLink / SetImm+JmpLinkReg) (r0==x0)
	Call,		//	cal	imm32		(JmpLink / SetImm+JmpLinkReg) (r0==x1(ra))
	BranchCmp,	//  bc	imm13, rs	(BranchOp) (rs2==x0)
	
	/// real instruction (r0--rd, r1--rs1, r2--rs2)
	SetImm,		//	[U]	lui	r0, imm20
	Oper,		//	[R]	op	r0,	r1, r2
	OperImm,	//	[I]	op	r0, r1, imm12
	Load,		//	[I]	lw	r0, imm12(r1)
	Store,		//	[S]	sw	imm12(r1), r2
	BranchOp,	//	[B]	bop imm13, r1, r2
	JmpLink,	//	[J]	jl	r0, imm21
	JmpLinkReg,	//	[I]	jlr	r0, imm12(r1)
};


struct Register {
public:
	const uint index;
private:
	Register(uint index) : index(index) { assert(index < 32); }
public:
	static Register Zero() { //zero
		return Register(0); 
	}
	static Register ReturnAddr() { // ra
		return Register(1); 
	}
	static Register StackPointer() { // sp
		return Register(2); 
	}
	static Register Temp(uint index) { // t0-t6
		if (index < 3) { return Register(index + 5); }
		if (index < 7) { return Register(index + 25); }
		assert(false); return Register(-1);
	}
	static Register Saved(uint index) { // s0-s11
		if (index < 2) { return Register(index + 8); }
		if (index < 12) { return Register(index + 16); }
		assert(false); return Register(-1);
	}
	static Register Argument(uint index) { // a0-a7
		if (index < 8) { return Register(index + 10); }
		assert(false); return Register(-1);
	}
	static Register ReturnValue() { // a0
		return Register(10); 
	}
public:
	uint AsTemp() const {
		if (index < 5) { assert(false); return -1; }
		if (index < 8) { return index - 5; }
		if (index < 28) { assert(false); return -1; }
		if (index < 32) { return index - 25; }
		assert(false); return -1;
	}
};


enum class ImmType : uchar {
	Symbol,
	SymbolAndImm,
};


enum class LabelType : uchar {
	Global,		// g:
	Func,		// f8:
	Label,		// l0:
};


struct Instruction {
	InstType type;
	uchar r0;
	uchar r1;
	uchar r2;
	LabelType label;
	char imme0;
	short imme1;
};

static_assert(sizeof(Instruction) == 8);


using TargetCode = vector<Instruction>;