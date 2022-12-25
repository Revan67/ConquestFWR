// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef MatrixUtil_h
#define MatrixUtil_h
// --------------------------------------------------------------------------
#include <Math.h>
#include <iostream>
#include <windows.h>
#include "Vector.h"
#include "TrigUtil.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class CPP_DECL Matrix
{
	public:
    	explicit Matrix(const Vector& i = Vector(1, 0, 0), const Vector& j = Vector(0, 1, 0), const Vector& k = Vector(0, 0, 1))
		{
			Set(i, j, k);
		}

		void Set(const Vector& i = Vector(1, 0, 0), const Vector& j = Vector(0, 1, 0), const Vector& k = Vector(0, 0, 1))
        {
        	SetI(i);
            SetJ(j);
            SetK(k);
        }
        void SetI(const Vector& i) { element[0][0] = i.x; element[1][0] = i.y; element[2][0] = i.z; };
        void SetJ(const Vector& j) { element[0][1] = j.x; element[1][1] = j.y; element[2][1] = j.z; };
        void SetK(const Vector& k) { element[0][2] = k.x; element[1][2] = k.y; element[2][2] = k.z; };
		void SetIdentity() { *this = Matrix(); };

        Vector GetI() const { return Vector(element[0][0], element[1][0], element[2][0]); };
        Vector GetJ() const { return Vector(element[0][1], element[1][1], element[2][1]); };
        Vector GetK() const { return Vector(element[0][2], element[1][2], element[2][2]); };
        float Get(unsigned int row, unsigned int col) const { return element[row][col]; };

		void MakeRotationMatrix(const Vector& vec, float angle);
        Matrix operator*(const Matrix& multiplier) const;

		void Write(std::ostream& ostreamR) const;
		void Read(std::istream& istreamR);

    private:
		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

    	float element[3][3];
};
// --------------------------------------------------------------------------
typedef float GLfloat;

CPP_DECL void GetGLMatrix(const Matrix& matrix, GLfloat glMatrix[4][4]);
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::Matrix& matrix)
{
	matrix.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::Matrix& matrix)
{
	matrix.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif
