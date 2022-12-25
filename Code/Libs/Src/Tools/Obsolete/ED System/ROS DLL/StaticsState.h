// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef StaticsState_h
#define StaticsState_h
// --------------------------------------------------------------------------
#include "AStaticsState.h"
#include "Position.h"
#include "TimeType.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  StaticsState
// --------------------------------------------------------------------------
class CPP_DECL StaticsState : public virtual AStaticsState
{
    public:
        StaticsState();
        StaticsState(const Position& kPositionR);

		StaticsState& operator=(const AStaticsState& kStaticsStateR);

        virtual Position GetPosition() const;
        virtual void SetPosition(const Position& kPositionR);

        virtual void Write(std::ostream& ostreamR) const;
        virtual void Read(std::istream& istreamR);

    private:
        void WriteSubObject(std::ostream& ostreamR) const;
        void ReadSubObject(std::istream& istreamR);

        typedef AStaticsState BaseClass;

        Position mPosition;
};
// --------------------------------------------------------------------------
static StaticsState LinearInterpolate(const StaticsState& previousState, Time previousTime, const StaticsState& nextState, Time nextTime, Time currentTime)
{
    if(nextTime == previousTime)
    {	
		return previousState;
    }
    else
    {	
		float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

		StaticsState	tState;

        previousState.Interpolate(nextState, t, tState);

		return tState;
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif