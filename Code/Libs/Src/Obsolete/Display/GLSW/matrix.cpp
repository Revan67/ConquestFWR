//---------------------------------------------------------------------------
/*
	MATRIX.CPP

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Paul Isaac
*/
//---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>	// PIXELFORMATDESCRIPTOR

#include "matrix.h"
#include "vbuffer.h"

#include "display.h"	// GLenum, etc.

#include <math.h>		// sqrt, sin, cos
#include <assert.h>		// assert()

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

float Identity[4*4] =
{
   1.0, 0.0, 0.0, 0.0,
   0.0, 1.0, 0.0, 0.0,
   0.0, 0.0, 1.0, 0.0,
   0.0, 0.0, 0.0, 1.0
};

//---------------------------------------------------------------------------
// Matrix_4x4
//---------------------------------------------------------------------------

void Matrix_4x4::scale (float x, float y, float z)
{
	m[0] *= x;	m[4] *= y;	m[ 8] *= z;
	m[1] *= x;	m[5] *= y;	m[ 9] *= z;
	m[2] *= x;	m[6] *= y;	m[10] *= z;
	m[3] *= x;	m[7] *= y;	m[11] *= z;

	type = MATRIX_MODIFIED;
}

void Matrix_4x4::translate (float x, float y, float z)
{
	m[12] += m[0]*x + m[4]*y + m[ 8]*z;
	m[13] += m[1]*x + m[5]*y + m[ 9]*z;
	m[14] += m[2]*x + m[6]*y + m[10]*z;
	m[15] += m[3]*x + m[7]*y + m[11]*z;

	type = MATRIX_MODIFIED;
}

void Matrix_4x4::analyze_view (void)
// Examine the current modelview matrix to determine its type.
// Later we use the matrix type to optimize vertex transformations.
{
	if (m[3]==0.0F && m[7]==0.0F && m[11]==0.0F && m[15]==1.0F)
	{
		if (m[2]==0.0F && m[6]==0.0F && m[10]==1.0F && m[14]==0.0F)
		{
			if(m[0]==1.0F && m[4]==0.0F && m[ 8]==0.0F && m[12]==0.0F
			&& m[1]==0.0F && m[5]==1.0F && m[ 9]==0.0F && m[13]==0.0F)
			{
				type = MATRIX_IDENTITY;
			}
			else if (        m[4]==0.0F && m[ 8]==0.0F               
			&& m[1]==0.0F               && m[ 9]==0.0F)
//				&& m[2]==0.0F && m[6]==0.0F && m[10]==1.0F && m[14]==0.0F
//				&& m[3]==0.0F && m[7]==0.0F && m[11]==0.0F && m[15]==1.0F)
			{
				type = MATRIX_2D_NO_ROT;
			}
			else if (                      m[ 8]==0.0F               
			&&                             m[ 9]==0.0F)
//				&& m[2]==0.0F && m[6]==0.0F && m[10]==1.0F && m[14]==0.0F
//				&& m[3]==0.0F && m[7]==0.0F && m[11]==0.0F && m[15]==1.0F)
			{
				type = MATRIX_2D;
			}
		}
		else
//			if (m[3]==0.0F && m[7]==0.0F && m[11]==0.0F && m[15]==1.0F)
		{
			type = MATRIX_3D;
		}
	}
	else
	{
		type = MATRIX_GENERAL;
	}
}

void Matrix_4x4::analyze_project (void)
// Examine the current projection matrix to determine its type.
// Later we use the matrix type to optimize vertex transformations.
{
	// look for common-case ortho and perspective matrices

	if(m[0]==1.0F && m[4]==0.0F && m[ 8]==0.0F && m[12]==0.0F
	&& m[1]==0.0F && m[5]==1.0F && m[ 9]==0.0F && m[13]==0.0F
	&& m[2]==0.0F && m[6]==0.0F && m[10]==1.0F && m[14]==0.0F
	&& m[3]==0.0F && m[7]==0.0F && m[11]==0.0F && m[15]==1.0F)
	{
		type = MATRIX_IDENTITY;
	}
	else if (        m[4]==0.0F && m[8] ==0.0F
	&& m[1]==0.0F               && m[9] ==0.0F
	&& m[2]==0.0F && m[6]==0.0F
	&& m[3]==0.0F && m[7]==0.0F && m[11]==0.0F && m[15]==1.0F)
	{
		type = MATRIX_ORTHO;
	}
	else if (        m[4]==0.0F                 && m[12]==0.0F
	&& m[1]==0.0F                               && m[13]==0.0F
	&& m[2]==0.0F && m[6]==0.0F
	&& m[3]==0.0F && m[7]==0.0F && m[11]==-1.0F && m[15]==0.0F)
	{
		type = MATRIX_PERSPECTIVE;
	}
	else
	{
		type = MATRIX_GENERAL;
	}
}

void Matrix_4x4::invert (const float *src)
{
	typedef float Mat2[2][2];
	Mat2 r1, r2, r3, r4, r5, r6, r7;

	const GLfloat * A = src;
	GLfloat *       C = m;
	GLfloat one_over_det;

	// A is the 4x4 source matrix (to be inverted).
	// C is the 4x4 destination matrix
	// a11 is the 2x2 matrix in the upper left quadrant of A
	// a12 is the 2x2 matrix in the upper right quadrant of A
	// a21 is the 2x2 matrix in the lower left quadrant of A
	// a22 is the 2x2 matrix in the lower right quadrant of A
	// similarly, cXX are the 2x2 quadrants of the destination matrix

	// R1 = inverse( a11 )
	one_over_det = 1.0f / ( ( A[M00] * A[M11] ) - ( A[M10] * A[M01] ) );
	r1[0][0] = one_over_det *  A[M11];
	r1[0][1] = one_over_det * -A[M01];
	r1[1][0] = one_over_det * -A[M10];
	r1[1][1] = one_over_det *  A[M00];

	// R2 = a21 x R1
	r2[0][0] = A[M20] * r1[0][0] + A[M21] * r1[1][0];
	r2[0][1] = A[M20] * r1[0][1] + A[M21] * r1[1][1];
	r2[1][0] = A[M30] * r1[0][0] + A[M31] * r1[1][0];
	r2[1][1] = A[M30] * r1[0][1] + A[M31] * r1[1][1];

	// R3 = R1 x a12
	r3[0][0] = r1[0][0] * A[M02] + r1[0][1] * A[M12];
	r3[0][1] = r1[0][0] * A[M03] + r1[0][1] * A[M13];
	r3[1][0] = r1[1][0] * A[M02] + r1[1][1] * A[M12];
	r3[1][1] = r1[1][0] * A[M03] + r1[1][1] * A[M13];

	// R4 = a21 x R3
	r4[0][0] = A[M20] * r3[0][0] + A[M21] * r3[1][0];
	r4[0][1] = A[M20] * r3[0][1] + A[M21] * r3[1][1];
	r4[1][0] = A[M30] * r3[0][0] + A[M31] * r3[1][0];
	r4[1][1] = A[M30] * r3[0][1] + A[M31] * r3[1][1];

	// R5 = R4 - a22
	r5[0][0] = r4[0][0] - A[M22];
	r5[0][1] = r4[0][1] - A[M23];
	r5[1][0] = r4[1][0] - A[M32];
	r5[1][1] = r4[1][1] - A[M33];

	// R6 = inverse( R5 )
	one_over_det = 1.0f / ( ( r5[0][0] * r5[1][1] ) - ( r5[1][0] * r5[0][1] ) );
	r6[0][0] = one_over_det * r5[1][1];
	r6[0][1] = one_over_det * -r5[0][1];
	r6[1][0] = one_over_det * -r5[1][0];
	r6[1][1] = one_over_det * r5[0][0];

	// c12 = R3 x R6
	C[M02] = r3[0][0] * r6[0][0] + r3[0][1] * r6[1][0];
	C[M03] = r3[0][0] * r6[0][1] + r3[0][1] * r6[1][1];
	C[M12] = r3[1][0] * r6[0][0] + r3[1][1] * r6[1][0];
	C[M13] = r3[1][0] * r6[0][1] + r3[1][1] * r6[1][1];

	// c21 = R6 x R2
	C[M20] = r6[0][0] * r2[0][0] + r6[0][1] * r2[1][0];
	C[M21] = r6[0][0] * r2[0][1] + r6[0][1] * r2[1][1];
	C[M30] = r6[1][0] * r2[0][0] + r6[1][1] * r2[1][0];
	C[M31] = r6[1][0] * r2[0][1] + r6[1][1] * r2[1][1];

	// R7 = R3 x c21
	r7[0][0] = r3[0][0] * C[M20] + r3[0][1] * C[M30];
	r7[0][1] = r3[0][0] * C[M21] + r3[0][1] * C[M31];
	r7[1][0] = r3[1][0] * C[M20] + r3[1][1] * C[M30];
	r7[1][1] = r3[1][0] * C[M21] + r3[1][1] * C[M31];

	// c11 = R1 - R7
	C[M00] = r1[0][0] - r7[0][0];
	C[M01] = r1[0][1] - r7[0][1];
	C[M10] = r1[1][0] - r7[1][0];
	C[M11] = r1[1][1] - r7[1][1];

	// c22 = -R6
	C[M22] = -r6[0][0];
	C[M23] = -r6[0][1];
	C[M32] = -r6[1][0];
	C[M33] = -r6[1][1];
}

void Matrix_4x4::mul (VECTOR *dst, const VECTOR *src)
{
	GLfloat ox = src->x, oy = src->y, oz = src->z;
	dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz + m[12];
	dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz + m[13];
	dst->z = m[2]*ox + m[6]*oy + m[10]*oz + m[14];
	dst->w = m[3]*ox + m[7]*oy + m[11]*oz + m[15];
}

void Matrix_4x4::mul4 (VECTOR *dst, const VECTOR *src)
{
	GLfloat ox = src->x, oy = src->y, oz = src->z, ow = src->w;
	dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz + m[12]*ow;
	dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz + m[13]*ow;
	dst->z = m[2]*ox + m[6]*oy + m[10]*oz + m[14]*ow;
	dst->w = m[3]*ox + m[7]*oy + m[11]*oz + m[15]*ow;
}

//---------------------------------------------------------------------------
// MatrixMgr
//---------------------------------------------------------------------------

MatrixMgr::MatrixMgr (void)
{
	stack_count = 0;

	ModelViewMatrix.set_identity();
	ProjectionMatrix.set_identity();
	TextureMatrix.set_identity();

	Matrix_mode = GL_MODELVIEW;

	enable_clipping = true;

	h_scale = v_scale = 0;
	h_offset = v_offset = 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::set_identity (GLenum mode)
{
	switch (mode)
	{
		case GL_MODELVIEW:
			ModelViewMatrix.set_identity();
			ModelViewInv.set_identity();
			return;

		case GL_PROJECTION:
			ProjectionMatrix.set_identity();
			return;

		case GL_TEXTURE:
			TextureMatrix.set_identity();
			return;
	}
	return; // gl_problem(ctx, "Bad matrix mode in LoadIdentity");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Matrix_4x4 *MatrixMgr::get_matrix (GLenum mode)
{
	switch (mode)
	{
		case GL_MODELVIEW:
			return &ModelViewMatrix;

		case GL_PROJECTION:
			return &ProjectionMatrix;

		case GL_TEXTURE:
			return &TextureMatrix;
	}
	assert(1); // "Bad matrix mode?"
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::load_matrix (const float *m)
{
	switch (Matrix_mode)
	{
		case GL_MODELVIEW:
			ModelViewMatrix.set(m);
			ModelViewInv.set_identity();
			break;

		case GL_PROJECTION:
			ProjectionMatrix.set(m);
			break;

		case GL_TEXTURE:
			TextureMatrix.set(m);
			break;

		default: // error?
			break;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::store_matrix (float *m)
{
	switch (Matrix_mode)
	{
		case GL_MODELVIEW:
			ModelViewMatrix.get(m);
			break;

		case GL_PROJECTION:
			ProjectionMatrix.get(m);
			break;

		case GL_TEXTURE:
			TextureMatrix.get(m);
			break;

		default: // error?
			break;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLenum MatrixMgr::push_matrix (void)
{
	uint i = stack_count;
	if (i >= MATRIX_STACK_DEPTH)
	{
		return GL_STACK_OVERFLOW;
	}

	stack_count = i+1;
	float *m = matrix_stack[i];
	store_matrix(m);
	return GL_NO_ERROR;
}

GLenum MatrixMgr::pop_matrix (void)
{
	uint i = stack_count;
	if (i < 1)
	{
		return GL_STACK_UNDERFLOW;
	}

	stack_count = --i;
	float *m = matrix_stack[i];
	load_matrix(m);
	return GL_NO_ERROR;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::rotate_translate (float *dst, float *src)
{
	#define mat ModelViewMatrix.m
	dst[0] = src[0]*M(mat,0,0) + src[1]*M(mat,1,0) + src[2]*M(mat,2,0) + M(mat,3,0);
	dst[1] = src[0]*M(mat,0,1) + src[1]*M(mat,1,1) + src[2]*M(mat,2,1) + M(mat,3,1);
	dst[2] = src[0]*M(mat,0,2) + src[1]*M(mat,1,2) + src[2]*M(mat,2,2) + M(mat,3,2);
//		dst[3] = src[0]*M(mat,0,3) + src[1]*M(mat,1,3) + src[2]*M(mat,2,3) + M(mat,3,3);
	#undef mat
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::matrix_mul (float product[16], const float a[16], const GLfloat b[16])
{
	// assert(product != b); // this will cause a problem!

	for (int i=0; i<4; i++)
	{
		float ai0=M(a,i,0), ai1=M(a,i,1), ai2=M(a,i,2), ai3=M(a,i,3);

		M(product,i,0) = ai0*M(b,0,0) + ai1*M(b,1,0) + ai2*M(b,2,0) + ai3*M(b,3,0);
		M(product,i,1) = ai0*M(b,0,1) + ai1*M(b,1,1) + ai2*M(b,2,1) + ai3*M(b,3,1);
		M(product,i,2) = ai0*M(b,0,2) + ai1*M(b,1,2) + ai2*M(b,2,2) + ai3*M(b,3,2);
		M(product,i,3) = ai0*M(b,0,3) + ai1*M(b,1,3) + ai2*M(b,2,3) + ai3*M(b,3,3);
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::frustum_matrix (float left, float right, float bottom, float top, float nearval, float farval)
{
	GLfloat x, y, a, b, c, d;
	GLfloat m[4*4];

	x = (2.0*nearval) / (right-left);
	y = (2.0*nearval) / (top-bottom);
	a = (right+left) / (right-left);
	b = (top+bottom) / (top-bottom);
	c = -(farval+nearval) / ( farval-nearval);
	d = -(2.0*farval*nearval) / (farval-nearval);  /* error? */

	M(m,0,0) = x;     M(m,0,1) = 0.0F;  M(m,0,2) = a;      M(m,0,3) = 0.0F;
	M(m,1,0) = 0.0F;  M(m,1,1) = y;     M(m,1,2) = b;      M(m,1,3) = 0.0F;
	M(m,2,0) = 0.0F;  M(m,2,1) = 0.0F;  M(m,2,2) = c;      M(m,2,3) = d;
	M(m,3,0) = 0.0F;  M(m,3,1) = 0.0F;  M(m,3,2) = -1.0F;  M(m,3,3) = 0.0F;

	matrix_mul(Matrix_mode,m);

	// FUTURE: stack of near/far to handle push/pop?
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::ortho_matrix (float left, float right, float bottom, float top, float nearval, float farval)
{
	GLfloat x, y, z;
	GLfloat tx, ty, tz;
	GLfloat m[4*4];

	x = 2.0 / (right-left);
	y = 2.0 / (top-bottom);
	z = -2.0 / (farval-nearval);
	tx = -(right+left) / (right-left);
	ty = -(top+bottom) / (top-bottom);
	tz = -(farval+nearval) / (farval-nearval);

	M(m,0,0) = x;     M(m,0,1) = 0.0F;  M(m,0,2) = 0.0F;  M(m,0,3) = tx;
	M(m,1,0) = 0.0F;  M(m,1,1) = y;     M(m,1,2) = 0.0F;  M(m,1,3) = ty;
	M(m,2,0) = 0.0F;  M(m,2,1) = 0.0F;  M(m,2,2) = z;     M(m,2,3) = tz;
	M(m,3,0) = 0.0F;  M(m,3,1) = 0.0F;  M(m,3,2) = 0.0F;  M(m,3,3) = 1.0F;

	matrix_mul(Matrix_mode,m);

//		if (ctx->Driver.NearFar) { (*ctx->Driver.NearFar)( ctx, nearval, farval ); }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::matrix_rotate (float _angle, float x, float y, float z)
{
	if (_angle == 0.0)
		return;

	GLfloat m[4*4];

	GLdouble mag, s, c;
	GLdouble xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	mag = sqrt( x*x + y*y + z*z );

	if (mag == 0.0)
	{
		// generate an identity matrix and return
		memcpy(m, Identity, sizeof(GLfloat)*16);
		return;
	}

	GLdouble inv_mag = 1.0 / mag;

	x *= inv_mag;
	y *= inv_mag;
	z *= inv_mag;

	double angle = _angle;
	angle *= DEG2RAD;	// input angle is in degrees
	s = sin(angle);
	c = cos(angle);

	//
	// Arbitrary axis rotation matrix.
	//
	//	R = Rz * Ry * T * Ry' * Rz'
	//

	one_c = 1.0F - c;

	xx = x * x * one_c;
	yy = y * y * one_c;
	zz = z * z * one_c;
	xy = x * y * one_c;
	yz = y * z * one_c;
	zx = z * x * one_c;

	xs = x * s;
	ys = y * s;
	zs = z * s;

	M(m,0,0) = (/*one_c */ xx) + c;
	M(m,0,1) = (/*one_c */ xy) - zs;
	M(m,0,2) = (/*one_c */ zx) + ys;
	M(m,0,3) = 0.0F;

	M(m,1,0) = (/*one_c */ xy) + zs;
	M(m,1,1) = (/*one_c */ yy) + c;
	M(m,1,2) = (/*one_c */ yz) - xs;
	M(m,1,3) = 0.0F;

	M(m,2,0) = (/*one_c */ zx) - ys;
	M(m,2,1) = (/*one_c */ yz) + xs;
	M(m,2,2) = (/*one_c */ zz) + c;
	M(m,2,3) = 0.0F;

	M(m,3,0) = 0.0F;
	M(m,3,1) = 0.0F;
	M(m,3,2) = 0.0F;
	M(m,3,3) = 1.0F;

	matrix_mul(Matrix_mode,m);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::transform_points (int n, VECTOR *& vEye, const VECTOR *vObj)
{
	if (ModelViewMatrix.type == MATRIX_MODIFIED)
	{
		ModelViewMatrix.analyze_view();
		ModelViewInv.invert(ModelViewMatrix);
	}

	switch (ModelViewMatrix.type)
	{
		case MATRIX_MODIFIED:
		case MATRIX_GENERAL:
		{
			const GLfloat *m = ModelViewMatrix;
			const VECTOR *src = vObj;
			VECTOR *dst = vEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->x, oy = src->y, oz = src->z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz + m[12];
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz + m[13];
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz + m[14];
				dst->w = 1; // m[3]*ox + m[7]*oy + m[11]*oz + m[15];
			}
		}
		break;

		case MATRIX_IDENTITY:
		{
		// Ack, useless copy.
			/*
			const VECTOR *src = vObj;
			VECTOR *dst = vEye;
			memcpy(dst, src, sizeof(VECTOR) * n);
			*/
			vEye = const_cast<VECTOR *>(vObj);
		}
		break;

		case MATRIX_2D:
		{
			const GLfloat *m = ModelViewMatrix;
			const VECTOR *src = vObj;
			VECTOR *dst = vEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->x, oy = src->y, oz = src->z;
				dst->x = m[0]*ox + m[4]*oy           + m[12];
				dst->y = m[1]*ox + m[5]*oy           + m[13];
				dst->z =                   +      oz        ;
				dst->w =                                1.0F;
			}
		}
		break;

		case MATRIX_2D_NO_ROT:
		{
			const GLfloat *m = ModelViewMatrix;
			const VECTOR *s = vObj;
			VECTOR *dst = vEye;
			for (int i=0; i<n; i++,s++,dst++)
			{
				dst->x = m[0]*s->x                     + m[12];
				dst->y =         + m[5]*s->y           + m[13];
				dst->z =                   +      s->z        ;
				dst->w =                                  1.0F;
			}
		}
		break;

		case MATRIX_3D:
		{
			const GLfloat *m = ModelViewMatrix;
			const VECTOR *src = vObj;
			VECTOR *dst = vEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->x, oy = src->y, oz = src->z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz + m[12];
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz + m[13];
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz + m[14];
				dst->w =                                 1.0F;
			}
		}
		break;

		default: // should never get here
			//gl_problem( NULL, "invalid matrix type in transform_points3()" );
			return;
	} // switch
}

//

#ifdef USE_VERT_STRUCT
void MatrixMgr::transform_points (int n, VECTOR *& vEye, const VBVertex *vObj)
{
	if (ModelViewMatrix.type == MATRIX_MODIFIED)
	{
		ModelViewMatrix.analyze_view();
		ModelViewInv.invert(ModelViewMatrix);
	}

	switch (ModelViewMatrix.type)
	{
		case MATRIX_MODIFIED:
		case MATRIX_GENERAL:
		{
			const GLfloat *m = ModelViewMatrix;
			const VBVertex *src = vObj;
			VECTOR *dst = vEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->obj.x, oy = src->obj.y, oz = src->obj.z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz + m[12];
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz + m[13];
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz + m[14];
				dst->w = 1; // m[3]*ox + m[7]*oy + m[11]*oz + m[15];
			}
		}
		break;

		case MATRIX_IDENTITY:
		{
		// Ack, useless copy.
			const VBVertex * src = vObj;
			VECTOR * dst = vEye;
			for (int i = 0; i < n; i++, src++, dst++)
			{
				*dst = src->obj;
			}
		}
		break;

		case MATRIX_2D:
		{
			const GLfloat *m = ModelViewMatrix;
			const VBVertex *src = vObj;
			VECTOR *dst = vEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->obj.x, oy = src->obj.y, oz = src->obj.z;
				dst->x = m[0]*ox + m[4]*oy           + m[12];
				dst->y = m[1]*ox + m[5]*oy           + m[13];
				dst->z =                   +      oz        ;
				dst->w =                                1.0F;
			}
		}
		break;

		case MATRIX_2D_NO_ROT:
		{
			const GLfloat *m = ModelViewMatrix;
			const VBVertex *s = vObj;
			VECTOR *dst = vEye;
			for (int i=0; i<n; i++,s++,dst++)
			{
				dst->x = m[0]*s->obj.x                     + m[12];
				dst->y =         + m[5]*s->obj.y           + m[13];
				dst->z =                   +      s->obj.z        ;
				dst->w =                                  1.0F;
			}
		}
		break;

		case MATRIX_3D:
		{
			const GLfloat *m = ModelViewMatrix;
			const VBVertex *src = vObj;
			VECTOR *dst = vEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->obj.x, oy = src->obj.y, oz = src->obj.z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz + m[12];
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz + m[13];
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz + m[14];
				dst->w =                                 1.0F;
			}
		}
		break;

		default: // should never get here
			//gl_problem( NULL, "invalid matrix type in transform_points3()" );
			return;
	} // switch
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::transform_normals (int n, VECTOR *nEye, const VECTOR *nObj)
{
	switch (ModelViewMatrix.type)
	{
		case MATRIX_MODIFIED:
		case MATRIX_GENERAL:
		{
			const GLfloat *m = ModelViewMatrix;
			const VECTOR *src = nObj;
			VECTOR *dst = nEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->x, oy = src->y, oz = src->z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz;
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz;
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz;
				dst->w = 1;
			}
		}
		break;

		case MATRIX_IDENTITY:
		{
		// Ack, useless copy.
			const VECTOR *src = nObj;
			VECTOR *dst = nEye;
			memcpy(dst, src, sizeof(VECTOR) * n);
		}
		break;

		case MATRIX_2D:
		{
			const GLfloat *m = ModelViewMatrix;
			const VECTOR *src = nObj;
			VECTOR *dst = nEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->x, oy = src->y, oz = src->z;
				dst->x = m[0]*ox + m[4]*oy;
				dst->y = m[1]*ox + m[5]*oy;
				dst->z = oz;
				dst->w = 1.0F;
			}
		}
		break;

		case MATRIX_2D_NO_ROT:
		{
			const GLfloat *m = ModelViewMatrix;
			const VECTOR *s = nObj;
			VECTOR *dst = nEye;
			for (int i=0; i<n; i++,s++,dst++)
			{
				dst->x = m[0]*s->x;
				dst->y = m[5]*s->y;
				dst->z = s->z;
				dst->w = 1.0F;
			}
		}
		break;

		case MATRIX_3D:
		{
			const GLfloat *m = ModelViewMatrix;
			const VECTOR *src = nObj;
			VECTOR *dst = nEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->x, oy = src->y, oz = src->z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz;
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz;
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz;
				dst->w = 1.0F;
			}
		}
		break;

		default: // should never get here
			//gl_problem( NULL, "invalid matrix type in transform_points3()" );
			return;
	} // switch
}

//
#ifdef USE_VERT_STRUCT
void MatrixMgr::transform_normals (int n, VECTOR *nEye, const VBVertex *nObj)
{
	switch (ModelViewMatrix.type)
	{
		case MATRIX_MODIFIED:
		case MATRIX_GENERAL:
		{
			const GLfloat *m = ModelViewMatrix;
			const VBVertex *src = nObj;
			VECTOR *dst = nEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->normal.x, oy = src->normal.y, oz = src->normal.z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz;
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz;
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz;
				dst->w = 1;
			}
		}
		break;

		case MATRIX_IDENTITY:
		{
		// Ack, useless copy.
			const VBVertex *src = nObj;
			VECTOR *dst = nEye;
			for (int i = 0; i < n; i++, src++, dst++)
			{
				*dst = src->normal;
			}
		}
		break;

		case MATRIX_2D:
		{
			const GLfloat *m = ModelViewMatrix;
			const VBVertex *src = nObj;
			VECTOR *dst = nEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->normal.x, oy = src->normal.y, oz = src->normal.z;
				dst->x = m[0]*ox + m[4]*oy;
				dst->y = m[1]*ox + m[5]*oy;
				dst->z = oz;
				dst->w = 1.0F;
			}
		}
		break;

		case MATRIX_2D_NO_ROT:
		{
			const GLfloat *m = ModelViewMatrix;
			const VBVertex *s = nObj;
			VECTOR *dst = nEye;
			for (int i=0; i<n; i++,s++,dst++)
			{
				dst->x = m[0]*s->normal.x;
				dst->y = m[5]*s->normal.y;
				dst->z = s->normal.z;
				dst->w = 1.0F;
			}
		}
		break;

		case MATRIX_3D:
		{
			const GLfloat *m = ModelViewMatrix;
			const VBVertex *src = nObj;
			VECTOR *dst = nEye;
			for (int i=0; i<n; i++,src++,dst++)
			{
				GLfloat ox = src->normal.x, oy = src->normal.y, oz = src->normal.z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz;
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz;
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz;
				dst->w = 1.0F;
			}
		}
		break;

		default: // should never get here
			//gl_problem( NULL, "invalid matrix type in transform_points3()" );
			return;
	} // switch
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MatrixMgr::project_clip (int vcount, uchar *clip_list, VECTOR *vClip, const VECTOR *vEye)
{
	if (ProjectionMatrix.type == MATRIX_MODIFIED)
	{
		ProjectionMatrix.analyze_project();
	}

	switch (ProjectionMatrix.type)
	{
		case MATRIX_IDENTITY:
		{
			// FUTURE: avoid un-necessary copying?
			memcpy(vClip,vEye,vcount*sizeof(VECTOR));
		}
		break;

		case MATRIX_ORTHO:
		{
			const GLfloat * m = ProjectionMatrix;
			const VECTOR * src = vEye;
			VECTOR * dst = vClip;
			for (int i = 0; i < vcount; i++, src++, dst++)
			{
				GLfloat ox = src->x, oy = src->y, oz = src->z, ow = 1.0;
				dst->x = m[0]*ox                       + m[12] * ow;
				dst->y =           m[5]*oy             + m[13] * ow;
				dst->z =                    m[10] * oz + m[14] * ow;
				dst->w =                                         ow;
			}
		}
		break;

		default:
		case MATRIX_MODIFIED:
		case MATRIX_GENERAL:
		{
			const GLfloat *m = ProjectionMatrix;
			const VECTOR *src = vEye;
			VECTOR *dst = vClip;
			for (int i=0; i<vcount; i++,src++,dst++)
			{
				GLfloat ox = src->x, oy = src->y, oz = src->z;
				dst->x = m[0]*ox + m[4]*oy + m[ 8]*oz + m[12];
				dst->y = m[1]*ox + m[5]*oy + m[ 9]*oz + m[13];
				dst->z = m[2]*ox + m[6]*oy + m[10]*oz + m[14];
				dst->w = m[3]*ox + m[7]*oy + m[11]*oz + m[15];
			}
		}
		break;
	}

// SET CLIPPING FLAGS

	// ASSUME: any_clip,all_clip are initialized 

	if (clip_list)
	if (enable_clipping)
	{
		VECTOR *dst = vClip;
		for (int i=0; i<vcount; i++,dst++)
		{
			uint clip = 0; // compiler sign problem?

			float pos = dst->w;
			float neg = -pos;
			if (dst->x > pos)		clip = CLIP_RIGHT;
			else if (dst->x < neg)	clip = CLIP_LEFT;
			if (dst->y > pos)		clip |= CLIP_TOP;
			else if (dst->y < neg)	clip |= CLIP_BOTTOM;
			if (dst->z > pos)		clip += CLIP_FAR;	// AVOID COMPILE BUG?
			else if (dst->z < neg)	clip |= CLIP_NEAR;

			clip_list[i] = clip;

			any_clip |= clip;
			all_clip &= clip;
		}
	}
	else
	{
		memset(clip_list,0,vcount*sizeof(clip_list[0]));
	}
}

//---------------------------------------------------------------------------
