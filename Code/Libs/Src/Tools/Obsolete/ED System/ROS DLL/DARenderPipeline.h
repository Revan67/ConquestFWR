// Author: Shaival Varma
// --------------------------------------------------------------------------
#include <Windows.h>

#include "RendPipeline.h"
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


#undef DXDEC_ROS
#undef DXDEF_ROS

#ifdef BUILD_ROS_DLL
	#define DXDEC_ROS __declspec(dllexport)
	#define DXDEF_ROS __declspec(dllexport)
#else
	#define DXDEC_ROS __declspec(dllimport)
#endif

#endif

#endif

// --------------------------------------------------------------------------
extern IRenderPipeline*	PIPE;
// --------------------------------------------------------------------------
#ifdef __cplusplus 
extern "C" { 
#endif
// --------------------------------------------------------------------------
bool __cdecl RPStartup(unsigned int colorBpp, unsigned int depthBpp);
void __cdecl RPShutDown();
// --------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
