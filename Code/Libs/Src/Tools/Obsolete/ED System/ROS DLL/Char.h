// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Char_h
#define Char_h
// --------------------------------------------------------------------------
#include <exception>
#include "DARenderPipeline.h"
#include "StringType.h"
// --------------------------------------------------------------------------
typedef void (*CallbackOnExit) (void);
// --------------------------------------------------------------------------

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
// --------------------------------------------------------------------------
class Vector;
class Matrix;
struct ISystemContainer;
struct IEngine;
struct ILightManager;

namespace ROS
{
class Matrix;
class Matrix4x4;
class DABaseCamera;
class DALight;
class IntersectInfo;
}	// namespace ROS
// --------------------------------------------------------------------------
class ExInvalidINIFile: public std::exception
{
	public:
		ExInvalidINIFile(const ROS::ROSString& iniFile)
		{	mMessage = ROS::ROSString("Invalid INI file: ") + iniFile;
		}

		virtual const char* what() const throw()
		{
		  return mMessage.c_str();
		}

	private:
		ROS::ROSString	mMessage;
};
// --------------------------------------------------------------------------
class ExDACOMAcquisitionFailure: public std::exception
{
	public:
		virtual const char* what() const throw()
		{
		  return "Failed to Acquire DACOM";
		}
};
// --------------------------------------------------------------------------
class ExGameSystemStartupFailure: public std::exception
{
	public:
		virtual const char* what() const throw()
		{
		  return "Failed to startup system";
		}

};
// --------------------------------------------------------------------------
class ExInterfaceAcquisitionFailure: public std::exception
{
	public:
		ExInterfaceAcquisitionFailure(const ROS::ROSString& interfaceName)
		{	mMessage = ROS::ROSString("Failed to acquire interface: ") + interfaceName;
		}

		virtual const char* what() const throw()
		{
		  return mMessage.c_str();
		}

	private:
		ROS::ROSString	mMessage;
};
// --------------------------------------------------------------------------
class ExDataAcquisitionFailure: public std::exception
{
	public:
		virtual const char* what() const throw()
		{
		  return "Failed to load data";
		}
};
// --------------------------------------------------------------------------
#ifdef __cplusplus 
extern "C" { 
#endif
// --------------------------------------------------------------------------
// World Light
DXDEC_ROS void __cdecl WorldSetAmbientLight(unsigned int red, unsigned int green, unsigned int blue);

DXDEC_ROS void __cdecl WorldSetMaterialAmbient(unsigned int red, unsigned int green, unsigned int blue);

DXDEC_ROS void __cdecl WorldSetMaterialDiffuse(unsigned int red, unsigned int green, unsigned int blue);

DXDEC_ROS void __cdecl WorldSetMaterialEmission(unsigned int red, unsigned int green, unsigned int blue);

DXDEC void __cdecl WorldGetAmbientLight(unsigned int & red, unsigned int & green, unsigned int & blue);

DXDEC void __cdecl WorldGetMaterialAmbient(unsigned int & red, unsigned int & green, unsigned int & blue);

DXDEC void __cdecl WorldGetMaterialDiffuse(unsigned int & red, unsigned int & green, unsigned int & blue);

DXDEC void __cdecl WorldGetMaterialEmission(unsigned int & red, unsigned int & green, unsigned int & blue);

// Lights
DXDEC const ROS::DALight* __cdecl LightCreate();

DXDEC void __cdecl LightDestroy(const ROS::DALight* camera);

DXDEC void __cdecl LightSetPosition(const ROS::DALight* light, const Vector& position);

DXDEC_ROS void __cdecl LightUpdateLighting(const ROS::DABaseCamera* camera);

// Misc
DXDEC_ROS void __cdecl CharMain(HINSTANCE hInstance, CallbackOnExit callbackFunc, const char* iniFile, unsigned int colorBpp, unsigned int depthBpp, IRenderPipeline** renderPipe);

DXDEC_ROS void __cdecl CharAppExit();

DXDEC_ROS ISystemContainer* __cdecl CharGetSystemContainer();

DXDEC_ROS IEngine* __cdecl CharGetEngine();

DXDEC ILightManager* __cdecl CharGetLightManager();

DXDEC void __cdecl GameEngineUpdate(float dt);

DXDEC void __cdecl DAInterpolateOrientation(const ROS::Matrix& previousOrientation, const ROS::Matrix& nextOrientation, float t, ROS::Matrix& orientationAtT);

DXDEC void __cdecl CharRenderLight(const Transform& transform, const ROS::DABaseCamera* camera);

DXDEC void __cdecl CharRenderCamera(const Transform& transform, const ROS::DABaseCamera* camera);

DXDEC bool __cdecl CharIntersectCamera(const ROS::IntersectInfo& intersectInfo, const Transform& transform, float* distance);
// --------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
// --------------------------------------------------------------------------
#endif