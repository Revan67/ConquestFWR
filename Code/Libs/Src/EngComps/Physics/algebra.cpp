#include <math.h>
#include "algebra.h"

//---------------------------------------------------------------------------
unsigned mgcAlgebraicRoots::
QuadraticRoots (float c[2], float root[2])
{
	// compute real roots to x^2+c[1]*x+c[0] = 0

	unsigned numberOfRoots = 0;
	float discr = c[1]*c[1]-4.0f*c[0];
	if ( fabs(discr) <= tolerance )
		discr = 0.0f;

	if ( discr >= 0.0f ) 
	{
		discr = float(sqrt(discr));
		root[0] = 0.5f*(-c[1]-discr);
		root[1] = 0.5f*(-c[1]+discr);
		numberOfRoots = 2;
	}

	return numberOfRoots;
}
//---------------------------------------------------------------------------
unsigned mgcAlgebraicRoots::
CubicRoots (float c[3], float root[3])
{
	// compute real roots to x^3+c[2]*x^2+c[1]*x+c[0] = 0

	const float third = 1.0f/3.0f;
	const float sqrt3 = float(sqrt(3.0));
	const float twentySeventh = 1.0f/27.0f;

	// convert to y^3+a*y+b = 0 by x = y-c[2]/3 and
	float a = third*(3*c[1]-c[2]*c[2]);
	float b = twentySeventh*(2.0f*c[2]*c[2]*c[2]-9.0f*c[1]*c[2]+27.0f*c[0]);
	float offset = third*c[2];

	unsigned numberOfRoots;
	float discr = 0.25f*b*b+twentySeventh*a*a*a;
	if ( fabs(discr) <= tolerance )
		discr = 0.0f;

	float halfb = 0.5f*b;
	if ( discr > 0.0f )  // 1 real, 2 complex roots
	{
		discr = float(sqrt(discr));
		float temp = -halfb + discr;
		if ( temp >= 0.0f )
			root[0] = float(pow(temp,third));
		else
			root[0] = -float(pow(-temp,third));
		temp = -halfb - discr;
		if ( temp >= 0.0f )
			root[0] += float(pow(temp,third));
		else
			root[0] -= float(pow(-temp,third));
		root[0] -= offset;
		numberOfRoots = 1;
	}
	else if ( discr < 0.0f ) 
	{
		float dist = float(sqrt(-third*a));
		float angle = third*float(atan2(sqrt(-discr),-halfb));
		float cs = float(cos(angle)), sn = float(sin(angle));
		root[0] = 2.0f*dist*cs-offset;
		root[1] = -dist*(cs+sqrt3*sn)-offset;
		root[2] = -dist*(cs-sqrt3*sn)-offset;
		numberOfRoots = 3;
	}
	else 
	{
		float temp;
		if ( halfb >= 0.0f )
			temp = -float(pow(halfb,third));
		else
			temp = float(pow(-halfb,third));
		root[0] = 2.0f*temp-offset;
		root[1] = -temp-offset;
		root[2] = root[1];
		numberOfRoots = 3;
	}

	return numberOfRoots;
}
//----------------------------------------------------------------------------
unsigned mgcAlgebraicRoots::
QuarticRoots (float c[4], float root[4])
{
	// compute real roots to x^4+c[3]*x^3+c[2]*x^2+c[1]*x+c[0] = 0

	// reduction to resolvent cubic polynomial
	float resolve[3];
	resolve[2] = -c[2];
	resolve[1] = c[3]*c[1]-4.0f*c[0];
	resolve[0] = -c[3]*c[3]*c[0]+4.0f*c[2]*c[0]-c[1]*c[1];
	float resolveRoot[3];
	CubicRoots(resolve,resolveRoot);
	float y = resolveRoot[0];

	unsigned numberOfRoots = 0;
	float discr = 0.25f*c[3]*c[3]-c[2]+y;
	if ( fabs(discr) <= tolerance )
		discr = 0.0f;

	if ( discr > 0.0f ) 
	{
		float R = float(sqrt(discr));
		float t1 = 0.75f*c[3]*c[3]-R*R-2.0f*c[2];
		float t2 = (4.0f*c[3]*c[2]-8.0f*c[1]-c[3]*c[3]*c[3])/(4.0f*R);

		// Threshold of t1+t2 and t1-t2 made the code more robust,
		// thanks to Greg W. Burgreen (burgre@ferdinand.surgery.pitt.edu)
		float tplus = t1+t2;
		float tminus = t1-t2;
		if ( fabs(tplus) <= tolerance ) 
			tplus = 0.0f;
		if ( fabs(tminus) <= tolerance ) 
			tminus = 0.0f;

		if ( tplus >= 0.0f ) 
		{
			float D = float(sqrt(tplus));
			root[0] = -0.25f*c[3]+0.5f*(R+D);
			root[1] = -0.25f*c[3]+0.5f*(R-D);
			numberOfRoots += 2;
		}
		if ( tminus >= 0.0f ) 
		{
			float E = float(sqrt(tminus));
			root[numberOfRoots++] = -0.25f*c[3]+0.5f*(E-R);
			root[numberOfRoots++] = -0.25f*c[3]-0.5f*(R+E);
		}
	}
	else if ( discr < 0.0f )
	{
		numberOfRoots = 0;
	}
	else
	{
		float t2 = y*y-4.0f*c[0];
		if ( t2 >= -tolerance ) 
		{
			if ( t2 < 0.0f ) // round to zero
				t2 = 0.0f;
			t2 = float(2.0*sqrt(t2));
			float t1 = 0.75f*c[3]*c[3]-2.0f*c[2];
			if ( t1+t2 >= tolerance ) 
			{
				float D = float(sqrt(t1+t2));
				root[0] = -0.25f*c[3]+0.5f*D;
				root[1] = -0.25f*c[3]-0.5f*D;
				numberOfRoots += 2;
			}
			if ( t1-t2 >= tolerance ) 
			{
				float E = float(sqrt(t1-t2));
				root[numberOfRoots++] = -0.25f*c[3]+0.5f*E;
				root[numberOfRoots++] = -0.25f*c[3]-0.5f*E;
			}
		}
	}

	return numberOfRoots;
}
//----------------------------------------------------------------------------
int mgcAlgebraicRoots::
AllRealPartsNegative (int n, float c[])
{
	// The degree of the polynomial is n.
	// Leading coefficient must be c[n] = 1.
	// The array values c[k] are changed by the code.

	if ( c[n-1] <= 0 )
		return 0;
	if ( n == 1 )
		return 1;

	float* b = new float[n];
	b[0] = 2*c[0]*c[n-1];
	int k;
	for (k = 1; k <= n-2; k++) 
	{
		b[k] = c[n-1]*c[k];
		if ( ((n-k) % 2) == 0 )
			b[k] -= c[k-1];
		b[k] *= 2;
	}
	b[n-1] = 2*c[n-1]*c[n-1];

	int degree;
	for (degree = n-1; degree >= 0; degree--)
		if ( b[degree] != 0 )
			break;
	for (k = 0; k <= degree-1; k++)
		c[k] = b[k]/b[degree];
	delete[] b;

	return AllRealPartsNegative(degree,c);
}
//----------------------------------------------------------------------------
int mgcAlgebraicRoots::
AllRealPartsPositive (int n, float c[])
{
	// reflect z -> -z
	int sign = -1;
	for (int k = n-1; k >= 0; k--, sign *= -1)
		c[k] *= sign;

	// reflection allows call to test if all real parts are negative
	return AllRealPartsNegative(n,c);
}
//---------------------------------------------------------------------------
unsigned mgcAlgebraicRootsD::
QuadraticRoots (double c[2], double root[2])
{
	// compute real roots to x^2+c[1]*x+c[0] = 0

	unsigned numberOfRoots = 0;
	double discr = c[1]*c[1]-4.0*c[0];
	if ( fabs(discr) <= tolerance )
		discr = 0.0;

	if ( discr >= 0.0 ) 
	{
		discr = sqrt(discr);
		root[0] = 0.5*(-c[1]-discr);
		root[1] = 0.5*(-c[1]+discr);
		numberOfRoots = 2;
	}

	return numberOfRoots;
}
//---------------------------------------------------------------------------
unsigned mgcAlgebraicRootsD::
CubicRoots (double c[3], double root[3])
{
	// compute real roots to x^3+c[2]*x^2+c[1]*x+c[0] = 0

	const double third = 1.0/3.0;
	const double sqrt3 = sqrt(3.0);
	const double twentySeventh = 1.0/27.0;

	// convert to y^3+a*y+b = 0 by x = y-c[2]/3 and
	double a = third*(3.0*c[1]-c[2]*c[2]);
	double b = twentySeventh*(2.0*c[2]*c[2]*c[2]-9.0*c[1]*c[2]+27.0*c[0]);
	double offset = third*c[2];

	unsigned numberOfRoots;
	double discr = 0.25*b*b+twentySeventh*a*a*a;
	if ( fabs(discr) <= tolerance )
		discr = 0.0;

	double halfb = 0.5*b;
	if ( discr > 0.0 )  // 1 real, 2 complex roots
	{
		discr = sqrt(discr);
		double temp = -halfb + discr;
		root[0] = ( temp >= 0.0 ? pow(temp,third) : -pow(-temp,third) );
		temp = -halfb - discr;
		root[0] += ( temp >= 0.0 ? pow(temp,third) : -pow(-temp,third) );
		root[0] -= offset;
		numberOfRoots = 1;
	}
	else if ( discr < 0 ) 
	{
		double dist = sqrt(-third*a);
		double angle = third*atan2(sqrt(-discr),-halfb);
		double cs = cos(angle), sn = sin(angle);
		root[0] = 2.0*dist*cs-offset;
		root[1] = -dist*(cs+sqrt3*sn)-offset;
		root[2] = -dist*(cs-sqrt3*sn)-offset;
		numberOfRoots = 3;
	}
	else 
	{
		double temp = (halfb >= 0.0 ? -pow(halfb,third) : pow(-halfb,third));
		root[0] = 2*temp-offset;
		root[1] = -temp-offset;
		root[2] = root[1];
		numberOfRoots = 3;
	}

	return numberOfRoots;
}
//----------------------------------------------------------------------------
unsigned mgcAlgebraicRootsD::
QuarticRoots (double c[4], double root[4])
{
	// compute real roots to x^4+c[3]*x^3+c[2]*x^2+c[1]*x+c[0] = 0

	// reduction to resolvent cubic polynomial
	double resolve[3];
	resolve[2] = -c[2];
	resolve[1] = c[3]*c[1]-4.0*c[0];
	resolve[0] = -c[3]*c[3]*c[0]+4.0*c[2]*c[0]-c[1]*c[1];
	double resolveRoot[3];
	CubicRoots(resolve,resolveRoot);
	double y = resolveRoot[0];

	unsigned numberOfRoots = 0;
	double discr = 0.25*c[3]*c[3]-c[2]+y;
	if ( fabs(discr) <= tolerance )
		discr = 0.0;

	if ( discr > 0.0 ) 
	{
		double R = sqrt(discr);
		double t1 = 0.75*c[3]*c[3]-R*R-2.0*c[2];
		double t2 = (4.0*c[3]*c[2]-8.0*c[1]-c[3]*c[3]*c[3])/(4.0*R);

		// Threshold of t1+t2 and t1-t2 made the code more robust,
		// thanks to Greg W. Burgreen (burgre@ferdinand.surgery.pitt.edu)
		double tplus = t1+t2;
		double tminus = t1-t2;
		if ( fabs(tplus) <= tolerance ) 
			tplus = 0.0;
		if ( fabs(tminus) <= tolerance ) 
			tminus = 0.0;

		if ( tplus >= 0.0 ) 
		{
			double D = sqrt(tplus);
			root[0] = -0.25*c[3]+0.5*(R+D);
			root[1] = -0.25*c[3]+0.5*(R-D);
			numberOfRoots += 2;
		}
		if ( tminus >= 0.0 ) 
		{
			double E = sqrt(tminus);
			root[numberOfRoots++] = -0.25*c[3]+0.5*(E-R);
			root[numberOfRoots++] = -0.25*c[3]-0.5*(R+E);
		}
	}
	else if ( discr < 0.0 )
	{
		numberOfRoots = 0;
	}
	else
	{
		double t2 = y*y-4.0*c[0];
		if ( t2 >= -tolerance ) 
		{
			if ( t2 < 0.0 ) // round to zero
				t2 = 0.0;
			t2 = 2.0*sqrt(t2);
			double t1 = 0.75*c[3]*c[3]-2.0*c[2];
			if ( t1+t2 >= tolerance ) 
			{
				double D = sqrt(t1+t2);
				root[0] = -0.25*c[3]+0.5*D;
				root[1] = -0.25*c[3]-0.5*D;
				numberOfRoots += 2;
			}
			if ( t1-t2 >= tolerance ) 
			{
				double E = sqrt(t1-t2);
				root[numberOfRoots++] = -0.25*c[3]+0.5*E;
				root[numberOfRoots++] = -0.25*c[3]-0.5*E;
			}
		}
	}

	return numberOfRoots;
}
//----------------------------------------------------------------------------
int mgcAlgebraicRootsD::
AllRealPartsNegative (int n, double c[])
{
	// The degree of the polynomial is n.
	// Leading coefficient must be c[n] = 1.
	// The array values c[k] are changed by the code.

	if ( c[n-1] <= 0 )
		return 0;
	if ( n == 1 )
		return 1;

	double* b = new double[n];
	b[0] = 2*c[0]*c[n-1];
	int k;
	for (k = 1; k <= n-2; k++) 
	{
		b[k] = c[n-1]*c[k];
		if ( ((n-k) % 2) == 0 )
			b[k] -= c[k-1];
		b[k] *= 2;
	}
	b[n-1] = 2*c[n-1]*c[n-1];

	int degree;
	for (degree = n-1; degree >= 0; degree--)
		if ( b[degree] != 0 )
			break;
	for (k = 0; k <= degree-1; k++)
		c[k] = b[k]/b[degree];
	delete[] b;

	return AllRealPartsNegative(degree,c);
}
//----------------------------------------------------------------------------
int mgcAlgebraicRootsD::
AllRealPartsPositive (int n, double c[])
{
	// reflect z -> -z
	int sign = -1;
	for (int k = n-1; k >= 0; k--, sign *= -1)
		c[k] *= sign;

	// reflection allows call to test if all real parts are negative
	return AllRealPartsNegative(n,c);
}
//===========================================================================

#ifdef ALGEBRA_TEST

#include <iostream.h>

int main ()
{
	mgcAlgebraicRootsD ar(1e-06);

	int i, count;

#if 0
	// quadratic roots
	double coeff[2] = { 2.0, 3.0 };  // x^2 + 3x + 2 = 0
	double root[2];
	count = ar.QuadraticRoots(coeff,root);

	cout << "roots = " << endl;
	for (i = 0; i < count; i++)
		cout << root[i] << endl;
	// roots = -2, -1

	cout << "all negative = " << ar.AllRealPartsNegative(2,coeff) << endl;
	cout << "all positive = " << ar.AllRealPartsPositive(2,coeff) << endl;
#endif

#if 0
	// cubic roots
	double coeff[3] = { 1.0, 1.0, 1.0 };  // x^3 + x^2 + x + 1 = 0
	double root[3];
	count = ar.CubicRoots(coeff,root);

	cout << "roots = " << endl;
	for (i = 0; i < count; i++)
		cout << root[i] << endl;
	// roots = -1

	cout << "all negative = " << ar.AllRealPartsNegative(3,coeff) << endl;
	cout << "all positive = " << ar.AllRealPartsPositive(3,coeff) << endl;

	coeff[0] = 6;    // x^3 - 2x^2 - 3x + 6 = (x^2-3)(x-2)
	coeff[1] = -3;
	coeff[2] = -2;
	count = ar.CubicRoots(coeff,root);
	cout << "roots = " << endl;
	for (i = 0; i < count; i++)
		cout << root[i] << endl;
	// roots = 2, -1.73205, +1.73205

	cout << "all negative = " << ar.AllRealPartsNegative(3,coeff) << endl;
	cout << "all positive = " << ar.AllRealPartsPositive(3,coeff) << endl;

	coeff[0] = -16;    // x^3 - 9x^2 + 24x - 16 = (x-1)*x-4)^2
	coeff[1] = 24;
	coeff[2] = -9;
	count = ar.CubicRoots(coeff,root);
	cout << "roots = " << endl;
	for (i = 0; i < count; i++)
		cout << root[i] << endl;
	// roots = 1,1,4

	cout << "all negative = " << ar.AllRealPartsNegative(3,coeff) << endl;
	cout << "all positive = " << ar.AllRealPartsPositive(3,coeff) << endl;

#endif

#if 0
	// quartic roots
	double coeff[4] = { 0.1, 0.0, -1.0, 0.0 };
		// 0 = x^4 - x^2 + 0.1 = y^2 - y + 0.1 where y = x^2
		// |y| = sqrt((1+sqrt(0.6))/2), sqrt((1-sqrt(0.6))/2)
	double root[4];
	count = ar.QuarticRoots(coeff,root);

	cout << "roots = " << endl;
	for (i = 0; i < count; i++)
		cout << root[i] << endl;
	// roots = 0.941965, 0.335711, -0.335711, -0.941965

	cout << "all negative = " << ar.AllRealPartsNegative(4,coeff) << endl;
	cout << "all positive = " << ar.AllRealPartsPositive(4,coeff) << endl;

	coeff[0] = -2.0;
	coeff[1] = 1.0;
	coeff[2] = -1.0;
	coeff[3] = 1.0;
	count = ar.QuarticRoots(coeff,root);
	cout << "roots = " << endl;
	for (i = 0; i < count; i++)
		cout << root[i] << endl;
	// roots 1, -2

#endif

#if 0
	double coeff[6] = { 6, 11, 17, 13, 9, 3 };
		// P(x) = x^6 + 3x^5 + 9x^4 + 13x^3 + 17x^2 + 11x + 6
		//      = (x^2+x+1)(x^2+x+2)(x^2+x+3)
		// Roots are all complex, but real parts are all -1
	cout << "all negative = " << ar.AllRealPartsNegative(6,coeff) << endl;
#endif

	return 0;
}

#endif
