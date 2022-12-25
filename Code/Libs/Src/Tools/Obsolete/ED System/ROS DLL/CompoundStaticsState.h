// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef CompoundStaticsState_h
#define CompoundStaticsState_h

#include "ACompoundSceneEntityState.h"
#include "StaticsState.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
//  CompoundStaticsState
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class CPP_DECL CompoundStaticsState: public virtual ACompoundSceneEntityState, public StaticsState
{
    private:
    	typedef ACompoundSceneEntityState ACompoundSceneEntityStateBaseClass;
		typedef StaticsState StaticsStateBaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
