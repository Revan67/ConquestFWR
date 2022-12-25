// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ACompoundSceneEntityState_h
#define ACompoundSceneEntityState_h

#include "AStaticsState.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
//  ACompoundSceneEntityState
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class CPP_DECL ACompoundSceneEntityState: public virtual AStaticsState
{
    public:
        virtual ~ACompoundSceneEntityState() = 0;

    private:
    	typedef AStaticsState BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
