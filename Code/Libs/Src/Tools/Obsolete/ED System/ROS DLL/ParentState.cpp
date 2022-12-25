//---------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ParentState.h"
#include "ACompoundSceneEntity.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kParentEvent,
	kChildHardPointName,
	kParentName,
	kParentHardPointName
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN void ParentState::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void ParentState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    const int parentEvent = mParentEventID;

	oWiz.Put(kParentEvent, parentEvent);
	oWiz.Put(kChildHardPointName, mChildHardPointName);

	ROSString	parentName;
	
	if(mParentEventID == kAttachToParent || mParentEventID == kDetachFromParent)
	{
		parentName = mParentEntity->GetConstSceneEntityStateAccessor()->GetName();
	}

	oWiz.Put(kParentName, parentName);
    oWiz.Put(kParentHardPointName, mParentHardPointName);
}
// --------------------------------------------------------------------------
CPP_DEFN void ParentState::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
inline void ParentState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    int parentEvent;

	iWiz.Get(kParentEvent, parentEvent);
    mParentEventID = (ParentEventID)parentEvent;

	iWiz.Get(kChildHardPointName, mChildHardPointName);

	ROSString	parentName;

	iWiz.Get(kParentName, parentName);

	if(mParentEventID == kAttachToParent || mParentEventID == kDetachFromParent)
	{
		SceneEntityRemapper::Add(parentName, new SceneEntityRemap<ACompoundSceneEntity*>(&mParentEntity));
	}
	else
	{
		mParentEntity = NULL;
	}

	iWiz.Get(kParentHardPointName, mParentHardPointName);
}
// --------------------------------------------------------------------------
}
