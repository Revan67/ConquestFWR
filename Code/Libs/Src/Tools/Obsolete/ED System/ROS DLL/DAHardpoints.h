// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DAHardPoints_h
#define DAHardPoints_h
// --------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "StringType.h"
// --------------------------------------------------------------------------
class Vector;
struct Mesh;
class Transform;

namespace ROS
{
class DAHardPoints;
class Matrix;
class DACompoundObject;
class DADeformableObject;
class HardPointHost;
}
// --------------------------------------------------------------------------
// HardPoints
const ROS::DAHardPoints* HardPointsCreate(const ROS::DACompoundObject* dACompoundObject);

void HardPointsDestroy(const ROS::DAHardPoints* dAHardPoints);

void HardPointsDraw(const ROS::DAHardPoints* dAHardPoints, const ROS::DABaseCamera* camera);

unsigned int HardPointsGetCount(const ROS::DAHardPoints* dAHardPoints);

const char* HardPointsGetHardPointName(const ROS::DAHardPoints* dAHardPoints, unsigned int idx);

void HardPointsGetHardPointPosition(const ROS::DAHardPoints* dAHardPoints, unsigned int idx, Vector& position);

void HardPointsGetHardPointOrientation(const ROS::DAHardPoints* dAHardPoints, unsigned int idx, ROS::Matrix& orientation);

const ROS::HardPointHost* HardPointsGetHardPointHost(const ROS::DAHardPoints* dAHardPoints, unsigned int idx);

void HardPointsAttachHardPointToParent(const ROS::DAHardPoints* dAHardPoints, unsigned int idx, const ROS::HardPointHost* parentHardPointHost, const ROS::ROSString& parentHardPointName);

void HardPointsDetachHardPointFromParent(const ROS::DAHardPoints* dAHardPoints, unsigned int idx, const ROS::HardPointHost* parentHardPointHost, const ROS::ROSString& parentHardPointName);
// --------------------------------------------------------------------------
#endif	//	DAHardPoints_h