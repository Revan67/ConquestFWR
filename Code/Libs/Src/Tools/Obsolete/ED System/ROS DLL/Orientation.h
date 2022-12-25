// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Orientation_h
#define Orientation_h

#include <iostream>
#include "MatrixUtil.h"
// --------------------------------------------------------------------------
//	Orientation
// --------------------------------------------------------------------------
namespace ROS
{
	typedef Matrix Orientation;
// --------------------------------------------------------------------------
Orientation Interpolate(const Orientation& previousOrientation, const Orientation& nextOrientation, float t);
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

