#ifndef BITARRAY_H
#define BITARRAY_H

//

struct BitArray
{
	int				dim;
	unsigned char *	bits;

	BitArray(void)
	{
		memset(this, 0, sizeof(*this));
	}

	BitArray(int dimension)
	{
		init(dimension);
	}

	~BitArray(void)
	{
		if(bits)
		{
			delete	[]	bits;
			bits	=NULL;
		}
	}

	void init(int dimension)
	{
		dim = dimension;

		unsigned int num_bytes = (dim >> 3) + 1;

		bits = new unsigned char[num_bytes];

		memset(bits, 0, num_bytes);
	}

	void set_bit(unsigned int idx)
	{
		int byte	= idx >> 3;
		int bit		= idx - (byte << 3);

		bits[byte] |= (1 << bit);
	}

	void clear_bit(unsigned int idx)
	{
		int byte	= idx >> 3;
		int bit		= idx - (byte << 3);

		bits[byte] &= ~(1 << bit);
	}

	bool get_bit(unsigned int idx) const
	{
		int byte	= idx >> 3;
		int bit		= idx - (byte << 3);

		return ((bits[byte] & (1 << bit)) != 0);
	}
};

//

#endif