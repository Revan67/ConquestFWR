// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DACompoundObject_h
#define DACompoundObject_h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "StringType.h"

#ifdef NO_DLL_EXPORTS

#define DXDEC
#define DXDEF

#define DXDEC_ROS
#define DXDEF_ROS

#else

#ifdef WIN32
  #define IS_WIN32 1
#endif

#ifdef _WIN32
  #define IS_WIN32 1
#endif

//
// If compiling Char library, use __declspec(dllexport) for both 
// declarations and definitions
//
// If compiling Char application, use __declspec(dllimport) for
// declarations -- definitions don't matter
//

#ifdef IS_WIN32

	#undef DXDEC
	#undef DXDEF

	#undef DXDEC_ROS
	#undef DXDEF_ROS

	#ifdef BUILD_ROS_DLL
		#define DXDEC_ROS __declspec(dllexport)
		#define DXDEF_ROS __declspec(dllexport)
	#else
		#define DXDEC_ROS __declspec(dllimport)
	#endif

	#define DXDEC
	#define DXDEF

#else

	#error Must define WIN32 or _WIN32 to use Char.h

#endif

#endif


#ifdef __cplusplus 
extern "C" { 
#endif

class Vector;
struct Mesh;
class Transform;

namespace ROS
{
class Matrix;
class DACompoundObject;
class DAMotionObject;
class DABaseCamera;
class DAHardPoints;
class IntersectInfo;
class StringList;
}

// Compounds
DXDEC_ROS const ROS::DACompoundObject* __cdecl CompoundObjectCreate(const ROS::StringList& descriptionStrings);

DXDEC_ROS void __cdecl CompoundObjectDestroy(const ROS::DACompoundObject* dACompoundObject);

DXDEC void CompoundObjectGetMotionNames(const ROS::DACompoundObject* dACompoundObject, ROS::StringList& motionNames);

DXDEC void __cdecl CompoundObjectLoadTextures(const char * filename);

DXDEC void __cdecl CompoundObjectRenderObject(const ROS::DACompoundObject* dACompoundObject, const ROS::DABaseCamera* camera);

DXDEC void __cdecl CompoundObjectSetPosition(const ROS::DACompoundObject* dACompoundObject, const Vector& position);

DXDEC void __cdecl CompoundObjectGetPosition(const ROS::DACompoundObject* dACompoundObject, Vector& position);

DXDEC void __cdecl CompoundObjectSetOrientation(const ROS::DACompoundObject* dACompoundObject, const ROS::Matrix& orientation);

DXDEC void __cdecl CompoundObjectGetOrientation(const ROS::DACompoundObject* dACompoundObject, ROS::Matrix& orientation);

DXDEC void __cdecl CompoundObjectSetTransform(const ROS::DACompoundObject* dACompoundObject, const Transform& transform);

DXDEC bool __cdecl CompoundObjectIntersect(const ROS::DACompoundObject* dACompoundObject, const ROS::IntersectInfo& intersectInfo, float* distance);

// Cameras
DXDEC unsigned int CamerasGetCount(const ROS::DACompoundObject* dACompoundObject);

DXDEC ROS::ROSString CamerasGetCameraName(const ROS::DACompoundObject* dACompoundObject, unsigned int idx);

DXDEC void CamerasGetCameraPosition(const ROS::DACompoundObject* dACompoundObject, unsigned int idx, Vector& position);

DXDEC void CamerasGetCameraOrientation(const ROS::DACompoundObject* dACompoundObject, unsigned int idx, ROS::Matrix& orientation);

DXDEC float CamerasGetCameraHorizontalFOV(const ROS::DACompoundObject* dACompoundObject, unsigned int idx);

DXDEC float CamerasGetCameraVerticalFOV(const ROS::DACompoundObject* dACompoundObject, unsigned int idx);

// Motion
DXDEC const ROS::DAMotionObject* __cdecl CompoundObjectCreateMotionObject(const ROS::DACompoundObject* dACompoundObject, const ROS::ROSString& motionName);

DXDEC void __cdecl MotionObjectDestroy(const ROS::DAMotionObject* dAMotionObject);

DXDEC void __cdecl MotionObjectStart(const ROS::DAMotionObject* dAMotionObject, bool loop, float startTime, float transition);

DXDEC void __cdecl MotionObjectStop(const ROS::DAMotionObject* dAMotionObject);

DXDEC void __cdecl MotionObjectPause(const ROS::DAMotionObject* dAMotionObject);

DXDEC void __cdecl MotionObjectResume(const ROS::DAMotionObject* dAMotionObject);

DXDEC float __cdecl MotionObjectGetDuration(const ROS::DAMotionObject* dAMotionObject);

#ifdef __cplusplus 
}
#endif

#endif	//	DACompoundObject_h