#ifndef SPLINE_H
#define SPLINE_H
//
// Spline.h - Spline related functions and classes
//

// NOTE: This was lifted from Loose Cannon

//
// Include files
//

#include "Vector.h"

//
// Global routines
//

// Spline functions: --------------------------------------------------------------------------------------------------------

// Catmull-Rom spline, given four control points, and a t value (from 0.0 to 1.0):
void	calculateCRSpline(Vector *p0, Vector *p1, Vector *p2, Vector *p3, SINGLE t, Vector *result);

// Hermite spline, given two control points, two direction vectors, and a t value:
void	calculateHermiteSpline(Vector *p0, Vector *p1, Vector *d0, Vector *d1, SINGLE t, Vector *result);

//
// Class and structure definitions
//

class Spline
{
private:
	float length;

public:
	Vector tp0, tp1, tc0, tc1;
	float t;

public:
	Spline( const Vector & p0, const Vector & p1,
				const Vector & c0, const Vector & c1 )
	{
		tc0=p0;
		tp0=p1;
		tp1=c0;
		tc1=c1;
		length=-1;
	}

	// for late assignment via operator =
	Spline( void ) { length=-1; }

	void operator =( const Spline & cs )
	{
		tp0= cs.tp0; 
		tp1= cs.tp1;
		tc0= cs.tc0;
		tc1= cs.tc1;
		length= cs.length;
	}

	float approx_length( void )
	{
		if (length < 0)
		{
			Vector p, s;
			float savedt= t;
			length=0;
			
			s= first_point();

			while (next_point((float) 1.0/1000.0, p))
			{
				float l= (p-s).magnitude_squared();
				if (l)
					length+= (float) sqrt( l );
				s=p;
			}

			t= savedt;
		}

		return length;
	}

	Vector first_point( void )
	{
		t=0;
		return tp0;
	}

	bool next_point( float step, Vector & res, float * remainder_dist= (float *) 0L )
	{
		t+=step;

		if (t < 1.0)
		{
			calculateCRSpline(&tc0, &tp0, &tp1, &tc1, t, &res);
			return true;
		}
		else
		{
			res= tp1;

			if (remainder_dist)
				*remainder_dist= (float) ((t-1.0) * approx_length());

			return false;
		}
	}
};

#endif
