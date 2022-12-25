#ifndef EULERANGLES_H
#define EULERANGLES_H

//

#include "Matrix.h"

//

void EulerAnglesFromMatrix(float & x, float & y, float & z, const Matrix & R, bool negate = false);
void MatrixFromEulerAngles(Matrix & R, float x, float y, float z);

void XRotationMatrix(Matrix & R, float x);
void YRotationMatrix(Matrix & R, float y);
void ZRotationMatrix(Matrix & R, float z);

//

#endif