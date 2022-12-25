// Author: Shaival Varma
// --------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include "PCH.h"
#include <windows.h>

#include "DAHardPoints.h"
#include "Engine.h"
#include "HardPoints.h"
#include "Misc.h"
#include "CodeMsg.h"
#include "Deform.h"
#include "DABaseCamera.h"
// --------------------------------------------------------------------------
const ROS::DAHardPoints* GetDAHardPoints(const HardPoints* hardPoints)
{
	return const_cast<ROS::DAHardPoints*>(reinterpret_cast<const ROS::DAHardPoints*>(hardPoints));
}
// --------------------------------------------------------------------------
HardPoints* GetHardPoints(const ROS::DAHardPoints* dAHardPoints)
{
	return const_cast<HardPoints*>(reinterpret_cast<const HardPoints*>(dAHardPoints));
}
// --------------------------------------------------------------------------
const ROS::DAHardPoints* HardPointsCreate(const ROS::DACompoundObject* dACompoundObject)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	try
	{
		return GetDAHardPoints(new HardPoints(objIdx, ENG));
	}
	catch(...)
	{	
		return NULL;
	}
}
// --------------------------------------------------------------------------
void HardPointsDestroy(const ROS::DAHardPoints* dAHardPoints)
{
	if(dAHardPoints)
	{	
		delete GetHardPoints(dAHardPoints);
	}
}
// --------------------------------------------------------------------------
unsigned int HardPointsGetCount(const ROS::DAHardPoints* dAHardPoints)
{
	if(dAHardPoints)
	{	
		return GetHardPoints(dAHardPoints)->Count();
	}
	else
	{	
		return 0;
	}
}
// --------------------------------------------------------------------------
const char* HardPointsGetHardPointName(const ROS::DAHardPoints* dAHardPoints, unsigned int idx)
{
	if(dAHardPoints)
	{	
		return (*GetHardPoints(dAHardPoints))[idx].GetName().c_str();
	}
	else
	{	
		return 0;
	}
}
// --------------------------------------------------------------------------
void HardPointsGetHardPointPosition(const ROS::DAHardPoints* dAHardPoints, unsigned int idx, Vector& position)
{
	if(dAHardPoints)
	{	
		position = (*GetHardPoints(dAHardPoints))[idx].GetPosition();
	}
	else
	{	
		position.zero();
	}
}
// --------------------------------------------------------------------------
void HardPointsGetHardPointOrientation(const ROS::DAHardPoints* dAHardPoints, unsigned int idx, ROS::Matrix& orientation)
{
	if(dAHardPoints)
	{	
		Matrix orient = (*GetHardPoints(dAHardPoints))[idx].GetOrientation();

		orientation.SetI(orient.get_i());
		orientation.SetJ(orient.get_j());
		orientation.SetK(orient.get_k());
	}
	else
	{	
		orientation.SetIdentity();
	}
}
// --------------------------------------------------------------------------
void HardPointsDraw(const ROS::DAHardPoints* dAHardPoints, const ROS::DABaseCamera* camera)
{
	const Transform	tr = CameraGetTransform(camera);
    const Transform	modelView = tr.get_inverse();

	if(dAHardPoints)
	{
		GetHardPoints(dAHardPoints)->Draw(modelView);
	}
}
// --------------------------------------------------------------------------
const ROS::HardPointHost* HardPointsGetHardPointHost(const ROS::DAHardPoints* dAHardPoints, unsigned int idx)
{
	return GetHardPointHost(GetHardPoints(dAHardPoints)->GetInstanceIndex());
}
// --------------------------------------------------------------------------
void HardPointsAttachHardPointToParent(const ROS::DAHardPoints* dAHardPoints, unsigned int idx, const ROS::HardPointHost* parentHardPointHost, const ROS::ROSString& parentHardPointName)
{
	ASSERT(dAHardPoints && parentHardPointHost);

	const INSTANCE_INDEX parentInstance = GetHardPointHostInstanceIndex(parentHardPointHost);

	const ROS::ROSString childHardPointName = HardPointsGetHardPointName(dAHardPoints, idx);

	const int	result = HARDPOINT->connect(parentInstance, parentHardPointName.c_str(), GetHardPoints(dAHardPoints)->GetInstanceIndex(), childHardPointName.c_str());

	ASSERT(result == 0);
}
// --------------------------------------------------------------------------
void HardPointsDetachHardPointFromParent(const ROS::DAHardPoints* dAHardPoints, unsigned int idx, const ROS::HardPointHost* parentHardPointHost, const ROS::ROSString& parentHardPointName)
{
	ASSERT(dAHardPoints && parentHardPointHost);

	const INSTANCE_INDEX parentInstance = GetHardPointHostInstanceIndex(parentHardPointHost);

	BOOL32	result = MODEL->disconnect(parentInstance, GetHardPoints(dAHardPoints)->GetInstanceIndex());
}
// --------------------------------------------------------------------------
