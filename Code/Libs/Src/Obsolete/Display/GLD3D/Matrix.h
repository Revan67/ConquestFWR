//---------------------------------------------------------------------------
/*
	MATRIX.H

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Paul Isaac
*/
//---------------------------------------------------------------------------

#ifndef _MATRIX_H
#define _MATRIX_H

//---------------------------------------------------------------------------

#include <memory.h>

#include "display.h"		// GLenum, etc.

#ifndef PI
  #define PI 3.1415926536
#endif

#define DEG2RAD (PI/180.0)

#pragma warning(disable:4244) // double to float
#pragma warning(disable:4305) // double to float

typedef unsigned int uint;
typedef unsigned char uchar;

//---------------------------------------------------------------------------
// CLIP BIT FIELD
//---------------------------------------------------------------------------

#define CLIP_LEFT	1
#define CLIP_RIGHT	2
#define CLIP_TOP	4
#define CLIP_BOTTOM	8
#define CLIP_NEAR	64
#define CLIP_FAR	128

//---------------------------------------------------------------------------
// VECTOR
//---------------------------------------------------------------------------

#define X 0
#define Y 1
#define Z 2
#define W 3

struct VECTOR
{
	float x,y,z,w;

	operator float *(void)
	{
		return &x;
	}

	void set (float _x, float _y, float _z, float _w=1.0)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
};

//---------------------------------------------------------------------------

enum MATRIX_TYPE
{
	MATRIX_GENERAL,			// general 4x4 matrix
	MATRIX_IDENTITY,
	MATRIX_ORTHO,			// orthographic projection
	MATRIX_PERSPECTIVE,
	MATRIX_2D,
	MATRIX_2D_NO_ROT,		// 2D scale & translate
	MATRIX_3D,

	MATRIX_MODIFIED=255		// aka. MATRIX_GENERAL
};

enum
{
    M00 = 0, M01 = 4, M02 = 8, M03 = 12,
    M10 = 1, M11 = 5, M12 = 9, M13 = 13,
    M20 = 2, M21 = 6, M22 = 10,M23 = 14,
    M30 = 3, M31 = 7, M32 = 11,M33 = 15
};

#define M(matrix,row,col) ((matrix)[row+(col)*4])

extern float Identity[4*4];

//---------------------------------------------------------------------------
// Matrix_4x4
//---------------------------------------------------------------------------

struct Matrix_4x4
{
	float m[4*4];

	MATRIX_TYPE type;

	operator float *(void)
	{
		return m;
	}

	inline void set_identity (void)
	{
		memcpy(m, Identity, sizeof(Identity));

		type = MATRIX_IDENTITY;
	}

	inline void set (const float *src)
	{
		memcpy(m, src, sizeof(m));
		type = MATRIX_MODIFIED;
	}

	inline void get (float *dst) const
	{
		memcpy(dst, m, sizeof(m));
	}

	void scale (float x, float y, float z);

	void translate (float x, float y, float z);

	void analyze_view (void);
	// Examine the current modelview matrix to determine its type.
	// Later we use the matrix type to optimize vertex transformations.

	void analyze_project (void);
	// Examine the current projection matrix to determine its type.
	// Later we use the matrix type to optimize vertex transformations.

	void invert (const float *src);

	void mul (VECTOR *dst, const VECTOR *src);

	void mul4 (VECTOR *dst, const VECTOR *src);
};

//---------------------------------------------------------------------------
// MatrixMgr
//---------------------------------------------------------------------------

struct MatrixMgr
{
// STATE

	GLenum Matrix_mode;

	Matrix_4x4 ModelViewMatrix;
	Matrix_4x4 ModelViewInv;;

	Matrix_4x4 ProjectionMatrix;

	Matrix_4x4 TextureMatrix;

// Clip Status

	bool enable_clipping;

	int any_clip;	// project_clip() determines if any vertices are clipped
	int all_clip;	// project_clip() determines if all vertices are clipped

// Projection

	float h_scale,v_scale;
	float h_offset,v_offset;

// STACK

	#define MATRIX_STACK_DEPTH		16		// required to have 16 elements
	// FUTURE: separate stack for each type?

	float matrix_stack[MATRIX_STACK_DEPTH][4*4];
	uint stack_count;

// CONSTRUCTION

	MatrixMgr (void);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void set_identity (GLenum mode);

	Matrix_4x4 *get_matrix (GLenum mode);

	void load_matrix (const float *m);

	void store_matrix (float *m);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	GLenum push_matrix (void);

	GLenum pop_matrix (void);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void matrix_mul (float product[16], const float a[16], const float b[16]);

	void matrix_mul (GLenum mode, const float *m)
	{
		Matrix_4x4 *dst = get_matrix(mode);
		matrix_mul(dst->m, dst->m,m);
		dst->type = MATRIX_MODIFIED;
	}

	void rotate_translate (float *dst, float *src);

	void matrix_rotate (float _angle, float x, float y, float z);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void matrix_scale (float x, float y, float z)
	{
		get_matrix(Matrix_mode)->scale(x,y,z);
	}

	void matrix_translate (float x, float y, float z)
	{
		get_matrix(Matrix_mode)->translate(x,y,z);
	}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void frustum_matrix (float left, float right, float bottom, float top, float nearval, float farval);

	void ortho_matrix (float left, float right, float bottom, float top, float nearval, float farval);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void transform_points (int n, VECTOR *& vEye, const VECTOR *vObj);
	void transform_normals(int n, VECTOR *nEye, const VECTOR *nObj);

	void transform_points (int n, VECTOR *& vEye, const struct VBVertex * vObj);
	void transform_normals(int n, VECTOR *nEye, const struct VBVertex *nObj);

	void project_clip (int vcount, uchar *clip_list, VECTOR *vClip, const VECTOR *vEye);
};

//---------------------------------------------------------------------------

#endif // _MATRIX_H
