//---------------------------------------------------------------------------
#include "PCH.h"
#include "MatrixUtil.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kElementAt_0_0
};
// --------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
CPP_DEFN void Matrix::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Matrix::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	unsigned int	elementIdx = 0;

    for(unsigned int r = 0; r < 3; ++r)
    {
		for(unsigned int c = 0; c < 3; ++c)
		{
			oWiz.Put(static_cast<FieldID>(kElementAt_0_0 + elementIdx), element[r][c]);

			++elementIdx;
        }
    }
}
//---------------------------------------------------------------------------
CPP_DEFN void Matrix::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Matrix::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	unsigned int	elementIdx = 0;

    for(unsigned int r = 0; r < 3; ++r)
    {
		for(unsigned int c = 0; c < 3; ++c)
		{
			iWiz.Get(static_cast<FieldID>(kElementAt_0_0 + elementIdx), element[r][c]);

			++elementIdx;
        }
    }
}
// --------------------------------------------------------------------------
CPP_DEFN Matrix Matrix::operator*(const Matrix& multiplier) const
{
    Matrix	result;

    for(int i = 0; i < 3; ++i)
    {
		for(int j = 0; j < 3; ++j)
        {
			result.element[i][j] =	element[i][0] * multiplier.element[0][j] +
                          	    	element[i][1] * multiplier.element[1][j] +
                            	    element[i][2] * multiplier.element[2][j];
        }
    }

    return result;
}
// --------------------------------------------------------------------------
CPP_DEFN void Matrix::MakeRotationMatrix(const Vector& vec, float angle)
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
}
// --------------------------------------------------------------------------
CPP_DEFN void GetGLMatrix(const Matrix& matrix, GLfloat glMatrix[4][4])
{
    for(int i = 0; i < 3; ++i)
    {
		for(int j = 0; j < 3; ++j)
    	{
			glMatrix[i][j] = matrix.Get(j, i);
        }
    }

    glMatrix[3][0] =
    glMatrix[3][1] =
    glMatrix[3][2] = 0.0;

    glMatrix[0][3] =
    glMatrix[1][3] =
    glMatrix[2][3] = 0.0;

    glMatrix[3][3] = 1.0;
}
//---------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
