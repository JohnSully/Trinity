// Ternac2.cpp : Defines the entry point for the console application.
//

#include <inttypes.h>
#include <assert.h>
#include <vector>
#include "InstructionSet.h"
#include "TernaryMath.h"

class Memory
{
public:
	static const int CTRITE_WIDTH = 2;
	Memory(unsigned ctrite)
	{
		m_vectrite.resize(ctrite);
	}

	Trite &operator[](Trite rgtriteaddr[2])
	{
		int iaddr = rgtriteaddr[1].to_int();
		iaddr *= Trite::TRITE_MAX;
		iaddr += rgtriteaddr[0].to_int();
		return m_vectrite[iaddr];
	}
private:
	std::vector<Trite> m_vectrite;
};

struct RegisterFile
{
	Trite accumulator;
	Trite operand;
	Trite flag;

	Trite program_counter[2];
	Trite stack_pointer[2];
	Trite index_pointer[2];
};

class Machine
{
public:
	Machine()
		: m_memory(Trite::TRITE_MAX * Trite::TRITE_MAX)
	{
		Reset();
	}

	void Reset()
	{
		m_regf.program_counter[0] = m_regf.program_counter[1] = Trite(0);
	}

	void Execute()
	{
		Trite triteOp = m_memory[m_regf.program_counter];
		Trit carry = '0';
		
		switch (triteOp.to_int())
		{
		case NAC:	// Negate Accumulator
			m_regf.accumulator = ~m_regf.accumulator;
			break;

		case AAC:	// And Accumulator
			m_regf.accumulator = m_regf.accumulator & m_regf.operand;
			break;

		case OAC:	// OR Accumulator
			m_regf.accumulator = m_regf.accumulator | m_regf.operand;
			break;

		case ADD:	// Add Accumulator
			carry = m_regf.accumulator.CarryAdd(m_regf.operand, m_regf.accumulator);
			break;

		case SUB:	// Sub Accumulator
			carry = m_regf.accumulator.CarryAdd(~m_regf.operand, m_regf.accumulator);
			break;

		case JEQ:
		{
			IncrementProgramCounter();
			Trite offset = m_memory[m_regf.program_counter];
			if (m_regf.accumulator == m_regf.operand)
				AddToDoubleTrite(m_regf.program_counter, offset);
		}
			break;

		case JMP:
		{
			IncrementProgramCounter();
			Trite offset = m_memory[m_regf.program_counter];
			AddToDoubleTrite(m_regf.program_counter, offset);
		}
			break;

		case JIX:	// Jump index reg
			m_regf.program_counter[0] = m_regf.index_pointer[0];
			m_regf.program_counter[1] = m_regf.index_pointer[1];
			break;

		case LSL:	// Load stack low
			m_regf.stack_pointer[0] = m_regf.accumulator;
			break;

		case LSH:	// Load stack hi
			m_regf.stack_pointer[1] = m_regf.accumulator;
			break;

		case SIL:	// Swap index low
			{
			Trite triteT = m_regf.index_pointer[0];
			m_regf.index_pointer[0] = m_regf.accumulator;
			m_regf.accumulator = triteT;
			}
			break;

		case SIH:	// Swap index hi
			{
			Trite triteT = m_regf.index_pointer[1];
			m_regf.index_pointer[1] = m_regf.accumulator;
			m_regf.accumulator = triteT;
			}
			break;

		case SWP:	// Swap Accumulator & operand
			{
			Trite triteT = m_regf.accumulator;
			m_regf.accumulator = m_regf.operand;
			m_regf.operand = triteT;
			}
			break;

		case LIM:	// Load Immediate
			IncrementProgramCounter();
			m_regf.accumulator = m_memory[m_regf.program_counter];
			break;

		case LIA:	// Load Indirect A
			m_regf.accumulator = m_memory[m_regf.index_pointer];
			break;

		case OUT:
			printf("%c", m_regf.accumulator.to_int());
			break;

		case IN:
			m_regf.accumulator = getchar();
			break;

		default:
			assert(false);
		}

		if (triteOp.to_int() != OPCODE::SWP)
		{
			m_regf.flag[FLAG::Carry] = carry;
			m_regf.flag[FLAG::Sign] = '0';	// TODO
			m_regf.flag[FLAG::True] = '+';
			m_regf.flag[FLAG::Zero] = (m_regf.accumulator.OrReduce() == Trit('0')) ? '+' : '-';
		}
		if (triteOp.to_int() != OPCODE::JIX)
			IncrementProgramCounter();
	}


	Memory &Memory() { return m_memory; }
private:
	void IncrementProgramCounter()
	{
		AddToDoubleTrite(m_regf.program_counter, 1);
	}

	void AddToDoubleTrite(Trite *rgtrite, Trite add)
	{
		Trit carry = rgtrite[0].CarryAdd(add, rgtrite[0]);
		Trite triteHi(0);
		triteHi[0] = carry;
		rgtrite[1].CarryAdd(triteHi, rgtrite[1]);
	}

	::Memory m_memory;
	RegisterFile m_regf;
};

void LoadListing(Memory &memory, const char *szFile)
{
	FILE *f = fopen(szFile, "rt");
	char linebuf[1024];
	char tritebuf[Trite::CTRIT + 1] = { '\0' };
	while (fgets(linebuf, 1024, f))
	{
		int ich = 0;
		// Load address
		assert(strlen(linebuf + ich) > Trite::CTRIT);
		memcpy(tritebuf, linebuf + ich, Trite::CTRIT);
		Trite rgtriteaddr[2] = { tritebuf, 0 };
		ich += Trite::CTRIT;
		assert(linebuf[ich] == ':');
		++ich;

		// Find values to load
		while (1)
		{
			while (linebuf[ich] == ' ' || linebuf[ich] == '\t')
				++ich;
			if (linebuf[ich] == '\0' || linebuf[ich] == '\n' || linebuf[ich] == ';')
				break;
			assert(strlen(linebuf + ich) > Trite::CTRIT);
			memcpy(tritebuf, linebuf + ich, Trite::CTRIT);
			memory[rgtriteaddr] = Trite(tritebuf);
			ich += Trite::CTRIT;
			assert(linebuf[ich] != '-' && linebuf[ich] != '0' && linebuf[ich] != '+');
			rgtriteaddr[0] = rgtriteaddr[0] + Trite(1);
		}

	}
	fclose(f);
}

int main(int argc, char* argv[])
{
	// Basic sanity tests
	assert(Trite(22).to_int() == 22);
	assert(Trite(-11).to_int() == -11);
	assert((Trite(0) + Trite(1)).to_int() == 1);
	assert((Trite(1) + Trite(1)).to_int() == 2);
	assert((Trite(-1) + Trite(1)) == Trite(0));
	assert(Trite("0000++") + Trite("+") == Trite("000+--"));	// Carry input test

	Machine machine;
	if (argc > 1)
		LoadListing(machine.Memory(), argv[1]);
	else
		fprintf(stderr, "Please provide a listing to execute.");

	while (1)
	{
		machine.Execute();
	}
	return 0;
}

