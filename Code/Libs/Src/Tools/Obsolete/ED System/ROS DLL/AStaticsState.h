// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AStaticsState_h
#define AStaticsState_h

#include "APhysicalState.h"
#include "Position.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
//  AStaticsState
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class CPP_DECL AStaticsState: public virtual APhysicalState
{
    public:
        virtual ~AStaticsState() = 0;

        virtual Position GetPosition() const = 0;
        virtual void SetPosition(const Position& position) = 0;

        virtual void SetStaticsState(const AStaticsState& staticsState);

		virtual void Interpolate(const AStaticsState& nextState, float t, AStaticsState& tState) const;

    private:
    	typedef APhysicalState BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
