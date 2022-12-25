// --------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include "PCH.h"
#include <windows.h>
//

#include <stdio.h>
#include <stdlib.h>


#include "DACom.h"
#include "3DMath.h"
#include "GameSys.h"
/*****NOTE: Explicit include to avoid confusion with Model.h in ROS dll******/
#include <Model.h>
#include "LightMan.h"
#include "ITXMLib.h"
#include "IAnim.h"
#include "IHardPoint.h"
#include "BaseLight.h"
#include "FileSys.h"
#include "drivermgr.h"
#include "BaseCam.h"
#include "Char.h"
#include "CodeMsg.h"
#include "MatrixUtil.h"
#include "DARenderPipeline.h"
#include "Matrix4x4.h"
#include "Misc.h"
#include "DACompoundObject.h"
#include "StringUtils.h"
#include "StringList.h"
#include "Deform.h"
// --------------------------------------------------------------------------
//
// Globals.
//

ICOManager *		DACOM = NULL;
IEngine *			ENG = NULL;
IModel *			MODEL = NULL;
IAnimation *		ANIM = NULL;
ILightManager *		LIGHT = NULL;
ITXMLib *			TXMLIB = NULL;
IChannel*			CHANNEL = NULL;
IRenderer*			RENDERER = NULL;
IHardpoint*			HARDPOINT = NULL;
bool				gDeformOpened = false;
HINSTANCE			GlobalInstance;
GameSystem			GAME;
int					CurrentDriver = 0;
char				Work[256];
DriverMgr			Drivers;

static const ROS::DACompoundObject*	gLight = NULL;
static const ROS::DACompoundObject*	gCamera = NULL;

CallbackOnExit		gCallbackFunc = NULL;

// --------------------------------------------------------------------------

//
// Externs.
//
//int MarkAllocatedBlocks (IHeap *pHeap);
//int PrintHeap (IHeap *pHeap);
// --------------------------------------------------------------------------
BaseLight* GetBaseLight(const ROS::DALight* light)
{
	return const_cast<BaseLight*>(reinterpret_cast<const BaseLight*>(light));
}
// --------------------------------------------------------------------------
const ROS::DALight* GetDALight(const BaseLight* light)
{
	return reinterpret_cast<const ROS::DALight*>(light);
}
// --------------------------------------------------------------------------









#if 0
// --------------------------------------------------------------------------
INSTANCE_INDEX CreateLight()
{
	PROPERTY light_properties[] = 
	{
		DP			("Red",		255),
		DP			("Green",	255),
		DP			("Blue",	255),
		DP_SINGLE	("Range",	5000),
		DP			(NULL,		0)
	};

	INSTANCE_INDEX result = ENG->create_instance("Light", light_properties);

	return result;
}
#endif







// --------------------------------------------------------------------------
//
// Exit handlers.
//
static int exit_handler_active = 0;

void WinClean(void)
{
	//OutputDebugString("WinClean called\n");

	if (exit_handler_active)
	{
		//OutputDebugString("Re-entered exit handler!\n");
		return;
	}

	exit_handler_active = 1;

    if(gDeformOpened)
	{	
		DeformClose();
		gDeformOpened = false;
	}

	if(gCamera)
	{	
		CompoundObjectDestroy(gCamera);
		gCamera = NULL;
	}

	if(gLight)
	{	
		CompoundObjectDestroy(gLight);
		gLight = NULL;
	}
	
	if(HARDPOINT)
	{
		HARDPOINT->Release();
		HARDPOINT = NULL;
	}

	if(RENDERER)
	{
		RENDERER->Release();
		RENDERER = NULL;
	}
	
	if(CHANNEL)
	{	
		CHANNEL->Release();
		CHANNEL = NULL;
	}
	
	if(TXMLIB)
	{	
		TXMLIB->Release();
		TXMLIB = NULL;
	}
	
	if(LIGHT)
	{	
		LIGHT->Release();
		LIGHT = NULL;
	}
	
	if(ANIM)
	{	
		ANIM->Release();
		ANIM = NULL;
	}

	if(MODEL)
	{	
		MODEL->Release();
		MODEL = NULL;
	}

	if(ENG)
	{	
		ENG->Release();
		ENG = NULL;
	}
	
	if(PIPE)
	{	
		RPShutDown();

		PIPE->Release();
		PIPE = NULL;
	}

	GAME.shut_down();

	if(DACOM != NULL)
	{
//		PrintHeap(HEAP);
		DACOM->ShutDown();
		DACOM = NULL;
	}

	//OutputDebugString("Final exit OK\n");
}
// --------------------------------------------------------------------------
void WinExit(int code)
{
	//OutputDebugString("WinExit called\n");

	if(gCallbackFunc)
	{
		gCallbackFunc();
	}

	if (!exit_handler_active)
	{
		WinClean();
	}

	exit(code);
}
// --------------------------------------------------------------------------
#ifdef __cplusplus 
extern "C" { 
#endif
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CharAppExit(void)
{
	//OutputDebugString("AppExit() called via atexit()\n");

	if (!exit_handler_active)
	{
		WinClean();
	}

	return;
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CharMain(HINSTANCE hInstance, CallbackOnExit callbackFunc, const char* iniFile, unsigned int colorBpp, unsigned int depthBpp, IRenderPipeline** renderPipe)
{
//
// Initialize component object manager
//
	gCallbackFunc = callbackFunc;

	GlobalInstance = hInstance;

	//OutputDebugString("Initializing DACOM\n");					 
	DACOM = DACOM_Acquire();

	if(DACOM == NULL)
	{
		throw ExDACOMAcquisitionFailure();
	}

	if(GR_OK != DACOM->SetINIConfig(iniFile))
	{
		DACOM->ShutDown();

		throw ExInvalidINIFile(ROS::ROSString(iniFile));
	}


//
// Initialize game system shell
//
	//OutputDebugString("Initializing game system interface\n");

	bool allow_multiple_instances = true;
	bool ok = false;

	if(GAME.startup(hInstance, "ROS", WinExit) == 0)
	{
		throw ExGameSystemStartupFailure();
	}

//	MarkAllocatedBlocks(HEAP);

	GAME.GS->QueryInterface(IID_IRenderPipeline,	(void**) &PIPE);
	GAME.ENG->QueryInterface(IID_IEngine,			(void**) &ENG);
	GAME.ENG->QueryInterface(IID_IModel,			(void**) &MODEL);
	GAME.ENG->QueryInterface("IAnimation",			(void**) &ANIM);
	GAME.ENG->QueryInterface("ILightManager",		(void**) &LIGHT);
	GAME.ENG->QueryInterface("ITXMLib",				(void**) &TXMLIB);
	GAME.ENG->QueryInterface(IID_IChannel,			(void**) &CHANNEL);
	GAME.ENG->QueryInterface(IID_IRenderer,			(void**) &RENDERER);
	GAME.ENG->QueryInterface(IID_IHardpoint,		(void**) &HARDPOINT);

	gDeformOpened = DeformOpen(GAME.GS, ENG, NULL);

	if(!(PIPE && ENG && MODEL && ANIM && LIGHT && TXMLIB && CHANNEL && RENDERER && HARDPOINT && gDeformOpened))
	{
		ROS::ROSString	interfaceName;

		if(!PIPE)
		{
			interfaceName = "IRenderPipeline";
		}
		else if(!ENG)
		{	
			interfaceName = "IEngine";
		}
		else if(!MODEL)
		{	
			interfaceName = "IModel";
		}
		else if(!ANIM) 
		{	
			interfaceName = "IAnimation";
		}
		else if(!LIGHT)
		{	
			interfaceName = "ILightManager";
		}
		else if(!TXMLIB)
		{	
			interfaceName = "ITXMLib";
		}
		else if(!CHANNEL)
		{	
			interfaceName = "IChannel";
		}
		else if(!RENDERER)
		{	
			interfaceName = "IRenderer";
		}
		else if(!HARDPOINT)
		{
			interfaceName = "IHardPoint";
		}
		else if(!gDeformOpened)
		{	
			interfaceName = "Deform System";
		}
		else
		{	
			interfaceName = "Unknown";
		}

		WinClean();

		throw ExInterfaceAcquisitionFailure(interfaceName);
	}

	*renderPipe = PIPE;
		
	if(!RPStartup(colorBpp, depthBpp))
	{
		WinClean();

		throw ExGameSystemStartupFailure();
	}

	const int	size = 255;
	char		moduleFileName[size];
	
	GetModuleFileName(NULL, moduleFileName, size);

	const ROS::ROSString	dataFolderName = GetFilePath(moduleFileName) + "ROS Data\\";
	ROS::StringList			list;

	// Light
	list.Add(dataFolderName + "Light.cmp");
	gLight = CompoundObjectCreate(list);

	// Camera
	list.Clear();
	list.Add(dataFolderName + "Camera.cmp");
	gCamera = CompoundObjectCreate(list);

	if(!gLight || !gCamera)
	{
		MessageBox (NULL, "Failed to load editor 3D objects.\n", "Fatal Error", MB_OK | MB_ICONSTOP);
		WinClean();
		
		throw ExDataAcquisitionFailure();
	}
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl WorldSetAmbientLight(unsigned int red, unsigned int green, unsigned int blue)
{
	LIGHT->set_ambient_light(red, green, blue);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl WorldSetMaterialAmbient(unsigned int red, unsigned int green, unsigned int blue)
{
	LIGHT->set_material_ambient(red, green, blue);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl WorldSetMaterialDiffuse(unsigned int red, unsigned int green, unsigned int blue)
{
	LIGHT->set_material_diffuse(red, green, blue);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl WorldSetMaterialEmission(unsigned int red, unsigned int green, unsigned int blue)
{
	LIGHT->set_material_emission(red, green, blue);
}
// --------------------------------------------------------------------------
DXDEF void __cdecl WorldGetAmbientLight(unsigned int & red, unsigned int & green, unsigned int & blue)
{
	int sRed, sGreen, sBlue;

	LIGHT->get_ambient_light(sRed, sGreen, sBlue);

	ASSERT(sRed >= 0 && sGreen >= 0 && sBlue >= 0);

	red = sRed;
	green = sGreen;
	blue = sBlue;
}
// --------------------------------------------------------------------------
DXDEF void __cdecl WorldGetMaterialAmbient(unsigned int & red, unsigned int & green, unsigned int & blue)
{
	int sRed, sGreen, sBlue;

	LIGHT->get_material_ambient(sRed, sGreen, sBlue);

	ASSERT(sRed >= 0 && sGreen >= 0 && sBlue >= 0);

	red = sRed;
	green = sGreen;
	blue = sBlue;
}
// --------------------------------------------------------------------------
DXDEF void __cdecl WorldGetMaterialDiffuse(unsigned int & red, unsigned int & green, unsigned int & blue)
{
	int sRed, sGreen, sBlue;

	LIGHT->get_material_diffuse(sRed, sGreen, sBlue);

	ASSERT(sRed >= 0 && sGreen >= 0 && sBlue >= 0);

	red = sRed;
	green = sGreen;
	blue = sBlue;
}
// --------------------------------------------------------------------------
DXDEF void __cdecl WorldGetMaterialEmission(unsigned int & red, unsigned int & green, unsigned int & blue)
{
	int sRed, sGreen, sBlue;

	LIGHT->get_material_emission(sRed, sGreen, sBlue);

	ASSERT(sRed >= 0 && sGreen >= 0 && sBlue >= 0);

	red = sRed;
	green = sGreen;
	blue = sBlue;
}
// --------------------------------------------------------------------------
DXDEF const ROS::DALight* __cdecl LightCreate()
{
	BaseLight* light = new BaseLight(ENG);
	light->color.r = 255;
	light->color.g = 255;
	light->color.b = 255;
	light->range = 1000;
	light->infinite = false;
	light->cutoff = 180.0;
	light->map = 0;
	light->set_On(true);

	light->set_position(Vector(0, 0, 0));
	ILight * light_list = light;
	LIGHT->activate_lights(&light_list, 1);

#if 0
	LIGHT->set_ambient_light(32, 32, 32);
#else
	LIGHT->set_ambient_light(255, 255, 255);
#endif
	LIGHT->set_material_ambient(255, 255, 255);
	LIGHT->set_material_diffuse(255, 255, 255);
	LIGHT->set_material_emission(0, 0, 0);

#if 0	// Disabling for now
#if 1
	// The following 2 lines were moved here from CharAppMain1() since they need a glRenderContext, 
	// and need to be set up for each context that the app creates
	GLuint txm = LoadTexture("C:\\Develop\\Projects\\ROS System\\shademap.bmp");
	glBindTexture(GL_TEXTURE_2D, txm);
#endif
#endif

	return GetDALight(light);
}
// --------------------------------------------------------------------------
DXDEF void __cdecl LightDestroy(const ROS::DALight* light)
{
	delete GetBaseLight(light);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl LightUpdateLighting(const ROS::DABaseCamera* camera)
{
	LIGHT->update_lighting(GetBaseCamera(camera));
}
// --------------------------------------------------------------------------
DXDEF void __cdecl LightSetPosition(const ROS::DALight* light, const Vector& position)
{
	GetBaseLight(light)->set_position(position);
}
// --------------------------------------------------------------------------
DXDEF ISystemContainer* __cdecl CharGetSystemContainer()
{
	ASSERT(GAME.GS != NULL);

	return GAME.GS;
}
// --------------------------------------------------------------------------
DXDEF IEngine* __cdecl CharGetEngine()
{
	ASSERT(ENG != NULL);

	return ENG;
}
// --------------------------------------------------------------------------
DXDEF ILightManager* __cdecl CharGetLightManager()
{
	ASSERT(LIGHT != NULL);

	return LIGHT;
}
// --------------------------------------------------------------------------
DXDEF void __cdecl GameEngineUpdate(float dt)
{
	ENG->update(dt);
}
// --------------------------------------------------------------------------
DXDEF void __cdecl DAInterpolateOrientation(const ROS::Matrix& previousOrientation, const ROS::Matrix& nextOrientation, float t, ROS::Matrix& orientationAtT)
{
	Matrix	previousOrient(previousOrientation.GetI(), previousOrientation.GetJ(), previousOrientation.GetK());
	Matrix	nextOrient(nextOrientation.GetI(), nextOrientation.GetJ(), nextOrientation.GetK());

	Quaternion	previousQuat(previousOrient);
	Quaternion	nextQuat(nextOrient);
	Quaternion	quatAtT = slerp(previousQuat, nextQuat, t);

	Matrix	orientAtT(quatAtT);

	orientationAtT.Set(orientAtT.get_i(), orientAtT.get_j(), orientAtT.get_k());
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CharRenderLight(const Transform& transform, const ROS::DABaseCamera* camera)
{
	if(gLight)
	{
		Transform	rotation;
		
		rotation.compose_rotation(X_AXIS, 180.0);

		CompoundObjectSetTransform(gLight, transform * rotation);

		CompoundObjectRenderObject(gLight, camera);
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CharRenderCamera(const Transform& transform, const ROS::DABaseCamera* camera)
{
	if(gCamera)
	{
		CompoundObjectSetTransform(gCamera, transform);
		CompoundObjectRenderObject(gCamera, camera);
	}
}
// --------------------------------------------------------------------------
DXDEF bool __cdecl CharIntersectCamera(const ROS::IntersectInfo& intersectInfo, const Transform& transform, float* distance)
{
	if(gCamera)
	{
		CompoundObjectSetTransform(gCamera, transform);

		return CompoundObjectIntersect(gCamera, intersectInfo, distance);
	}
	else
	{
		return false;
	}
}
// --------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
// --------------------------------------------------------------------------
