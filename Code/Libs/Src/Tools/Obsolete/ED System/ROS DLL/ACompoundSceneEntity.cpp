// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include <iostream>

#include "ACompoundSceneEntity.h"
#include "ACompoundSceneEntityState.h"
#include "ConstMotionStateAccessor.h"
#include "MotionStateAccessor.h"
#include "SceneEntityState.h"
#include "HardPoint.h"

// The following automatically adds a disconnect state when a connect state is added
#define DISCONNECT_CHILD_KEY_POINT


// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
int GetHardPointIndex(const ACompoundSceneEntity& entity, const ROSString& hardPointName)
{
	const std::auto_ptr<ConstMotionStateAccessor>	access = entity.GetConstMotionStateAccessor();

	const unsigned int	hardPointCount = access->GetHardPointCount();

	for(unsigned int idx = 0; idx < hardPointCount; ++idx)
	{
		if(access->GetHardPointName(idx) == hardPointName)
		{
			return idx;
		}
	}

	// Hardpoint not found!
	return -1;
}
// --------------------------------------------------------------------------
// ParentRoleCallback
// --------------------------------------------------------------------------
ACompoundSceneEntity::ParentRoleCallback::ParentRoleCallback(ACompoundSceneEntity& aCompoundSE)
: mACompoundSE(aCompoundSE), mLastParentRemoved(NULL)
{
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::ParentRoleCallback::RemoveStarted(const ParentRole::UpdateCallback::RoleType& role, Time time)
{
	mLastParentRemoved = role.GetState(time).GetParentEntity();
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::ParentRoleCallback::RemoveFinished(const ParentRole::UpdateCallback::RoleType& role, Time time)
{
	RemoveParent();
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::ParentRoleCallback::RemoveStarted(const ParentRole::UpdateCallback::RoleType& role, unsigned int timePointIndex)
{
	mLastParentRemoved = role.GetState(timePointIndex).GetParentEntity();
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::ParentRoleCallback::RemoveFinished(const ParentRole::UpdateCallback::RoleType& role, unsigned int timePointIndex)
{
	RemoveParent();
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::ParentRoleCallback::RemoveParent()
{
	if(mLastParentRemoved)
	{
		mACompoundSE.RemoveParent(*mLastParentRemoved);
	}
}
// --------------------------------------------------------------------------
// ACompoundSceneEntity methods
// --------------------------------------------------------------------------
// This is a protected constructor for use by descendants.
CPP_DEFN ACompoundSceneEntity::ACompoundSceneEntity()
{
}
// --------------------------------------------------------------------------
CPP_DEFN const std::auto_ptr<ConstMotionStateAccessor> ACompoundSceneEntity::GetConstMotionStateAccessor() const
{
	const APhysicalState*				state = &GetPhysicalState();
	const ACompoundSceneEntityState*	cState = dynamic_cast<const ACompoundSceneEntityState*>(state);
	ASSERT(cState);

    return std::auto_ptr<ConstMotionStateAccessor>(new ConstMotionStateAccessor(*this, *cState));
}
// --------------------------------------------------------------------------
CPP_DEFN std::auto_ptr<MotionStateAccessor> ACompoundSceneEntity::GetMotionStateAccessor()
{
	APhysicalState*				state = &GetPhysicalState();
	ACompoundSceneEntityState*	cState = dynamic_cast<ACompoundSceneEntityState*>(state);
	ASSERT(cState);

    return std::auto_ptr<MotionStateAccessor>(new MotionStateAccessor(*this, *cState));
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::Goto(Time time)
{
	BaseClass::Goto(time);
	
    GotoForMotionRole(time);
	GotoForParentRole(time);
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::AttachHardPointToParent(unsigned int hardPointIndex, const HardPoint& parent)
{
	if(!Performing())
    {
		const Time	currentTime = GetCurrentTimePoint();
		const int	parentRoleIndex = GetParentRoleIndex();
		ASSERT(parentRoleIndex >= 0);
		ARole*		aRole = &GetSceneEntityState().GetRole(parentRoleIndex);
        ParentRole* parentRole = dynamic_cast<ParentRole*>(aRole);
		ASSERT(parentRole);

		ROSString	childHardPointName;

		parentRole->StateUpdated(ParentState(ParentState::kAttachToParent, GetHardPointName(hardPointIndex), parent.GetACompoundSceneEntity(), parent.GetHardPointName()), currentTime);

		parent.GetACompoundSceneEntity()->GetSceneEntityStateAccessor()->AddListener(*this);
		GetSceneEntityState().AddSource(*parent.GetACompoundSceneEntity());

#ifdef DISCONNECT_CHILD_KEY_POINT
		parentRole->StateUpdated(ParentState(ParentState::kDetachFromParent, GetHardPointName(hardPointIndex), parent.GetACompoundSceneEntity(), parent.GetHardPointName()), currentTime + Time(5));

		parent.GetACompoundSceneEntity()->GetSceneEntityStateAccessor()->AddListener(*this);
		GetSceneEntityState().AddSource(*parent.GetACompoundSceneEntity());
#endif
    }
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::DetachHardPointFromParent(unsigned int hardPointIndex, const HardPoint& parent)
{
	if(!Performing())
    {
		const Time	currentTime = GetCurrentTimePoint();
		const int	parentRoleIndex = GetParentRoleIndex();
		ASSERT(parentRoleIndex >= 0);
		ARole*		aRole = &GetSceneEntityState().GetRole(parentRoleIndex);
        ParentRole* parentRole = dynamic_cast<ParentRole*>(aRole);
		ASSERT(parentRole);

		ROSString	childHardPointName;

		parentRole->StateUpdated(ParentState(ParentState::kDetachFromParent, GetHardPointName(hardPointIndex), parent.GetACompoundSceneEntity(), parent.GetHardPointName()), currentTime);

		parent.GetACompoundSceneEntity()->GetSceneEntityStateAccessor()->AddListener(*this);
		GetSceneEntityState().AddSource(*parent.GetACompoundSceneEntity());
    }
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::RemoveParent(ACompoundSceneEntity& aCompoundSE)
{
	GetSceneEntityState().RemoveSource(aCompoundSE);

	aCompoundSE.GetSceneEntityStateAccessor()->RemoveListener(*this);
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::Respond(const SceneEntityEvent& event)
{
    BaseClass::Respond(event);
	
	if(event.GetID() == SceneEntityEvent::kSourceEntityDeleted)
	{
		ASceneEntity*			entity = &event.GetSourceEntity();
		ACompoundSceneEntity*	aCompoundSE = dynamic_cast<ACompoundSceneEntity*>(entity);

		if(aCompoundSE)
		{
			const int		parentRoleIndex = GetParentRoleIndex();
			ASSERT(parentRoleIndex >= 0);
			ARole*			aRole = &GetSceneEntityState().GetRole(parentRoleIndex);
			ParentRole*		pRole = dynamic_cast<ParentRole*>(aRole);
			ASSERT(pRole);

			const unsigned int	originalTimePointCount = pRole->CountTimePoints();
			unsigned int		timePointCount = originalTimePointCount;
			unsigned int		idx = 0;

			if(originalTimePointCount > 0)
			{
				InitParentEventFlags();
			}

			while(idx < timePointCount)
			{
				if(pRole->GetState(idx).GetParentEntity() == aCompoundSE)
				{
					pRole->Remove(idx);

					--timePointCount;
				}
				else
				{
					++idx;
				}
			}

			if(originalTimePointCount != pRole->CountTimePoints())
			{
				RoleUpdated();
			}
		}
	}
}
// --------------------------------------------------------------------------
void ACompoundSceneEntity::GotoForParentRole(Time time)
{   
	const int	parentRoleIndex = GetParentRoleIndex();
	ASSERT(parentRoleIndex >= 0);
	ARole*		aRole = &GetSceneEntityState().GetRole(parentRoleIndex);
	ParentRole*	parentRole = dynamic_cast<ParentRole*>(aRole);
    ASSERT(parentRole);
    
	Time		parentEventTime(0);
	const bool	hasEvent = parentRole->GetNearestPreviousOrEqualTime(time, parentEventTime);

	if(!hasEvent || time == kTime0)
	{
		InitParentEventFlags();
   	}

	const ParentState					parentState = parentRole->GetState(time);
    const ParentState::ParentEventID	parentEvent = parentState.GetEvent();

    if(Performing())
    {   
        if(parentEvent == ParentState::kAttachToParent)
		{
            if(IsParentEventFlagSet(parentEventTime))
            {
                return;
            }
            else
            {   
				SetParentEventFlag(parentEventTime, true);

				const int	childHardPointIndex = GetHardPointIndex(*this, parentState.GetChildHardPointName());
				const int	parentHardPointIndex = GetHardPointIndex(*parentState.GetParentEntity(), parentState.GetParentHardPointName());

				if(childHardPointIndex < 0 || parentHardPointIndex < 0)
				{
					// Hardpoints not found!
					return;
				}

				const HardPointHost*	parentHardPointHost = parentState.GetParentEntity()->GetConstMotionStateAccessor()->GetHardPointHost(parentHardPointIndex);

				AttachHardPointToParent(childHardPointIndex, parentHardPointHost, parentState.GetParentHardPointName());
#if 0
//OutputDebugString("Attaching to parent\n");
#endif
            }
		}
		else if(parentEvent == ParentState::kDetachFromParent)
		{
            if(IsParentEventFlagSet(parentEventTime))
            {
                return;
            }
            else
            {   
				SetParentEventFlag(parentEventTime, true);

				const int	childHardPointIndex = GetHardPointIndex(*this, parentState.GetChildHardPointName());
				const int	parentHardPointIndex = GetHardPointIndex(*parentState.GetParentEntity(), parentState.GetParentHardPointName());

				if(childHardPointIndex < 0 || parentHardPointIndex < 0)
				{
					// Hardpoints not found!
					return;
				}

				const HardPointHost*	parentHardPointHost = parentState.GetParentEntity()->GetConstMotionStateAccessor()->GetHardPointHost(parentHardPointIndex);

				DetachHardPointFromParent(childHardPointIndex, parentHardPointHost, parentState.GetParentHardPointName());
#if 0
//OutputDebugString("Detaching from parent\n");
#endif
            }
		}
		else if(parentEvent == ParentState::kInternalParentEvent)
		{
			// Don't do anything!
		}
		else
		{
            ASSERT(0);	// Unhandled case
            return;
		}
    }
    else
    {   // Not performing
        InitParentEventFlags();
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void ACompoundSceneEntity::InitParentEventFlags()
{
	const int	parentRoleIndex = GetParentRoleIndex();
	ASSERT(parentRoleIndex >= 0);
	ARole*		aRole = &GetSceneEntityState().GetRole(parentRoleIndex);
	ParentRole*	parentRole = dynamic_cast<ParentRole*>(aRole);
    ASSERT(parentRole);


	// Run through events that have already been handled, and disconnect
	ParentEventFlags&	flags = GetParentEventFlags();

	ParentEventFlags::const_iterator		begin = flags.begin();
    const ParentEventFlags::const_iterator	end = flags.end();

    while(begin != end)
    {   
		const ParentState		parentState = parentRole->GetState(*begin);
		const int				childHardPointIndex = GetHardPointIndex(*this, parentState.GetChildHardPointName());
		const int				parentHardPointIndex = GetHardPointIndex(*parentState.GetParentEntity(), parentState.GetParentHardPointName());

		if(childHardPointIndex >= 0 || parentHardPointIndex >= 0)
		{
			// Hardpoints found!
			const HardPointHost*	parentHardPointHost = parentState.GetParentEntity()->GetConstMotionStateAccessor()->GetHardPointHost(parentHardPointIndex);

			DetachHardPointFromParent(childHardPointIndex, parentHardPointHost, parentState.GetParentHardPointName());
		}
        
		++begin;
    }

    GetParentEventFlags().clear();
}
// --------------------------------------------------------------------------
CPP_DEFN bool ACompoundSceneEntity::IsParentEventFlagSet(Time startTime) const
{
    ASSERT(GetParentRoleIndex() >= 0 && GetSceneEntityState().GetRoleCount() > GetParentRoleIndex());
    
	const ParentEventFlags&	flags = GetParentEventFlags();

	ParentEventFlags::const_iterator		begin = flags.begin();
    const ParentEventFlags::const_iterator	end = flags.end();

    while(begin != end)
    {   
		if(*begin == startTime)
        {
			return true;
        }
        
		++begin;
    }

    //  The parent event time is not in the list!
    return false;
}
// --------------------------------------------------------------------------
CPP_DEFN void ACompoundSceneEntity::SetParentEventFlag(Time startTime, bool isHandled)
{
    ASSERT(GetParentRoleIndex() >= 0 && GetSceneEntityState().GetRoleCount() > GetParentRoleIndex());
    
	if(isHandled)
	{
		GetParentEventFlags().insert(startTime);
	}
	else
	{
#if 0
		GetParentEventFlags().erase(startTime);	// erase may be buggy
#else
		ParentEventFlags&			flags = GetParentEventFlags();
		ParentEventFlags::iterator	iter = flags.find(startTime);

		if(iter != flags.end())
		{
			flags.erase(iter);
		}
#endif
	}
}
// ---------------------------------------------------------------------------
CPP_DEFN void ACompoundSceneEntity::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	InitParentEventFlags();
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

