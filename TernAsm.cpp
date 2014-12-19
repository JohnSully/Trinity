// TernAsm.cpp : Defines the entry point for the console application.
//

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <vector>
#include <set>
#include <map>
#include "..\Ternac2\InstructionSet.h"
#include "..\Ternac2\TernaryMath.h"

struct MnemonicMap
{
	OPCODE code;
	char mnemonic[4];
	int coperands;

	bool fRelative;
};

std::map<int, MnemonicMap> mmap = {
		// Arithmetic + Logic
		{ OPCODE::NAC, { OPCODE::NAC, "NAC", 0, false }, }, // 00000 
		{ OPCODE::AAC, { OPCODE::AAC, "AAC", 0, false }, }, // 00000
		{ OPCODE::OAC, { OPCODE::OAC, "OAC", 0, false }, }, // 0000+
		{ OPCODE::ADD, { OPCODE::ADD, "ADD", 0, false }, }, // 000+-
		{ OPCODE::SUB, { OPCODE::SUB, "SUB", 0, false }, }, // 000+0
		//{ OPCODE::UNUSED0, { OPCODE::UNUSED0, "---", 0 }, // 000++	

		// Jump
		{ OPCODE::JEQ, { OPCODE::JEQ, "JEQ", 1, true }, },	// 00+--
		{ OPCODE::JEZ, { OPCODE::JEZ, "JEZ", 1, true }, },	// 00+-0
		{ OPCODE::JMP, { OPCODE::JMP, "JMP", 1, true }, },	// 00+-+
		{ OPCODE::JIX, { OPCODE::JIX, "JIX", 0, false }, },	// 00++-

		// Load + store
		{ OPCODE::PSH, { OPCODE::PSH, "PSH", 0, false }, },	// 00++0
		{ OPCODE::POP, { OPCODE::POP, "POP", 0, false }, },	// 00+++
		{ OPCODE::LSL, { OPCODE::LSL, "LSL", 0, false }, },	// 0+00-
		{ OPCODE::LSH, { OPCODE::LSH, "LSH", 0, false }, },	// 0+000
		{ OPCODE::SIL, { OPCODE::SIL, "SIL", 0, false }, },	// 0+00+
		{ OPCODE::SIH, { OPCODE::SIH, "SIH", 0, false }, },	// 0+0+-
		{ OPCODE::SWP, { OPCODE::SWP, "SWP", 0, false }, },	// 0+0+0
		{ OPCODE::LIM, { OPCODE::LIM, "LIM", 1, false }, },	// 0+0++
		{ OPCODE::LIA, { OPCODE::LIA, "LIA", 0, false }, },	// 0++0-

		// Terminal IO
		{ OPCODE::OUT, { OPCODE::OUT, "OUT", 0, false }, },	// 0++00
		{ OPCODE::IN, { OPCODE::IN, "IN", 0, false }, },		// 0++0+
};

enum class ParseStage
{
	Label = 0,
	Instruction = 1,
	InstructionDone = 2,
	Operand = 2,
	Comment = 3,
};

class LabelRecord
{
public:
	LabelRecord(const char *rgch, size_t cch, int iop, bool fForceAbsolute = false)
		: m_strlbl(rgch, cch), m_iop(iop), fForceAbsolute(fForceAbsolute)
	{
		m_itrit = -1;
	}

	LabelRecord(const std::string &str, int iop = -1, bool fForceAbsolute = false)
		: m_strlbl(str), m_iop(iop), fForceAbsolute(fForceAbsolute)
	{}

	bool operator<(const LabelRecord &other) const
	{
		return m_strlbl < other.m_strlbl;
	}
	const std::string m_strlbl;
	const int m_iop;
	int m_itrit;
	bool fForceAbsolute = false;
};

class Operand
{
public:
	enum class Kind
	{
		Literal,
		ResolvedLiteral,
		IndirectLabel
	};
	Operand(int literal)
		: m_kind(Kind::Literal)
	{
		m_literal = literal;
	}
	Operand(const char *rgchLbl, int cchLbl, int index)
		: m_kind(Kind::IndirectLabel), m_label(rgchLbl, cchLbl), m_idx(index)
	{}

	void ResolveIndirectLabel(const std::set<LabelRecord> &setlbl, bool fRelative, int addrcur)
	{
		auto itr = setlbl.find(LabelRecord(m_label));
		if (itr == setlbl.end())
		{
			fprintf(stderr, "Label %s not defined\n", m_label.c_str());
			exit(0);
		}
		m_kind = Kind::ResolvedLiteral;
		int addrop = itr->m_iop;
		if (fRelative && !itr->fForceAbsolute)
			addrop -= addrcur + 2;
		if (m_idx == 0)
			m_literal = addrop;
		if (m_idx == 1)
			m_literal = 0;	// TODO, handle high trite labels
	}

	Trite Literal()
	{
		assert(m_kind == Kind::Literal || m_kind == Kind::ResolvedLiteral);
		return Trite(m_literal);
	}

	int LabelIndex() { return m_idx; }

	const std::string &Label(){ return m_label; }

	Kind m_kind;
protected:
	int m_literal;
	// Label
	std::string m_label;
	int m_idx;
};

struct InstructionRecord
{
	OPCODE code;
	std::vector<Operand> vecoperand;
};

bool FMatchSubstr(const char *sz1, const char *sz2)
{
	if (*sz1 == '\0' || *sz2 == '\0')
		return false;	// we don't match empty strings
	while (*sz1 != '\0' && *sz2 != '\0')
	{
		if (*sz1 != *sz2)
			return false;
		++sz1;
		++sz2;
	}
	return true;
}

OPCODE ParseOp(const char *sz, size_t *pich)
{
	while (sz[*pich] == ' ' || sz[*pich] == '\t')
		++(*pich);

	if (sz[*pich] == '\0' || sz[*pich] == '\n')
		return OPCODE::Nil;

	// On non-whitespace, try to match opcode
	for (auto itr : mmap)
	{
		if (FMatchSubstr(sz + *pich, itr.second.mnemonic))
		{
			(*pich) += strlen(itr.second.mnemonic);
			return itr.second.code;
		}
	}
	return OPCODE::INV;
}

void ParseError(const char *sz, int line)
{
	fprintf(stderr, "Error: %s line %d", sz, line);
	exit(0);
}

bool FLabelCh(char ch, bool fFirstCh)
{
	if (ch >= 'a' && ch <= 'z')
		return true;
	if (ch >= 'A' && ch <= 'Z')
		return true;
	if (!fFirstCh && ch >= '0' && ch <= '9')
		return true;
	return false;
}

bool FNumeralCh(char ch)
{
	return (ch >= '0' && ch <= '9');
}

void ParseLabelOperand(InstructionRecord &insrec, const char *szLine, size_t *pich, int line)
{
	bool fFirst = true;
	size_t ichLblEnd = *pich;
	while (FLabelCh(szLine[ichLblEnd], fFirst))
	{
		++ichLblEnd;
		fFirst = false;
	}

	if (ichLblEnd == *pich)
		ParseError("Expected label operand", line);

	switch (szLine[ichLblEnd])
	{
	case '[':
		if (szLine[ichLblEnd + 1] != '0' && szLine[ichLblEnd + 1] != '1')
		{
			ParseError("Expected label index between 0-1", line);
		}
		insrec.vecoperand.push_back(Operand(szLine + *pich, ichLblEnd - *pich, szLine[ichLblEnd + 1] - '0'));
		ichLblEnd += 2;
		if (szLine[ichLblEnd] != ']')
			ParseError("Expected ']'", line);
		++ichLblEnd;
		break;

	default:
		insrec.vecoperand.push_back(Operand(szLine + *pich, ichLblEnd - *pich, 0));
	}
	*pich = ichLblEnd;
}

void ParseAsciiLiteralOperand(InstructionRecord &insrec, const char *szLine, size_t *pich, int line)
{
	assert(szLine[*pich] == '\'');
	++(*pich);
	if (szLine[*pich] == '\0' || szLine[*pich] == '\n')
		ParseError("Unexpected EOL parsing char literal", line);
	insrec.vecoperand.push_back(Operand(szLine[*pich]));
	++(*pich);
	if (szLine[*pich] != '\'')
		ParseError("Expected end-quote for char literal", line);
	++(*pich);
}

void ParseNumericLiteralOperand(InstructionRecord &insrec, const char *szLine, size_t *pich, int line)
{
	assert(FNumeralCh(szLine[*pich]));
	size_t ichEnd = *pich;
	while (FNumeralCh(szLine[ichEnd]))
		++ichEnd;
	int val = strtol(szLine + *pich, nullptr, 10);
	insrec.vecoperand.push_back(Operand(val));
	*pich = ichEnd;
}

void ParseOperand(InstructionRecord &insrec, const char *szLine, size_t *pich, int line)
{
	while (szLine[*pich] == ' ' || szLine[*pich] == '\t')
		++(*pich);

	if (FNumeralCh(szLine[*pich]))
	{
		ParseNumericLiteralOperand(insrec, szLine, pich, line);
		return;
	}

	// Onto our first char
	switch (szLine[*pich])
	{
	case '\0':
		if (mmap[insrec.code].coperands != insrec.vecoperand.size())
		{
			ParseError("Unexpected EOL looking for operands", line);
		}

	case '\'':
		ParseAsciiLiteralOperand(insrec, szLine, pich, line);
		break;

	default:
		ParseLabelOperand(insrec, szLine, pich, line);
	}
}

int main(int argc, char* argv[])
{
	Trite triteT(0);
	while (triteT.CarryAdd(Trite(1), triteT) == Trit('0'));
	while (triteT.CarryAdd(Trite(-1), triteT) == Trit('0'));

	char linebuf[1024];
	int line = 0;
	FILE *f = fopen(argv[1], "rt");

	std::vector<InstructionRecord> vecinstrec;
	std::set<LabelRecord> setlrec;

	// Add builtin labels
		//	Carry 000+
		//  Sign  00+0
		//  Zero  0+00
		//  True  +000
	setlrec.insert(LabelRecord(std::string("C"), Trite("000+").to_int(), true));
	setlrec.insert(LabelRecord(std::string("S"), Trite("00+0").to_int(), true));
	setlrec.insert(LabelRecord(std::string("Z"), Trite("0+00").to_int(), true));
	setlrec.insert(LabelRecord(std::string("T"), Trite("+000").to_int(), true));

	size_t lbloffset = 0;
	while (fgets(linebuf, 1024, f))
	{
		ParseStage stage = ParseStage::Label;
		size_t ich = 0;
		++line;

		while (linebuf[ich] != '\0')
		{
			// Compute stage change
			switch (linebuf[ich])
			{
			case '\n':
				++ich;
				continue;

			case ' ':
			case '\t':
				if (stage == ParseStage::Label)
				{
					stage = ParseStage::Instruction;
				}
				else if (stage == ParseStage::InstructionDone)
				{
					stage = ParseStage::Operand;
				}
				++ich;
				continue;

			case ';':
				stage = ParseStage::Comment;
				goto LNextLine;
				break;

			case ':':
				if (stage != ParseStage::Label)
					ParseError("Unexpected ';'", line);
				// Add the label
				assert(lbloffset >= vecinstrec.size());
				setlrec.insert(LabelRecord(linebuf, ich, lbloffset));
				stage = ParseStage::Instruction;
				++ich;
				break;

			case ',':
				if (stage != ParseStage::Operand)
					ParseError("Unexpected ','", line);
				++ich;
				break;

			default:
				;
			}

			// Parse Stage
			switch (stage)
			{
			case ParseStage::Instruction:
				{
				OPCODE op = ParseOp(linebuf, &ich);
				if (op == OPCODE::INV)
					ParseError("Unknown opcode", line);
				if (op != OPCODE::Nil)
				{
					vecinstrec.push_back({ op });
					lbloffset += mmap[op].coperands + 1;
				}
				stage = ParseStage::InstructionDone;
				break;
				}

			case ParseStage::Operand:
				ParseOperand(vecinstrec.back(), linebuf, &ich, line);
				break;

			default:
				++ich;
			}
		}
	LNextLine:
		;
	}

	// Resolve Label Operands
	int addrcur = 0;
	for (InstructionRecord &rec : vecinstrec)
	{
		bool fRelative = mmap[rec.code].fRelative;
		for (Operand &operand : rec.vecoperand)
		{
			switch (operand.m_kind)
			{
			case Operand::Kind::IndirectLabel:
				operand.ResolveIndirectLabel(setlrec, fRelative, addrcur);
			}
		}
		addrcur += rec.vecoperand.size() + 1;
	}

	// Now output
	Trite triteAddr(0);
	for (InstructionRecord &rec : vecinstrec)
	{
		printf("%s: %s ", triteAddr.str().c_str(), Trite(rec.code).str().c_str());
		for (Operand &operand : rec.vecoperand)
		{
			printf("%s ", operand.Literal().str().c_str());
		}

		printf("\t;%s ", mmap[rec.code].mnemonic);
		for (Operand &operand : rec.vecoperand)
		{
			switch (operand.m_kind)
			{
			case Operand::Kind::ResolvedLiteral:
				printf("%s[%d]:(%s) ", operand.Label().c_str(), operand.LabelIndex(), operand.Literal().str().c_str());
				break;

			default:
				printf("%s ", operand.Literal().str().c_str());
			}
		}
		printf("\n");
		triteAddr = triteAddr + Trite(mmap[rec.code].coperands + 1);
	}
}

