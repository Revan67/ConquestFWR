// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "StaticSceneEntitySoundListener.h"
#include "ASoundListener.h"
// --------------------------------------------------------------------------
StaticSceneEntitySoundListener::StaticSceneEntitySoundListener(const ASoundListener* targetListener)
: mRefCount(1), mTargetListener(targetListener)
{
}
// --------------------------------------------------------------------------
GENRESULT COMAPI StaticSceneEntitySoundListener::get_ear_orientation(Vector *front, Vector *up)
{
	if(mTargetListener)
	{
		const ROS::Orientation	orientation = mTargetListener->GetOrientation();
		
		*front = orientation.GetK();
		*up = orientation.GetJ();
	}
	else
	{
		front->set(0, 0, 1);
		up->set(0, 1, 0);
	}

	return GR_OK;
}
// --------------------------------------------------------------------------
GENRESULT COMAPI StaticSceneEntitySoundListener::get_ear_position(Vector *position)
{
	if(mTargetListener)
	{
		*position = mTargetListener->GetLocation().GetVector();
	}
	else
	{
		position->set(0, 0, 0);
	}

	return GR_OK;
}
// --------------------------------------------------------------------------
