#pragma once

enum OPCODE : int
{
	// Arithmetic + Logic
	NAC	= -1,	// 0000-
	AAC,		// 00000
	OAC,		// 0000+
	ADD,		// 000+-
	SUB,		// 000+0
	UNUSED0,	// 000++	

	// Jump
	JEQ = 5,	// 00+--
	JEZ,		// 00+-0
	JMP,		// 00+-+
	JIX,		// 00++-

	// Load + store
	PSH,		// 00++0
	POP,		// 00+++
	LSL,		// 0+00-
	LSH,		// 0+000
	SIL,		// 0+00+
	SIH,		// 0+0+-
	SWP,		// 0+0+0
	LIM,		// 0+0++
	LIA,		// 0++0-

	// Terminal IO
	OUT,		// 0++00
	IN,			// 0++0+

	// Parse Only Opcodes (not executable)
	Nil = INT_MAX-1,
	INV = INT_MAX
};

enum FLAG
{
	Carry = 0,
	Sign,
	Zero,
	True,
};