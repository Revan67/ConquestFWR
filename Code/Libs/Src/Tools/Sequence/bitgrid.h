#ifndef BITGRID_H
#define BITGRID_H

//

#include "bitarray.h"

//

struct BitGrid : public BitArray
{
	BitGrid(void) : BitArray()
	{
	}

	BitGrid(int dimension)
	{
		init(dimension * dimension);
	}

	~BitGrid(void)
	{
		if(bits)
		{
			delete	[]	bits;
			bits	=NULL;
		}
	}

	void set_bit(unsigned int i, unsigned int j)
	{
		int idx	= j * dim + i;
		BitArray::set_bit(idx);
	}

	void clear_bit(unsigned int i, unsigned int j)
	{
		int idx = j * dim + i;
		BitArray::clear_bit(idx);
	}

	bool get_bit(unsigned int i, unsigned int j) const
	{
		int idx	= j * dim + i;
		return BitArray::get_bit(idx);
	}
};

//

#endif