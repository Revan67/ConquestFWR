// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DADeformableObject_h
#define DADeformableObject_h
// --------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "DACOM.h"
#include "StringType.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
class Vector;
class Matrix;
class EventIterator;
struct IEngine;
struct Mesh;
typedef S32 INSTANCE_INDEX;

namespace ROS
{
class Matrix;
class Matrix4x4;
class DADeformableObject;
class DABaseCamera;
class IntersectInfo;
class StringList;
class HardPoint;
class HardPointHost;
class IKState;
class DAIK;

typedef void (__cdecl *EventHandler) (unsigned int channel_id, const EventIterator& event_iter);
}	// namespace ROS
// --------------------------------------------------------------------------
class ExAnimationArchetypeCreationFailure: std::exception
{
	public:
		ExAnimationArchetypeCreationFailure(const ROS::ROSString& filename)
		{	mMessage = ROS::ROSString("Failed to create animation archetype from file: ") + filename;
		}

		virtual const char* what() const throw()
		{
		  return mMessage.c_str();
		}

	private:
		ROS::ROSString	mMessage;
};
// --------------------------------------------------------------------------
#ifdef __cplusplus 
extern "C" { 
#endif
// --------------------------------------------------------------------------
// startup and shutdown for ROSSystem's use
bool __cdecl ROSSystemStartup(IDAComponent* system, IEngine* engine, HWND wndH);

void __cdecl ROSSystemShutdown();

// Deformables
CPP_DECL const ROS::DADeformableObject* __cdecl DeformableObjectCreate(const ROS::StringList& descriptionStrings, ROS::EventHandler EventHandlerFunction);

CPP_DECL void __cdecl DeformableObjectDestroy(const ROS::DADeformableObject* defObj, const ROS::StringList& descriptionStrings);

void __cdecl DeformableObjectLoadTextures(const char * filename);

void __cdecl DeformableObjectRender(const ROS::DADeformableObject* defObj, const ROS::DABaseCamera* camera);

void __cdecl DeformableObjectAddArchetypeTimeTagChannel(const ROS::DADeformableObject* defObj, const ROS::ROSString& motionName);

void __cdecl DeformableObjectReplaceArchetypeTimeTagChannelData(const ROS::DADeformableObject* defObj, const ROS::ROSString& motionName, float time[], int tag[], unsigned int count);

unsigned int __cdecl DeformableObjectGetMotionCount(const ROS::DADeformableObject* defObj);

ROS::ROSString __cdecl DeformableObjectGetMotionName(const ROS::DADeformableObject* defObj, int motionIdx);

float __cdecl DeformableObjectGetMotionLength(const ROS::DADeformableObject* defObj, const ROS::ROSString& motionName);

void __cdecl DeformableObjectStartMotion(const ROS::DADeformableObject* defObj, const ROS::ROSString& motionName, bool loop, float startTime, float transition);

void __cdecl DeformableObjectStopMotion(const ROS::DADeformableObject* defObj);

void __cdecl DeformableObjectPauseMotion(const ROS::DADeformableObject* defObj);

void __cdecl DeformableObjectResumeMotion(const ROS::DADeformableObject* defObj);

void __cdecl DeformableObjectUpdate(const ROS::DADeformableObject* defObj);

#if 0
float	__cdecl DeformableObjectGetCurrentMotionTime(const ROS::DADeformableObject* defObj);

bool __cdecl DeformableObjectIsMotionOver(const ROS::DADeformableObject* defObj);
#endif

void __cdecl DeformableObjectSetPosition(const ROS::DADeformableObject* defObj, const Vector& position);

void __cdecl DeformableObjectSetPositionViaEngine(const ROS::DADeformableObject* defObj, const Vector& position);

void __cdecl DeformableObjectSetPositionOnly(const ROS::DADeformableObject* defObj, const Vector& position);

void __cdecl DeformableObjectGetPosition(const ROS::DADeformableObject* defObj, Vector& position);

void __cdecl DeformableObjectSetOrientation(const ROS::DADeformableObject* defObj, const ROS::Matrix& orientation);

void __cdecl DeformableObjectGetOrientation(const ROS::DADeformableObject* defObj, ROS::Matrix& orientation);

float __cdecl DeformableObjectGetFloorHeight(const ROS::DADeformableObject* defObj);

bool __cdecl DeformableObjectIntersect(const ROS::DADeformableObject* defObj, const ROS::IntersectInfo& intersectInfo, float* distance);

unsigned int __cdecl DeformableObjectGetHardpointCount(const ROS::DADeformableObject* defObj);

void __cdecl DeformableObjectGetHardPointName(const ROS::DADeformableObject* defObj, unsigned int index, ROS::ROSString& hardPointName);

void __cdecl DeformableObjectGetHardPointPosition(const ROS::DADeformableObject* defObj, unsigned int index, Vector& position);

void __cdecl DeformableObjectGetHardPointOrientation(const ROS::DADeformableObject* defObj, unsigned int index, ROS::Matrix& orientation);

void __cdecl DeformableObjectRenderHardpoints(const ROS::DADeformableObject* defObj, const ROS::DABaseCamera* camera);

const ROS::HardPointHost* __cdecl DeformableObjectGetHardPointHost(const ROS::DADeformableObject* defObj, unsigned int index);

void __cdecl DeformableObjectAttachHardPointToParent(const ROS::DADeformableObject* defObj, unsigned int index, const ROS::HardPointHost* parentHardPointHost, const ROS::ROSString& parentHardPointName);

void __cdecl DeformableObjectDetachHardPointFromParent(const ROS::DADeformableObject* defObj, unsigned int index, const ROS::HardPointHost* parentHardPointHost, const ROS::ROSString& parentHardPointName);

const ROS::DAIK* __cdecl DeformableObjectStartIK(const ROS::DADeformableObject* defObj, const ROS::IKState& iKState, const Vector& location, const Matrix& orient, float transition);

void __cdecl DeformableObjectGetEndEffectorPosition (const ROS::DADeformableObject* defObj, const ROS::IKState& iKState, Vector& position);

void __cdecl DeformableObjectStopIK(const ROS::DADeformableObject* defObj, const ROS::DAIK* dAIK);

long __cdecl DeformableObjectGetRoot(const ROS::DADeformableObject* defObj);

void __cdecl DeformableObjectRenderSkeleton(const ROS::DADeformableObject* defObj, const ROS::DABaseCamera* camera, float boneLength);

// --------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
// --------------------------------------------------------------------------
#endif	// DADeformableObject_h