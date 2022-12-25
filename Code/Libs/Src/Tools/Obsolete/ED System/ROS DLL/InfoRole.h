// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef InfoRole_h
#define InfoRole_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "StateRole.h"
#include "TimeType.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  InfoRole
// --------------------------------------------------------------------------
template <class TStateType, typename TInfoType>
class InfoRole: public StateRole<TStateType>
{
	public:
    	typedef TStateType (*InterpolateCB)(const InfoRole& role, TimeStateIterator&, TimeStateIterator&, Time, const TimeStateIterator&, const TimeStateIterator&, TInfoType*);
		typedef InterpolateCB	GenerateStateCB;

        InfoRole(bool entryAtZero, const InterpolateCB interpolateCB, const GenerateStateCB generateStateCB, const GetStateNameCB getStateNameCB, UpdateCallback* updateCallback, TInfoType* info);

        virtual TStateType 		GetState(Time time) const;
        virtual TStateType 		GetState(unsigned int timePointIndex) const;

		virtual void			GenerateState(Time time);

		TInfoType*				GetInfo() const;
		void					SetInfo(TInfoType* info);

	private:
		typedef StateRole<TStateType> BaseClass;

        const InterpolateCB		mInterpolateCB;
		const GenerateStateCB	mGenerateStateCB;
		TInfoType*				mInfo;
};
// --------------------------------------------------------------------------
template <class TStateType, typename TInfoType>
InfoRole<TStateType, TInfoType>::InfoRole(bool entryAtZero, const InterpolateCB interpolateCB, const GenerateStateCB generateStateCB, const GetStateNameCB getStateNameCB, UpdateCallback* updateCallback, TInfoType* info)
: BaseClass(entryAtZero, NULL, getStateNameCB, updateCallback), mInterpolateCB(interpolateCB), mGenerateStateCB(generateStateCB), mInfo(info)
{
//    mTimeStateV.push_back(new TimeStatePair);
}
// --------------------------------------------------------------------------
template <class TStateType, typename TInfoType>
TStateType InfoRole<TStateType, TInfoType>::GetState(Time time) const
{
	ASSERT(mInterpolateCB);

    if(mTimeStateV.empty())
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

       	return mInterpolateCB(*this, last, last, time, initial, end, mInfo);
    }
    else
    {
		if(begin == mTimeStateV.begin())
    	{
			// At the first entry. 
	       	return mInterpolateCB(*this, begin, begin, time, initial, end, mInfo);
        }
        else
        {
			// Not at the first or last entries
            if((*begin)->GetTime() == time)
            {
				return mInterpolateCB(*this, begin, begin, time, initial, end, mInfo);
            }
            else
            {
				TimeStateVector::const_iterator	previous = begin - 1;

				return mInterpolateCB(*this, previous, begin, time, initial, end, mInfo);
            }
        }
    }
}
// --------------------------------------------------------------------------
template <class TStateType, typename TInfoType>
TStateType InfoRole<TStateType, TInfoType>::GetState(unsigned int timePointIndex) const
{
	return BaseClass::GetState(timePointIndex);
}
// --------------------------------------------------------------------------
template <class TStateType, typename TInfoType>
void InfoRole<TStateType, TInfoType>::GenerateState(Time time)
{
	ASSERT(mInterpolateCB);

    if(mTimeStateV.empty())
	{
		StateUpdated(TStateType(), time);
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

       	StateUpdated(mGenerateStateCB(*this, last, last, time, initial, end, mInfo), time);
    }
    else
    {
		if(begin == mTimeStateV.begin())
    	{
			// At the first entry. 
	       	StateUpdated(mGenerateStateCB(*this, begin, begin, time, initial, end, mInfo), time);
        }
        else
        {
			// Not at the first or last entries
            if((*begin)->GetTime() == time)
            {
				StateUpdated(mGenerateStateCB(*this, begin, begin, time, initial, end, mInfo), time);
            }
            else
            {
				TimeStateVector::const_iterator	previous = begin - 1;

				StateUpdated(mGenerateStateCB(*this, previous, begin, time, initial, end, mInfo), time);
            }
        }
    }
}
// --------------------------------------------------------------------------
template <class TStateType, typename TInfoType>
TInfoType* InfoRole<TStateType, TInfoType>::GetInfo() const
{
	return mInfo;
}
// --------------------------------------------------------------------------
template <class TStateType, typename TInfoType>
void InfoRole<TStateType, TInfoType>::SetInfo(TInfoType* info)
{
	mInfo = info
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
