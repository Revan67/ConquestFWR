// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Matrix4x4_h
#define Matrix4x4_h
// --------------------------------------------------------------------------
#include <iostream>
#include <Math.h>
#include <windows.h>
#include "Vector.h"

#include "TrigUtil.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Matrix4x4
{
	public:
    	Matrix4x4(const Vector& i = Vector(1, 0, 0), const Vector& j = Vector(0, 1, 0), const Vector& k = Vector(0, 0, 1), const Vector& t = Vector(0, 0, 0))
		{
			Set(i, j, k, t);
            InitLastRow();
		}

    	Matrix4x4(const float mtx[4][4]);

		void Set(const Vector& i = Vector(1, 0, 0), const Vector& j = Vector(0, 1, 0), const Vector& k = Vector(0, 0, 1), const Vector& t = Vector(0, 0, 0))
        {
        	SetI(i);
            SetJ(j);
            SetK(k);
            SetT(t);
        }
        void SetI(const Vector& i) { element[0][0] = i.x; element[1][0] = i.y; element[2][0] = i.z; };
        void SetJ(const Vector& j) { element[0][1] = j.x; element[1][1] = j.y; element[2][1] = j.z; };
        void SetK(const Vector& k) { element[0][2] = k.x; element[1][2] = k.y; element[2][2] = k.z; };
        void SetT(const Vector& t) { element[0][3] = t.x; element[1][3] = t.y; element[2][3] = t.z; };
		void SetIdentity() { *this = Matrix4x4(); };

        Vector GetI() const { return Vector(element[0][0], element[1][0], element[2][0]); };
        Vector GetJ() const { return Vector(element[0][1], element[1][1], element[2][1]); };
        Vector GetK() const { return Vector(element[0][2], element[1][2], element[2][2]); };
        Vector GetT() const { return Vector(element[0][3], element[1][3], element[2][3]); };
        float Get(unsigned int row, unsigned int col) const { return element[row][col]; };

        Matrix4x4 GetTranspose() const;
        Matrix4x4 GetInverse() const;

		void MakeRotationMatrix(const Vector& vec, float angle);
        Matrix4x4 operator*(const Matrix4x4& multiplier) const;

//		void Write(std::ostream& oStream) const;
//		void Read(std::istream& iStream);

    private:
    	void InitLastRow() { element[3][0] = element[3][1] = element[3][2] = 0; element[3][3] = 1; };

    	float element[4][4];
};
// --------------------------------------------------------------------------
inline Matrix4x4::Matrix4x4(const float mtx[4][4])
{
    for(int i = 0; i < 4; ++i)
    {	for(int j = 0; j < 4; ++j)
        {	element[i][j] = mtx[i][j];
        }
    }
}
// --------------------------------------------------------------------------
inline Matrix4x4 Matrix4x4::GetTranspose() const
{
	Matrix4x4	inv;

    for(int i = 0; i < 4; ++i)
    {	for(int j = 0; j < 4; ++j)
    	{	inv.element[i][j] = element[j][i];
        }
    }

    return inv;
}
// --------------------------------------------------------------------------
inline Matrix4x4 Matrix4x4::GetInverse() const
{
	Matrix4x4 inverse = GetTranspose();
    float	tx = - element[0][3];
    float	ty = - element[1][3];
    float	tz = - element[2][3];

    inverse.element[0][3] = element[0][0]*tx + element[1][0]*ty + element[2][0]*tz;
    inverse.element[1][3] = element[0][1]*tx + element[1][1]*ty + element[2][1]*tz;
    inverse.element[2][3] = element[0][2]*tx + element[1][2]*ty + element[2][2]*tz;

    inverse.InitLastRow();

    return inverse;
}
// --------------------------------------------------------------------------
inline Matrix4x4 Matrix4x4::operator*(const Matrix4x4& multiplier) const
{
    Matrix4x4	result;

    for(int i = 0; i < 4; ++i)
    {	for(int j = 0; j < 4; ++j)
        {	result.element[i][j] =	element[i][0] * multiplier.element[0][j] +
                          	    	element[i][1] * multiplier.element[1][j] +
                            	    element[i][2] * multiplier.element[2][j] +
                            	    element[i][3] * multiplier.element[3][j];
        }
    }

    return result;
}
// --------------------------------------------------------------------------
inline void Matrix4x4::MakeRotationMatrix(const Vector& vec, float angle)
{
	float x = vec.x;
    float y = vec.y;
    float z = vec.z;

    float s = sin(DegreeToRadian(angle));
    float c = cos(DegreeToRadian(angle));
    float t = 1.0 - c;

    // | tx^2 + c   txy + sz  txz - sy |
    // | txy - sz   ty^2 + c  tyz + sx |
    // | txz + sy   tyz - sx  tz^2 + c |
    element[0][0] = t * x * x + c;		element[0][1] = t * x * y + s * z;	element[0][2] = t * x * z - s * y;
    element[1][0] = t * x * y - s * z;	element[1][1] = t * y * y + c;		element[1][2] = t * y * z + s * x;
    element[2][0] = t * x * z + s * y;	element[2][1] = t * y * z - s * x;	element[2][2] = t * z * z + c;

    SetT(Vector(0, 0, 0));
    InitLastRow();
}
// --------------------------------------------------------------------------
inline void GetGLMatrix(const Matrix4x4& matrix, GLfloat glMatrix[4][4])
{
    for(int i = 0; i < 4; ++i)
    {	for(int j = 0; j < 4; ++j)
    	{	glMatrix[i][j] = matrix.Get(j, i);
        }
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
