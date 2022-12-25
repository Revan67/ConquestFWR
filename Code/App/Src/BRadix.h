//
// <radix.h> - radix sorter
// ported from andre reinvald's original
//

#ifndef RADIX_H
#define RADIX_H

#define FINT_MAGIC ((((65536.0 * 65536.0 * 16) + (65536.0 * 0.5)) * 65536.0))

inline __int32 fast_f2int(float in)
{
	double dtemp = FINT_MAGIC + in;
	return ((*(__int32 *) &dtemp) - 0x80000000);
}

namespace RadixSort
{
	U32 * sort_temp;
	U32 * index_temp;
	U32 * index_list;

	void set_sort_size(U32 n)
	{
		if (sort_temp)
		{
			delete [] sort_temp;
			sort_temp = NULL;
		}

		sort_temp = new U32[n];

		if (index_list)
		{
			delete [] index_list;
			index_list = NULL;
		}

		index_list = new U32[n];

		if (index_temp)
		{
			delete [] index_temp;
			index_temp = NULL;
		}

		index_temp = new U32[n];
	}

	void free_arrays()
	{
		if (sort_temp)
		{
			delete [] sort_temp;
			sort_temp = NULL;
		}

		if (index_list)
		{
			delete [] index_list;
			index_list = NULL;
		}

		if (index_temp)
		{
			delete [] index_temp;
			index_temp = NULL;
		}
	}
	
	// complicated expression better fits as macro (or inline in C++)
	#define byte_of(x) (((x) >> bitsOffset) & 0xff)

	void radix (short bitsOffset, U32 N, U32 * source, U32 * dest, U32 * isrc, U32 * idst)
	{
		// suppressed the need for index as it is reported in count
		U32 count[256];

		// temp vars
		U32 *cp, *sp, *isp, s, c, i, is;
		
		// faster than memset()
		cp = count;
		for (i = 256; i > 0; --i, ++cp)
			*cp = 0;

		// count occurences of every byte value
		sp = source;
		for (i = N; i > 0; --i, ++sp) 
		{
			cp = count + byte_of (*sp);
			++(*cp);
		}

		// transform count into index by summing elements and storing into same array
		s = 0;
		cp = count;
		for (i = 256; i > 0; --i, ++cp) 
		{
			c = *cp;
			*cp = s;
			s += c;
		}

		// fill dest with the right values in the right place
		// sort index list in parallel

		sp	= source;
		isp	= isrc;

		for (i = N; i > 0; --i, ++sp, ++isp) 
		{
			s = *sp;
			is = *isp;

			cp = count + byte_of (s);
			
			dest[*cp] = s;
			idst[*cp] = is;
			
			++(*cp);
		}
	}

	// n is number of ints
	// source is array of ints
	// index_list (U32[n]) will fill in a list of indices which will tell you which slot each unsorted item will be moved to
	
	void sort (U32 * source, U32 n)
	{
		// init index list

		for (U32 i = 0; i < n; i++)
		{
			index_list[i] = i;
		}

		radix (0,	n,	source,		sort_temp,	index_list,		index_temp);
		radix (8,	n,	sort_temp,	source,		index_temp,		index_list);
		radix (16,	n,	source,		sort_temp,	index_list,		index_temp);
		radix (24,	n,	sort_temp,	source,		index_temp,		index_list);

	}
}

#endif