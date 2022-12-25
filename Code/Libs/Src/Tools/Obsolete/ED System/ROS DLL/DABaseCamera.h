#ifndef DABaseCamera_h
#define DABaseCamera_h

#include "xform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

#if 0
	#ifdef BUILD_CHAR_DLL
		#define DXDEC __declspec(dllexport)
		#define DXDEF __declspec(dllexport)
	#else
		#define DXDEC __declspec(dllimport)
	#endif
#else
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
#endif

#else

	#error Must define WIN32 or _WIN32 to use Char.h

#endif

#endif

class Vector;
struct BaseCamera;

namespace ROS
{
class Matrix;
class DABaseCamera;
}

#ifdef __cplusplus 
extern "C" { 
#endif

// Cameras
DXDEC_ROS const ROS::DABaseCamera* CameraCreate(unsigned int displayWidth, unsigned int displayHeight);

DXDEC_ROS void __cdecl CameraDestroy(const ROS::DABaseCamera* camera);

DXDEC void __cdecl CameraScale(const ROS::DABaseCamera* camera, float scale);

DXDEC_ROS void __cdecl CameraGetPosition(const ROS::DABaseCamera* camera, Vector& position);

DXDEC_ROS void __cdecl CameraSetPosition(const ROS::DABaseCamera* camera, const Vector& position);

DXDEC_ROS void __cdecl CameraGetOrientation(const ROS::DABaseCamera* camera, ROS::Matrix& orientation);

DXDEC_ROS void __cdecl CameraSetOrientation(const ROS::DABaseCamera* camera, const ROS::Matrix& orientation);

DXDEC_ROS Transform __cdecl CameraGetTransform(const ROS::DABaseCamera* camera);

DXDEC_ROS void __cdecl CameraSetTransform(const ROS::DABaseCamera* camera, const Transform& transform);

DXDEC_ROS float __cdecl CameraGetHorizontalFOV(const ROS::DABaseCamera* camera);

DXDEC_ROS void __cdecl CameraSetHorizontalFOV(const ROS::DABaseCamera* camera, float hFOV);

DXDEC_ROS float __cdecl CameraGetVerticalFOV(const ROS::DABaseCamera* camera);

DXDEC_ROS void __cdecl CameraSetVerticalFOV(const ROS::DABaseCamera* camera, float vFOV);

DXDEC_ROS float __cdecl CameraGetAspectRatio(const ROS::DABaseCamera* camera);

DXDEC_ROS float __cdecl CameraGetZNear(const ROS::DABaseCamera* camera);

DXDEC void __cdecl CameraSetZNear(const ROS::DABaseCamera* camera, float zNear);

DXDEC_ROS float __cdecl CameraGetZFar(const ROS::DABaseCamera* camera);

DXDEC void __cdecl CameraSetZFar(const ROS::DABaseCamera* camera, float zFar);

DXDEC void __cdecl CameraMoveBy(const ROS::DABaseCamera* camera, float dx, float dy, float dz);

DXDEC_ROS void __cdecl CameraGetScreenToPoint(const ROS::DABaseCamera* camera, int screenX, int screenY, Vector& worldPoint);

DXDEC_ROS void __cdecl CameraGetPointToScreen(const ROS::DABaseCamera* camera, Vector& worldPoint, int &screenX, int &screenY, float &depth);

DXDEC_ROS BaseCamera* CameraGetBaseCamera(const ROS::DABaseCamera* camera);

#ifdef __cplusplus 
}
#endif
#endif