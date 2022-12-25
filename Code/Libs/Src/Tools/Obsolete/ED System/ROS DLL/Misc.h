#ifndef Misc_h
#define Misc_h

// --------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "GameSys.h"
#include "Renderer.h"
#include "IAnim.h"
// --------------------------------------------------------------------------
struct ILightManager;
struct IChannel;
struct ITXMLib;
struct IModel;
struct DeformableObject;
struct IHardpoint;
// --------------------------------------------------------------------------
extern IEngine*			ENG;
extern IAnimation*		ANIM;
extern ILightManager*	LIGHT;
extern GameSystem		GAME;
extern IChannel*		CHANNEL;
extern ITXMLib*			TXMLIB;
extern IModel*			MODEL;
extern IRenderer*		RENDERER;
extern IHardpoint*		HARDPOINT;
// --------------------------------------------------------------------------
struct BaseCamera;

namespace ROS
{
class DABaseCamera;
class HardPointHost;
}
// --------------------------------------------------------------------------
inline const ROS::DABaseCamera* GetBaseCam(BaseCamera* camera)
{
	return reinterpret_cast<const ROS::DABaseCamera*>(camera);
}
// --------------------------------------------------------------------------
inline BaseCamera* GetBaseCamera(const ROS::DABaseCamera* camera)
{
	return const_cast<BaseCamera*>(reinterpret_cast<const BaseCamera*>(camera));
}
// --------------------------------------------------------------------------
inline const ROS::DACompoundObject* GetDACompoundObject(INSTANCE_INDEX compoundObjIdx)
{
	return reinterpret_cast<const ROS::DACompoundObject*>(compoundObjIdx + 1);		// Adding 1 since 0 is a valid INSTANCE_INDEX, but don't want to return a NULL pointer (indicating failure)
}
// --------------------------------------------------------------------------
inline INSTANCE_INDEX GetCompoundObjectIndex(const ROS::DACompoundObject* dACompoundObject)
{
	return reinterpret_cast<INSTANCE_INDEX>(dACompoundObject) - 1;	// Subtracting 1: See GetDACompoundObject()
}
// --------------------------------------------------------------------------
inline DeformableObject* GetDeformableObject(const ROS::DADeformableObject* defObj)
{
	return const_cast<DeformableObject*>(reinterpret_cast<const DeformableObject*>(defObj));
}
// --------------------------------------------------------------------------
inline const ROS::DADeformableObject* GetDADeformableObject(const DeformableObject* object)
{
	return reinterpret_cast<const ROS::DADeformableObject*>(object);
}
// --------------------------------------------------------------------------
inline const ROS::HardPointHost* GetHardPointHost(INSTANCE_INDEX instanceIndex)
{
	return reinterpret_cast<const ROS::HardPointHost*>(instanceIndex + 1);		// Adding 1 since 0 is a valid INSTANCE_INDEX, but don't want to return a NULL pointer (indicating failure)
}
// --------------------------------------------------------------------------
inline INSTANCE_INDEX GetHardPointHostInstanceIndex(const ROS::HardPointHost* hardPointHost)
{
	return reinterpret_cast<INSTANCE_INDEX>(hardPointHost) - 1;	// Subtracting 1: See GetHardPointHost()
}
// --------------------------------------------------------------------------
#endif	// Misc_h