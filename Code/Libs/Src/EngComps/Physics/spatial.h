//
// Spatial algebra. See Featherstone "Robot Dynamics Algorithms" for details.
//

#ifndef SPATIAL_H
#define SPATIAL_H

//

#include "3dmath.h"

//

class SpatialVector
{
public:

	union
	{
		float d[6];
		struct
		{
			Vector v1;
			Vector v2;
		};
	};

	bool prime;

	SpatialVector(void)
	{
		prime = false;
	}

	SpatialVector(const Vector & top, const Vector & bottom)
	{
		v1 = top;
		v2 = bottom;
		prime = false;
	}

	void zero(void)
	{
		v1.zero();
		v2.zero();
	}

	SpatialVector transpose(void) const
	{
		SpatialVector result;
		result.v1 = v2;
		result.v2 = v1;
		result.prime = true;
		return result;
	}

	SpatialVector operator += (const SpatialVector & v)
	{
		v1 += v.v1;
		v2 += v.v2;
		return *this;
	}

	SpatialVector operator -= (const SpatialVector & v)
	{
		v1 -= v.v1;
		v2 -= v.v2;
		return *this;
	}

	friend SpatialVector operator - (const SpatialVector & v)
	{
		SpatialVector result;
		result.v1 = -v.v1;
		result.v2 = -v.v2;
		return result;
	}

	friend SpatialVector operator * (const SpatialVector & v, float s)
	{
		SpatialVector result;
		result.v1 = v.v1 * s;
		result.v2 = v.v2 * s;
		result.prime = v.prime;
		return result;
	}

	friend SpatialVector operator * (float s, const SpatialVector & v)
	{
		return v * s;
	}

	friend SpatialVector operator / (const SpatialVector & v, float s)
	{							  
		SpatialVector result;
		result.v1 = v.v1 / s;
		result.v2 = v.v2 / s;
		result.prime = v.prime;
		return result;
	}


	friend SpatialVector operator + (const SpatialVector & v1, const SpatialVector & v2)
	{
		SpatialVector result(v1.v1 + v2.v1, v1.v2 + v2.v2);
		return result;
	}

	friend SpatialVector operator - (const SpatialVector & v1, const SpatialVector & v2)
	{
		SpatialVector result(v1.v1 - v2.v1, v1.v2 - v2.v2);
		return result;
	}

	friend float dot(const SpatialVector & v1, const SpatialVector & v2)
	{
		//ASSERT((v1.prime == true) && (v2.prime != true));

		return dot_product(v1.v1, v2.v1) + dot_product(v1.v2, v2.v2);
	}

	friend class SpatialMatrix operator * (const SpatialVector & v1, const SpatialVector & v2);
};

//

class SpatialMatrix
{
public:

	float d[6][6];

	SpatialMatrix(void)
	{
	}

	SpatialMatrix(const Matrix & m00, const Matrix & m01, const Matrix & m10, const Matrix & m11)
	{
		init(m00, m01, m10, m11);
	}

	void init(const Matrix & m00, const Matrix & m01, const Matrix & m10, const Matrix & m11)
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				d[i][j]		= m00.d[i][j];
				d[i][j+3]	= m01.d[i][j];
				d[i+3][j]	= m10.d[i][j];
				d[i+3][j+3]	= m11.d[i][j];
			}
		}
	}

	SpatialMatrix set_identity(void)
	{
		memset(d, 0, sizeof(float) * 6 * 6);
		for (int i = 0; i < 6; i++)
		{
			d[i][i] = 1.0;
		}
		return *this;
	}

	SpatialMatrix get_inverse(void) const;

	SpatialMatrix operator - (void) const
	{
		SpatialMatrix result;
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				result.d[i][j] = -d[i][j];
			}
		}
		return result;
	}

	SpatialMatrix operator += (const SpatialMatrix & m)
	{
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				d[i][j] += m.d[i][j];
			}
		}

		return *this;
	}

	SpatialMatrix operator -= (const SpatialMatrix & m)
	{
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				d[i][j] -= m.d[i][j];
			}
		}

		return *this;
	}


	friend SpatialMatrix operator + (const SpatialMatrix & m1, const SpatialMatrix & m2)
	{
		SpatialMatrix result;

		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				result.d[i][j] = m1.d[i][j] + m2.d[i][j];
			}
		}

		return result;
	}


	friend SpatialMatrix operator - (const SpatialMatrix & m1, const SpatialMatrix & m2)
	{
		SpatialMatrix result;

		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				result.d[i][j] = m1.d[i][j] - m2.d[i][j];
			}
		}

		return result;
	}

	friend SpatialMatrix operator * (const SpatialMatrix & m, float s)
	{
		SpatialMatrix result;

		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				result.d[i][j] = m.d[i][j] * s;
			}
		}

		return result;
	}

	friend SpatialMatrix operator * (float s, const SpatialMatrix & m)
	{
		return m * s;
	}

	friend SpatialMatrix operator / (const SpatialMatrix & m, float s)
	{
		SpatialMatrix result;

		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				result.d[i][j] = m.d[i][j] / s;
			}
		}

		return result;
	}

	friend SpatialMatrix operator * (const SpatialMatrix & m1, const SpatialMatrix & m2)
	{
		SpatialMatrix result;

		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				result.d[i][j] = 0.0;
				for (int k = 0; k < 6; k++)
				{
					result.d[i][j] += m1.d[i][k] * m2.d[k][j];
				}
			}
		}

		return result;
	}

	friend SpatialVector operator * (const SpatialMatrix & m, const SpatialVector & vec)
	{
		//ASSERT(vec.prime != true);

		SpatialVector result;

		for (int i = 0; i < 6; i++)
		{
			result.d[i] = 0;

			for (int j = 0; j < 6; j++)
			{
				result.d[i] += m.d[i][j] * vec.d[j];
			}
		}

		return result;
	}

	friend SpatialVector operator * (const SpatialVector & vec, const SpatialMatrix & m)
	{
		//ASSERT(vec.prime == true);

		SpatialVector result;
		for (int i = 0; i < 6; i++)
		{
			result.d[i] = 0.0;
			for (int j = 0; j < 6; j++)
			{
				result.d[i] += vec.d[j] * m.d[j][i];
			}
		}

		result.prime = true;
		return result;
	}

	friend SpatialMatrix operator * (const SpatialVector & v1, const SpatialVector & v2)
	{
		SpatialMatrix result;

		//ASSERT(!v1.prime && v2.prime);

		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				result.d[i][j] = v1.d[i] * v2.d[j];
			}
		}

		return result;
	}
};

//
// Articulated body inertia.
//
struct SpatialABI
{
	Matrix M;
	Matrix H;
	Matrix I;

	SpatialABI(void) {}
	SpatialABI(const Matrix & _M, const Matrix & _H, const Matrix & _I)
	{
		M = _M;
		H = _H;
		I = _I;
	}

	SpatialABI operator += (const SpatialABI & s)
	{
		M = M + s.M;
		H = H + s.H;
		I = I + s.I;
		return *this;
	}

	friend SpatialVector operator * (const SpatialABI & s, const SpatialVector & v)
	{
		return SpatialVector(s.H.get_transpose() * v.v1 + s.M * v.v2, s.I * v.v1 + s.H * v.v2);
	}

	friend SpatialVector operator * (const SpatialVector & v, const SpatialABI & s)
	{
		SpatialVector result(v.v1 * s.H.get_transpose() + v.v2 * s.I, v.v1 * s.M + v.v2 * s.H);
		result.prime = true;
		return result;
	}
};

//
// Rigid body inertia.
//
struct SpatialRBI
{
	float	m;
	Vector	h;
	Matrix	I;

	SpatialRBI(void) {}
	SpatialRBI(float _m, const Vector _h, const Matrix & _I)
	{
		m = _m;
		h = _h;
		I = _I;
	}
};


//

struct SpatialTransform
{
	Matrix E;	// 3x3
	Vector r;

//			   | E	     0 |
// Transform = |           |
//			   | E ~rT   E |

	SpatialTransform(const Matrix & _E, const Vector & _r)
	{
		E = _E;
		r = _r;
	}

// SpatialTransform * SpatialTransform
	friend SpatialTransform operator * (const SpatialTransform & t0, const SpatialTransform & t1)
	{
		return SpatialTransform(t0.E * t1.E, t1.r + t1.E.get_transpose() * t0.r);
	}

// SpatialTransform * SpatialVector
	SpatialVector transform_vector(const SpatialVector & v)
	{
		Vector v1 = E * v.v1;
		Vector v2 = E * (v.v2 - cross_product(r, v.v1));
		return SpatialVector(v1, v2);
	}

// SpatialTransform * SpatialABI
	SpatialABI transform_ABI(const SpatialABI & abi)
	{
		Matrix ET = E.get_transpose();
		const Matrix & M = abi.M;
		const Matrix & H = abi.H;
		const Matrix & I = abi.I;
		Matrix rx = dual(r);
		Matrix rxM = rx * M;

		return SpatialABI(E*M*ET, E*(H - rxM)*ET, E*(I - rx * H.get_transpose() + (H - rxM) * rx)*ET);
	}


// SpatialTransform * SpatialRBI
	friend SpatialRBI operator * (const SpatialTransform & t, const SpatialRBI & rbi)
	{
		Matrix rx = dual(t.r);
		Matrix hx = dual(rbi.h);

		Vector h_mr = rbi.h - rbi.m * t.r;
		Matrix h_mrx = dual(h_mr);

		return SpatialRBI(rbi.m, t.E * h_mr, t.E*(rbi.I + rx * hx + h_mrx * rx)*t.E.get_transpose());
	}
};

//

#endif