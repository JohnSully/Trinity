#pragma once

class Trit
{
public:
	Trit()
	{
		m_ch = '0';
	}

	Trit(char ch)
	{
		switch (ch)
		{
		case '0':
		case '+':
		case '-':
			m_ch = ch;
			break;
		default:
			assert(false);
		}
	}

	explicit operator const char() const
	{
		return m_ch;
	}

	explicit operator bool() const
	{
		return m_ch == '+';
	}

	Trit operator~() const
	{
		if (m_ch == '+')
			return '-';
		if (m_ch == '-')
			return '+';
		return *this;
	}

	Trit operator&(Trit other) const
	{
		if (m_ch == '+' && other.m_ch == '+')
			return '+';
		if (m_ch == '0' || other.m_ch == '0')	// UNKNOWN
			return '0';

		return '-';
	}

	Trit operator|(Trit other) const
	{
		if (m_ch == '+' || other == '+')
			return '+';
		if (m_ch == '-' || other == '-')
			return '-';
		if (m_ch == '0' || other == '0')
			return '0';
		return '-';
	}

	Trit operator^(Trit other) const
	{
		return (*this | other) & ~(*this & other);
	}

	bool operator==(Trit other) const
	{
		return (m_ch == other.m_ch);
	}

	Trit HalfAdd(Trit other) const
	{
		if (m_ch == '+' && other.m_ch == '+')
			return '-';
		if (m_ch == '-' && other.m_ch == '-')
			return '+';
		if (m_ch == '0')
			return other;
		if (other.m_ch == '0')
			return *this;
		return '0';
	}

	Trit SaturationAdd(Trit other) const
	{
		if (m_ch == '+'  && other.m_ch != '-')
			return '+';
		else if (m_ch == '+')
			return '0';

		if (m_ch == '0')
			return other;

		if (m_ch == '-' && other.m_ch != '+')
			return '-';
		else if (m_ch == '-')
			return '0';
	}

	Trit NoDissentWQuorum(Trit other1, Trit other2) const
	{
		int cpos = (m_ch == '+') + (other1.m_ch == '+') + (other2.m_ch == '+');
		int cneg = (m_ch == '-') + (other1.m_ch == '-') + (other2.m_ch == '-');

		if (cpos >= 2 && cneg == 0)
			return '+';
		if (cneg >= 2 && cpos == 0)
			return '-';
		return '0';
	}

	Trit Carry(Trit other) const
	{
		if (m_ch == other.m_ch)
			return *this;
		return '0';
	}

	Trit IncOp() const
	{
		switch (m_ch)
		{
		case '-':
			return '0';
		case '0':
			return '+';
		case '+':
			return '-';
		}
		assert(false);
		return '0';
	}

	Trit DecOp() const
	{
		switch (m_ch)
		{
		case '-':
			return '+';
		case '0':
			return '-';
		case '+':
			return '0';
		}
		assert(false);
		return '0';
	}

private:
	char m_ch;
};

class Trite
{
	// We define a trite as 4 trits, this is for efficient binary packing to 6 bits when necessary
public:
	static const int CTRIT = 6;
	static const int TRITE_MAX = 729;

	Trite() {}
	Trite(int int8)
	{
		int ival = int8;
		bool fneg = ival < 0;
		if (fneg)
			ival = -ival;
		for (int idigit = 0; idigit < CTRIT; ++idigit)
		{
			switch (ival % 3)
			{
			case 0:
				rgdigit[idigit] = '0';
				break;

			case 1:
				rgdigit[idigit] = '+';
				break;

			case 2:
				rgdigit[idigit] = '-';
				++ival;
				break;
			}
			ival /= 3;
		}
		assert(ival == 0);
		if (fneg)
			*this = ~(*this);
	}

	Trite(const char *sz)
	{
		int cch = strlen(sz);
		assert(cch <= CTRIT);
		int itrit = 0;
		while (cch > 0)
		{
			--cch;
			rgdigit[itrit] = sz[cch];
			++itrit;
		}
	}

	int to_int() const
	{
		int iret = 0;
		for (int idigit = CTRIT - 1; idigit >= 0; --idigit)
		{
			iret *= 3;
			switch ((char)rgdigit[idigit])
			{
			case '+':
				iret++;
				break;

			case '-':
				iret--;
				break;
			}
		}
		return iret;
	}

	Trit &operator[](int idx)
	{
		assert(idx >= 0 && idx < CTRIT);
		return rgdigit[idx];
	}

	Trite operator~() const
	{
		Trite trite;
		for (int idigit = 0; idigit < CTRIT; ++idigit)
		{
			trite.rgdigit[idigit] = ~rgdigit[idigit];
		}
		return trite;
	}
	Trite operator&(const Trite &other) const
	{
		Trite trite;
		for (int idigit = 0; idigit < CTRIT; ++idigit)
			trite[idigit] = rgdigit[idigit] & other.rgdigit[idigit];
		return trite;
	}
	Trite operator|(const Trite &other) const
	{
		Trite trite;
		for (int idigit = 0; idigit < CTRIT; ++idigit)
			trite[idigit] = rgdigit[idigit] | other.rgdigit[idigit];
		return trite;
	}
	Trite operator^(const Trite &other) const
	{
		Trite trite;
		for (int idigit = 0; idigit < CTRIT; ++idigit)
			trite[idigit] = rgdigit[idigit] ^ other.rgdigit[idigit];
		return trite;
	}

	Trite operator+(const Trite &other) const
	{
		//return Trite(to_int() + other.to_int());
		Trite result;
		CarryAdd(other, result);
		return result;
	}

	Trit CarryAdd(const Trite &other, Trite &result) const
	{
		Trit carry = '0';
		Trite resultT;
		for (int idigit = 0; idigit < CTRIT; ++idigit)
		{
			Trit halfIn = rgdigit[idigit].HalfAdd(other.rgdigit[idigit]);
			resultT.rgdigit[idigit] = halfIn.HalfAdd(carry);

			carry = carry.NoDissentWQuorum(rgdigit[idigit], other.rgdigit[idigit]);
			
		}
#ifdef _DEBUG
		if (carry == '0')
		{
			if (resultT.to_int() != (this->to_int() + other.to_int()))
				assert(false);
		}
#endif
		result = resultT;
		return carry;
	}

	Trit OrReduce() const
	{
		Trit trit('0');
		for (int idigit = 0; idigit < CTRIT; ++idigit)
		{
			trit = trit | rgdigit[idigit];
		}
		return trit;
	}

	bool operator==(const Trite &other) const
	{
		for (int idigit = 0; idigit < CTRIT; ++idigit)
		{
			if (((char)rgdigit[idigit] != (char)other.rgdigit[idigit]))
				return false;
		}
		return true;
	}

	operator bool() const
	{
		return to_int() != 0;
	}

	operator int() const = delete;

	std::string str() {
		reinterpret_cast<char&>(rgdigit[CTRIT]) = '\0';
		std::string str((char*)rgdigit);
		std::reverse(str.begin(), str.end());
		return str;
	}

private:
	Trit rgdigit[CTRIT + 1];
};