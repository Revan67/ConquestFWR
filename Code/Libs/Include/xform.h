#ifndef XFORM_H
#define XFORM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 XForm.h                                  //
//                                                                          //
//               COPYRIGHT (C) 1997,1998 BY DIGITAL ANVIL, INC.             //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/Libs/Include/xform.h 5     4/28/00 11:57p Rmarr $
*/			    
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//

#ifndef MATRIX_H
#include "matrix.h"
#endif

typedef SINGLE OASIS[3][3];

#ifndef PI
  #define PI 3.14159265358979323846f
#endif

#ifndef MUL_DEG_TO_RAD
  #define MUL_DEG_TO_RAD (PI / 180.0f)
#endif

//
// 3x4 Transform class.
//
class Transform : public Matrix
{
public:
   //
   // 4x4 matrix in [row][col] order -- 4th row is implicit { 0 0 0 1 }
   //
	//SINGLE element[3][4];
	Vector translation;

	
	Transform (void)
	{
		set_identity();
	}
	Transform (const Vector & i, const Vector & j, const Vector & k) : Matrix (i, j, k) 
	{ 
		translation.zero();
	}

	Transform (const Transform & transform)
	{
	 	*this = transform;
	}

	Transform (const Matrix & matrix)
	{
		*static_cast<Matrix *>(this) = matrix;
		translation.zero();
	}

	Transform (const Matrix & rot, const Vector & trans)
	{
		*static_cast<Matrix *>(this) = rot;
		translation = trans;
	}

	Transform (const Vector & trans)
	{
		Matrix::set_identity();
		translation = trans;
	}

	// enable construction of a Transform without initialization
	explicit Transform (bool)
	{
	}

	Transform (const PersistTransform & src)
	{
		d[0][0] = src.m.e00;
		d[0][1] = src.m.e01;
		d[0][2] = src.m.e02;
		d[1][0] = src.m.e10;
		d[1][1] = src.m.e11;
		d[1][2] = src.m.e12;
		d[2][0] = src.m.e20;
		d[2][1] = src.m.e21;
		d[2][2] = src.m.e22;

		translation.x = src.v.x;
		translation.y = src.v.y;
		translation.z = src.v.z;
	}

			void		set_identity(void);
			Transform	multiply(const Transform & t) const;

//
// NOTE: See Quaternion class for easily creating a rotation about an arbitrary
// axis...
//
			void		compose_rotation(const U32 axis, const SINGLE angle); // expects degrees

			Transform	get_transpose(void) const;
			Transform	get_inverse(void) const;
			// assumes last row of 4x4 is 0 0 0 1
			Transform	get_general_inverse(SINGLE & w) const;

                        const Vector &  get_position(void) const;
			void		set_position(const SINGLE x, const SINGLE y, const SINGLE z);
			void		set_position(const Vector & pos);
			void		move_position(const SINGLE dx, const SINGLE dy, const SINGLE dz);
			void		move_position(const Vector & delta);

                        const Matrix &  get_orientation(void) const;
			void		set_orientation(const SINGLE pitch, const SINGLE roll, const SINGLE yaw); // expects degrees
			void		set_orientation(const Matrix & mat);
			void		set_orientation(const Quaternion & q);

			Vector		rotate(const Vector & v) const;
			Vector		rotate_translate(const Vector & v) const;
			Vector		inverse_rotate(const Vector & v) const;
			Vector		inverse_rotate_translate(const Vector & v) const;

	inline  Transform & operator = (const Matrix & matrix)
	{
		*static_cast<Matrix *>(this) = matrix;
		return *this;
	}

#ifdef _INC_MEMORY	
	inline bool operator == (const Transform &t) const
	{
		return ( 0 == memcmp(this, &t, sizeof(*this)) );
	}
	
	inline bool operator != (const Transform &t) const
	{
		return !(*this == t);
	}
#endif

	inline bool equal(const Transform & t, const SINGLE tolerance) const
	{
		return translation.equal( t.translation, tolerance ) && Matrix::equal( t, tolerance );
	}
	//
	// override Matrix operators so that Transform has expected behavior
	//

	friend Transform add (const Transform & m1, const Transform & m2)
	{
		return Transform(add(*((const Matrix *)&m1), *((const Matrix *)&m2)), (m1.translation + m2.translation));
	}

	friend Transform subtract (const Transform & m1, const Transform & m2)
	{
		return Transform(subtract(*((const Matrix *)&m1), *((const Matrix *)&m2)), (m1.translation - m2.translation));
	}

	inline const Transform & operator *= (const Transform & t);

	inline friend Transform operator + (const Transform &t1, const Transform & t2)
	{
		return add(t1, t2);
	}

	inline friend Transform operator - (const Transform & t1, const Transform & t2)
	{
		return subtract(t1, t2);
	}

//
// NOTE WELL that the following 2 operators ARE NOT EQUIVALENT, since
// as we all know, matrix multiplication is not commutative.
//
// mat * vec PRE-multiplies a column vector, and 
// vec * mat POST-multiplies a row vector.
//
// The former is equivalent to mul(mat, vec).
// The latter IS equivalent to transpose_mul(mat, vec). The operators
// are provided for completeness.
//
	inline friend Vector operator * (const Transform & t, const Vector & v)
	{
		return t.rotate_translate(v);
	}

	inline friend Vector operator * (const Vector & v, const Transform & t)
	{
		return t.inverse_rotate_translate(v);
	}

	inline friend Transform operator * (const Transform & t1, const Transform & t2)
	{
		return t1.multiply(t2);
	}

	//-------------------------------------------------------------------
	//
	void rotate_about_i (SINGLE angle)	// pitch
	{
		OASIS temp;
		SINGLE cosine, sine;

	//	angle *= -PI_RADIANS;		// convert to radians
		angle = -angle;
		cosine = (SINGLE)cos(angle);
		sine = (SINGLE)sin(angle);

		memcpy(temp, d, sizeof(d));
		d[0][1] = (cosine * temp[0][1]) - (sine * temp[0][2]);
		d[1][1] = (cosine * temp[1][1]) - (sine * temp[1][2]);
		d[2][1] = (cosine * temp[2][1]) - (sine * temp[2][2]);

		d[0][2] = (cosine * temp[0][2]) + (sine * temp[0][1]);
		d[1][2] = (cosine * temp[1][2]) + (sine * temp[1][1]);
		d[2][2] = (cosine * temp[2][2]) + (sine * temp[2][1]);
	}
	//-------------------------------------------------------------------
	//
	void rotate_about_j (SINGLE angle)	// yaw
	{
		OASIS temp;
		SINGLE cosine, sine;

	//	angle *= PI_RADIANS;		// convert to radians
		cosine = (SINGLE)cos(angle);
		sine = (SINGLE)sin(angle);
		memcpy(temp, d, sizeof(d));

		d[0][0] = (cosine * temp[0][0]) + (sine * temp[0][2]);
		d[1][0] = (cosine * temp[1][0]) + (sine * temp[1][2]);
		d[2][0] = (cosine * temp[2][0]) + (sine * temp[2][2]);

		d[0][2] = (cosine * temp[0][2]) - (sine * temp[0][0]);
		d[1][2] = (cosine * temp[1][2]) - (sine * temp[1][0]);
		d[2][2] = (cosine * temp[2][2]) - (sine * temp[2][0]);
	}

	void rotate_about_k (SINGLE angle)	// roll
	{
		OASIS temp;
		SINGLE cosine, sine;

	//	angle *= -PI_RADIANS;		// convert to radians
		angle = -angle;
		cosine = (SINGLE)cos(angle);
		sine = (SINGLE)sin(angle);

		memcpy(temp, d, sizeof(d));

		d[0][0] = (cosine * temp[0][0]) + (sine * temp[0][1]);
		d[1][0] = (cosine * temp[1][0]) + (sine * temp[1][1]);
		d[2][0] = (cosine * temp[2][0]) + (sine * temp[2][1]);

		d[0][1] = (cosine * temp[0][1]) - (sine * temp[0][0]);
		d[1][1] = (cosine * temp[1][1]) - (sine * temp[1][0]);
		d[2][1] = (cosine * temp[2][1]) - (sine * temp[2][0]);
	}
};

//

#endif
