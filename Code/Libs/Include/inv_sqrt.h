#ifndef __INV_SQRT
#define __INV_SQRT

/* Compute the Inverse Square Root
 * of an IEEE Single Precision Floating-Point number.
 *
 * original code from Graphics Gems V - Written by Ken Turkowski.
 */

/* Specified parameters */
#define LOOKUP_BITS    9   /* Number of mantissa bits for lookup */
#define EXP_POS       23   /* Position of the exponent */
#define EXP_BIAS     127   /* Bias of exponent */
/* The mantissa is assumed to be just down from the exponent */

#ifndef SINGLE
typedef float SINGLE;
#endif

/* Derived parameters */
#define LOOKUP_POS   (EXP_POS-LOOKUP_BITS)  /* Position of mantissa lookup */
#define SEED_POS     (EXP_POS-8)            /* Position of mantissa seed */
#define TABLE_SIZE   (2 << LOOKUP_BITS)     /* Number of entries in table */
#define LOOKUP_MASK  (TABLE_SIZE - 1)           /* Mask for table input */
#define GET_EXP(a)   (((a) >> EXP_POS) & 0xFF)  /* Extract exponent */
#define SET_EXP(a)   ((a) << EXP_POS)           /* Set exponent */
#define GET_EMANT(a) (((a) >> LOOKUP_POS) & LOOKUP_MASK)  /* Extended mantissa
                                                           * MSB's */
#define SET_MANTSEED(a) (((unsigned long)(a)) << SEED_POS)  /* Set mantissa
                                                             * 8 MSB's */

#include <stdlib.h>
#include <math.h>

union _flint {
    unsigned long    i;
    float            f;
};// _fi, _fo;

class ISQRT
{
private:
	unsigned char iSqrt[TABLE_SIZE];
	void MakeInverseSqrtLookupTable(void)
	{
		long f;
		unsigned char *h;
		union _flint fi, fo;

		h = iSqrt;
		for (f = 0, h = iSqrt; f < TABLE_SIZE; f++)
		{
			fi.i = ((EXP_BIAS-1) << EXP_POS) | (f << LOOKUP_POS);
			fo.f = 1.0f / sqrt(fi.f);
			*h++ = (unsigned char)(((fo.i + (1<<(SEED_POS-2))) >> SEED_POS) & 0xFF); /* rounding */
		}
		iSqrt[TABLE_SIZE / 2] = 0xFF;    /* Special case for 1.0 */
	}

public:
	ISQRT(void)
	{
		MakeInverseSqrtLookupTable();
	}

	~ISQRT()
	{
	}

	/* The following returns the inverse square root */
	inline SINGLE InvSqrt(const float x) const
	{
		const unsigned long a = ((union _flint*)(&x))->i;
		union _flint seed;

		/* Seed: accurate to LOOKUP_BITS */
		seed.i = SET_EXP(((3*EXP_BIAS-1) - GET_EXP(a)) >> 1) | SET_MANTSEED(iSqrt[GET_EMANT(a)]);

		/* First iteration: accurate to 2*LOOKUP_BITS */
		seed.f = (3.0f - seed.f * seed.f * x) * seed.f * 0.5f;

		/* Second iteration: accurate to 4*LOOKUP_BITS */
		seed.f = (3.0f - seed.f * seed.f * x) * seed.f * 0.5f;

		return seed.f;
	}

	inline SINGLE Sqrt(const float x) const
	{
		return x * InvSqrt(x);
	}
};

#undef LOOKUP_BITS
#undef EXP_POS
#undef EXP_BIAS

#undef LOOKUP_POS
#undef SEED_POS
#undef TABLE_SIZE
#undef LOOKUP_MASK
#undef GET_EXP
#undef SET_EXP
#undef GET_EMANT
                                                           
#undef SET_MANTSEED
                                                             
#endif