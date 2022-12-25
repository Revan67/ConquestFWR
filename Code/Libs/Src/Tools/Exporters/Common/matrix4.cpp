#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression

#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#endif

#include "matrix4.h"

bool Matrix4::inverse(void)
{
	bool result = false;

	SINGLE det = det4x4();

	if(fabs(det) >= 0.0001f)
	{
		*this = get_adjoint();

		det = 1.0f / det;

		d[0][0] *= det;
		d[0][1] *= det;
		d[0][2] *= det;
		d[0][3] *= det;

		d[1][0] *= det;
		d[1][1] *= det;
		d[1][2] *= det;
		d[1][3] *= det;

		d[2][0] *= det;
		d[2][1] *= det;
		d[2][2] *= det;
		d[2][3] *= det;

		d[3][0] *= det;
		d[3][1] *= det;
		d[3][2] *= det;
		d[3][3] *= det;

		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}

Matrix4 Matrix4::get_adjoint(void) const
{
	#define a1 d[0][0]
	#define b1 d[0][1]
	#define c1 d[0][2]
	#define d1 d[0][3]

	#define a2 d[1][0]
	#define b2 d[1][1]
	#define c2 d[1][2]
	#define d2 d[1][3]

	#define a3 d[2][0]
	#define b3 d[2][1]
	#define c3 d[2][2]
	#define d3 d[2][3]

	#define a4 d[3][0]
	#define b4 d[3][1]
	#define c4 d[3][2]
	#define d4 d[3][3]


	return Matrix4 (
		 det3x3(b2, b3, b4,  c2, c3, c4,  d2, d3, d4),
		-det3x3(b1, b3, b4,  c1, c3, c4,  d1, d3, d4),
		 det3x3(b1, b2, b4,  c1, c2, c4,  d1, d2, d4),
		-det3x3(b1, b2, b3,  c1, c2, c3,  d1, d2, d3),

		-det3x3(a2, a3, a4,  c2, c3, c4,  d2, d3, d4),
		 det3x3(a1, a3, a4,  c1, c3, c4,  d1, d3, d4),
		-det3x3(a1, a2, a4,  c1, c2, c4,  d1, d2, d4),
		 det3x3(a1, a2, a3,  c1, c2, c3,  d1, d2, d3),

		 det3x3(a2, a3, a4,  b2, b3, b4,  d2, d3, d4),
		-det3x3(a1, a3, a4,  b1, b3, b4,  d1, d3, d4),
		 det3x3(a1, a2, a4,  b1, b2, b4,  d1, d2, d4),
		-det3x3(a1, a2, a3,  b1, b2, b3,  d1, d2, d3),

		-det3x3(a2, a3, a4,  b2, b3, b4,  c2, c3, c4),
		 det3x3(a1, a3, a4,  b1, b3, b4,  c1, c3, c4),		
		-det3x3(a1, a2, a4,  b1, b2, b4,  c1, c2, c4),
		 det3x3(a1, a2, a3,  b1, b2, b3,  c1, c2, c3)
	);

	#undef a1
	#undef b1
	#undef c1
	#undef d1

	#undef a2
	#undef b2
	#undef c2
	#undef d2

	#undef a3
	#undef b3
	#undef c3
	#undef d3

	#undef a4
	#undef b4
	#undef c4
	#undef d4
}

SINGLE Matrix4::det4x4(void) const
{
	#define a1 d[0][0]
	#define b1 d[0][1]
	#define c1 d[0][2]
	#define d1 d[0][3]

	#define a2 d[1][0]
	#define b2 d[1][1]
	#define c2 d[1][2]
	#define d2 d[1][3]

	#define a3 d[2][0]
	#define b3 d[2][1]
	#define c3 d[2][2]
	#define d3 d[2][3]

	#define a4 d[3][0]
	#define b4 d[3][1]
	#define c4 d[3][2]
	#define d4 d[3][3]


	return (  a1 * det3x3(b2, b3, b4,  c2, c3, c4,  d2, d3, d4)
			- b1 * det3x3(a2, a3, a4,  c2, c3, c4,  d2, d3, d4)
			+ c1 * det3x3(a2, a3, a4,  b2, b3, b4,  d2, d3, d4)
			- d1 * det3x3(a2, a3, a4,  b2, b3, b4,  c2, c3, c4) );

	#undef a1
	#undef b1
	#undef c1
	#undef d1

	#undef a2
	#undef b2
	#undef c2
	#undef d2

	#undef a3
	#undef b3
	#undef c3
	#undef d3

	#undef a4
	#undef b4
	#undef c4
	#undef d4
}

SINGLE det3x3(const SINGLE a1, const SINGLE a2, const SINGLE a3, 
			  const SINGLE b1, const SINGLE b2, const SINGLE b3, 
			  const SINGLE c1, const SINGLE c2, const SINGLE c3)
{
	return (  a1 * det2x2(b2, b3, c2, c3) 
			- b1 * det2x2(a2, a3, c2, c3)
			+ c1 * det2x2(a2, a3, b2, b3) );
}