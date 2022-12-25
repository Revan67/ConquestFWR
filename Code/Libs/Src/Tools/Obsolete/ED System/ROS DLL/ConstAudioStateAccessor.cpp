// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "ConstAudioStateAccessor.h"
#include "AAudibleSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN ConstAudioStateAccessor::ConstAudioStateAccessor(const AAudibleSceneEntity& owner)
: mOwner(owner)
{
}
// --------------------------------------------------------------------------
CPP_DEFN const AAudibleSceneEntity& ConstAudioStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
