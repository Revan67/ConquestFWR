// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "FlaggedOrientation.h"
#include "AStaticSceneEntity.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kInterpolationType,
	kTargetName			// Added ROS Version 2.0.2.0
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN void FlaggedOrientation::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void FlaggedOrientation::WriteSubObject(std::ostream& oStream) const
{
 	OStreamWiz<FieldID>	oWiz(oStream);

	int type = GetInterpolationType();

    oWiz.Put(kInterpolationType, type);

	ROSString	targetName;

	if(GetInterpolationType() == kLookAt)
	{
		targetName = GetTargetEntity()->GetConstSceneEntityStateAccessor()->GetName();
	}

	oWiz.Put(kTargetName, targetName);
}
// --------------------------------------------------------------------------
CPP_DEFN void FlaggedOrientation::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void FlaggedOrientation::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    int	type;

    iWiz.Get(kInterpolationType, type);

	SetInterpolationType((InterpolationType)type);

	AStaticSceneEntity*	target = NULL;

	if(GetInterpolationType() == kLookAt && iWiz.Has(kTargetName))
	{
		ROSString	targetName;

		iWiz.Get(kTargetName, targetName);

		SceneEntityRemapper::Add(targetName, new SceneEntityRemap<AStaticSceneEntity*>(&mTargetEntity));
	}
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
