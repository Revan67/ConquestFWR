// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ParentRole.h"
#include "ACompoundSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ROSString GetStateName(const ParentState& parentState)
{
	ParentState::ParentEventID	parentEvent = parentState.GetEvent();
    ROSString                   stateName("Parent: ");

    switch(parentEvent)
    {
		case ParentState::kAttachToParent:
    		stateName += parentState.GetParentEntity()->GetConstSceneEntityStateAccessor()->GetName();
    		break;
    	case ParentState::kDetachFromParent:
    		stateName += "<None>";
    		break;
        default:
        	ASSERT(0);	// Unhandled case
            return "";
    }

    return  stateName;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
