// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "AudioStateAccessor.h"
#include "AAudibleSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN AudioStateAccessor::AudioStateAccessor(AAudibleSceneEntity& owner)
: mOwner(owner)
{
}
//---------------------------------------------------------------------------
CPP_DEFN void AudioStateAccessor::Start(const ROSString& name, const StringList& descriptionStrings, Time startPoint)
{
    GetOwner().StartSound(name, descriptionStrings, startPoint);
}
// --------------------------------------------------------------------------
CPP_DEFN AAudibleSceneEntity& AudioStateAccessor::GetOwner()
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN const AAudibleSceneEntity& AudioStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
