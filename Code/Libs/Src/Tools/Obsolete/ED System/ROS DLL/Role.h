// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Role_h
#define Role_h
// --------------------------------------------------------------------------
#include <vector>

#include "StringType.h"
#include "Links.h"
#include "StateRole.h"
#include "TimeType.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  Role
// --------------------------------------------------------------------------
template <class TStateType>
class Role: public StateRole<TStateType>
{
	public:
		typedef GenerateStateCB InterpolateCB;

        Role(bool entryAtZero, const InterpolateCB interpolateCB, const GenerateStateCB generateStateCB, const GetStateNameCB getStateNameCB, UpdateCallback* updateCallback);

        virtual TStateType 		GetState(Time time) const;
        virtual TStateType 		GetState(unsigned int timePointIndex) const;

	private:
		typedef StateRole<TStateType> BaseClass;

        const InterpolateCB		mInterpolateCB;
};
// --------------------------------------------------------------------------
template <class TStateType>
Role<TStateType>::Role(bool entryAtZero, const InterpolateCB interpolateCB, const GenerateStateCB generateStateCB, const GetStateNameCB getStateNameCB, UpdateCallback* updateCallback)
: BaseClass(entryAtZero, generateStateCB, getStateNameCB, updateCallback), mInterpolateCB(interpolateCB)
{
//    mTimeStateV.push_back(new TimeStatePair);
}
// --------------------------------------------------------------------------
template <class TStateType>
TStateType Role<TStateType>::GetState(Time time) const
{
	ASSERT(mInterpolateCB);

    if(CountTimePoints() == 0)
	{
		return TStateType();
	}

    TimeStateVector::const_iterator			begin = mTimeStateV.begin();
	const TimeStateVector::const_iterator	initial = begin;
    const TimeStateVector::const_iterator   end = mTimeStateV.end();

    while(begin != end && (*begin)->GetTime() < time)
    {
		++begin;
    }

    if(begin == end)
    {
		// At the last entry.
		TimeStateVector::const_iterator			last = mTimeStateV.end() - 1;

       	return mInterpolateCB(last, last, time, initial, end);
    }
    else
    {
		if(begin == mTimeStateV.begin())
    	{
			// At the first entry. 
	       	return mInterpolateCB(begin, begin, time, initial, end);
        }
        else
        {
			// Not at the first or last entries
            if((*begin)->GetTime() == time)
            {
				return mInterpolateCB(begin, begin, time, initial, end);
            }
            else
            {
				TimeStateVector::const_iterator	previous = begin - 1;

				return mInterpolateCB(previous, begin, time, initial, end);
            }
        }
    }
}
// --------------------------------------------------------------------------
template <class TStateType>
TStateType Role<TStateType>::GetState(unsigned int timePointIndex) const
{
	return BaseClass::GetState(timePointIndex);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
