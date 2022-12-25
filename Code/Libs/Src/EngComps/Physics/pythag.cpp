#include <math.h>
#define NRANSI
#include "nrutil.h"

#pragma warning(disable : 4244 4305)

float pythag(float a, float b)
{
	float absa,absb;
	absa=fabs(a);
	absb=fabs(b);
	if (absa > absb) return absa*sqrt(1.0+SQR(absb/absa));
	else return (absb == 0.0f ? 0.0f : absb*sqrt(1.0f+SQR(absa/absb)));
}
#undef NRANSI
/* (C) Copr. 1986-92 Numerical Recipes Software 7L`2s5N:Z. */

