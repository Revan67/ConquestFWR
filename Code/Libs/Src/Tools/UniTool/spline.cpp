//
// Spline.cpp - Spline code
//

//
// Include files
//

#include "spline.h"

//
// Macros
//

#ifndef NULL
#define NULL ((void *) 0L)
#endif

//
// Routines
//

void calculateHermiteSpline(Vector *p0, Vector *p1, Vector *d0, Vector *d1, SINGLE t, Vector *result)
{
	// Calculate hermite spline, given two start points (p0, p1), and their directional vectors (d0, d1):
	
	SINGLE t2, t3;

	t2 = t * t;
	t3 = t2 * t;

	*result =	((2.0 * t3) - (3.0 * t2) + 1.0) * (*p0) + 
				((-2.0 * t3) + (3.0 * t2)) * (*p1) +
				(t3 - (2.0 * t2) + t) * (*d0) +
				(t3 - t2) * (*d1);

}

void calculateCRSpline(Vector *p0, Vector *p1, Vector *p2, Vector *p3, SINGLE t, Vector *result)
{
    // TODO: FORWARD DIFFERENCE THIS (down to third derivative)
	
    // calculateCRSpline:
	// This function calculates the Catmull-Rom spline interpolant (in 3 dimensions) for the given control points, given
	// the t interpolant. T ranges from 0.0 to 1.0 and describes where exactly the result point should be
	// generated. If t = 0.0, the resultant point is at p1, if t = 1.0, it is at p2, if it is in between,
	// then the point lies proportionally on the curve between p1 and p2
	// 
	//					.	.R	.	p2	.
	//			   . p1		                  .     .	
	//		   .										p3
	//		p0
	//
	//
	// P0 and P3 may be NULL, if so, then the algorithm assumes the point to be collinear with p1 and p2.
	//

	SINGLE t2 = t * t;
	SINGLE t3 = t * t * t;

	// Convert p0 and p3 if NULL:
	
	if (p0 == NULL)
	{
		p0->x = p1->x;
		p0->y = p1->y;
		p0->z = p1->z;
	}

	if (p3 == NULL)
	{
		p3->x = p2->x;
		p3->y = p2->y;
		p3->z = p2->z;
	}

	// Compressed matrix multiply:
	
	result->x =	((t3 * (-p0->x + (p1->x * 3) - (p2->x * 3) + p3->x)) +
				 (t2 * ((p0->x * 2) - (p1->x * 5) + (p2->x * 4) - p3->x)) +
				 (t  * (-p0->x + p2->x)) +
				 (p1->x * 2)) * 0.5;

	result->y =	((t3 * (-p0->y + (p1->y * 3) - (p2->y * 3) + p3->y)) +
				 (t2 * ((p0->y * 2) - (p1->y * 5) + (p2->y * 4) - p3->y)) +
				 (t  * (-p0->y + p2->y)) +
				 (p1->y * 2)) * 0.5;
	
	result->z =	((t3 * (-p0->z + (p1->z * 3) - (p2->z * 3) + p3->z)) +
				 (t2 * ((p0->z * 2) - (p1->z * 5) + (p2->z * 4) - p3->z)) +
				 (t  * (-p0->z + p2->z)) +
				 (p1->z * 2)) * 0.5;
	
}

