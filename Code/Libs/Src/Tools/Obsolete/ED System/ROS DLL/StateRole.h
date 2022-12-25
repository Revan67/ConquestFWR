// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef StateRole_h
#define StateRole_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "ARole.h"
#include "TimeType.h"
// --------------------------------------------------------------------------
namespace ROS
{
//
class AStateRole: public ARole
{
	public:
		virtual void			GenerateState(Time time) = 0;
};
// --------------------------------------------------------------------------
//  StateRole
// --------------------------------------------------------------------------
template <class TStateType>
class StateRole: public AStateRole
{
	public:
// --------------------------------------------------------------------------
//  Callback
// --------------------------------------------------------------------------
		class UpdateCallback
		{
			public:
				typedef StateRole RoleType;

				virtual void	SetNameStarted(const RoleType& role, const ROSString& name) {}
				virtual void	SetNameFinished(const RoleType& role, const ROSString& name) {}

				virtual void	StateUpdatedStarted(const RoleType& role, const TStateType& state, Time time) {}
				virtual void	StateUpdatedFinished(const RoleType& role, const TStateType& state, Time time) {}
				
				virtual void	StateUpdatedStarted(const RoleType& role, const TStateType& state, unsigned int timePointIndex) {}
				virtual void	StateUpdatedFinished(const RoleType& role, const TStateType& state, unsigned int timePointIndex) {}
				
				virtual void	ChangeTimeStarted(const RoleType& role, Time currentTime, Time newTime) {}
				virtual void	ChangeTimeFinished(const RoleType& role, Time currentTime, Time newTime) {}
				
				virtual void	RemoveStarted(const RoleType& role, Time time) {}
				virtual void	RemoveFinished(const RoleType& role, Time time) {}
				
				virtual void	RemoveStarted(const RoleType& role, unsigned int timePointIndex) {}
				virtual void	RemoveFinished(const RoleType& role, unsigned int timePointIndex) {}
		};
// --------------------------------------------------------------------------
//  TimeStatePair
// --------------------------------------------------------------------------
		class TimeStatePair
		{
			public:
				TimeStatePair()
				: mTime(0)
				{
				}

				TimeStatePair(Time time, const TStateType& state)
				: mTime(time), mState(state)
				{
				}

				void SetTime(Time time)
				{
					mTime = time;
				}
				void SetState(const TStateType& state)
				{
					mState = state;
				}
				void Set(Time time, const TStateType& state)
				{
					SetTime(time);
					SetState(state);
				}

				Time GetTime() const
				{
					return mTime;
				}
				TStateType GetState() const
				{
					return mState;
				}

       			void Write(std::ostream& oStream) const
				{
					WriteSubObject(oStream);
				}
       			void Read(std::istream& iStream)
				{
					ReadSubObject(iStream);
				}

			private :
				enum FieldID
				{
					kTime,
					kState
				};
				
				void WriteSubObject(std::ostream& oStream) const
				{
					OStreamWiz<FieldID>	oWiz(oStream);

					oWiz.Put(kTime, mTime);
					oWiz.Put(kState, mState);
				}
       			void ReadSubObject(std::istream& iStream)
				{
					IStreamWiz<FieldID>	iWiz(iStream);

					iWiz.Get(kTime, mTime);
					iWiz.Get(kState, mState);
				}

				Time        mTime;
				TStateType  mState;
		};

		typedef std::vector<TimeStatePair*> TimeStateVector;

		typedef TimeStateVector::const_iterator TimeStateIterator;
    
    	typedef ROSString  (*GetStateNameCB)(const TStateType&);

		enum TimeStateVectorField
		{
			kSize,
			kFirstTimeState
		};

	   	typedef TStateType (*GenerateStateCB)(TimeStateIterator&, TimeStateIterator&, Time, const TimeStateIterator&, const TimeStateIterator&);

        StateRole(bool entryAtZero, const GenerateStateCB generateStateCB, const GetStateNameCB getStateNameCB, UpdateCallback* updateCallback);
		virtual ~StateRole();

        virtual ROSString 		GetName() const;
        virtual void			SetName(const ROSString& name);

        virtual void 			StateUpdated(const TStateType& state, Time time);
        virtual void 			StateUpdated(const TStateType& state, unsigned int timePointIndex);

        virtual TStateType		GetState(Time time) const;
        virtual TStateType 		GetState(unsigned int timePointIndex) const;

		virtual void			GenerateState(Time time);

        virtual unsigned int	CountTimePoints() const;
        virtual Time            GetTime(unsigned int timePointIndex) const;
		virtual unsigned int	GetIndex(Time time) const;
		virtual ROSString 		GetName(Time time) const;
        virtual bool            HasTime(Time time) const;
		virtual bool			GetNearestPreviousOrEqualTime(Time time, Time& prevOrEqualTime) const;
//		virtual bool			GetNearestPreviousOrEqualTime(Time time, unsigned int& prevOrEqualTimeIndex) const;
        virtual void            ChangeTime(Time currentTime, Time newTime);
        virtual void			Remove(Time time);
        virtual void			Remove(unsigned int timePointIndex);

       	virtual void			Write(std::ostream& oStream) const;
       	virtual void			Read(std::istream& iStream);

	protected:
        /**#: [Cardinalities = "1..n/"]*/
        TimeStateVector   		mTimeStateV;

	private:
		typedef ARole BaseClass;

		enum FieldID
		{
			kName,
			kTimeStateVector,
			kEntryAtZero
		};

		void WriteSubObject(std::ostream& oStream) const;
       	void ReadSubObject(std::istream& iStream);

        ROSString				mName;
        bool					mEntryAtZero;
		const GenerateStateCB	mGenerateStateCB;
		const GetStateNameCB	mGetStateNameCB;
		UpdateCallback*			mUpdateCallback;
};
// --------------------------------------------------------------------------
template <class TStateType>
StateRole<TStateType>::StateRole(bool entryAtZero, const GenerateStateCB generateStateCB, const GetStateNameCB getStateNameCB, UpdateCallback* updateCallback)
: mEntryAtZero(entryAtZero), mGenerateStateCB(generateStateCB), mGetStateNameCB(getStateNameCB), mUpdateCallback(updateCallback)
{
//    mTimeStateV.push_back(new TimeStatePair);
}
// --------------------------------------------------------------------------
template <class TStateType>
StateRole<TStateType>::~StateRole()
{
    TimeStateVector::iterator   begin = mTimeStateV.begin();
    const TimeStateVector::const_iterator   end = mTimeStateV.end();

    while(begin != end)
    {
		delete (*begin);
		++begin;
    }

	if(mUpdateCallback)
	{
		delete mUpdateCallback;
	}
}
// --------------------------------------------------------------------------
template <class TStateType>
ROSString StateRole<TStateType>::GetName() const
{
	return mName;
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::SetName(const ROSString& name)
{
	if(mUpdateCallback)
	{
		mUpdateCallback->SetNameStarted(*this, name);
	}

	mName = name;

	if(mUpdateCallback)
	{
		mUpdateCallback->SetNameFinished(*this, name);
	}
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::StateUpdated(const TStateType& state, Time time)
{
	if(mUpdateCallback)
	{
		mUpdateCallback->StateUpdatedStarted(*this, state, time);
	}

    TimeStateVector::iterator				begin = mTimeStateV.begin();
    const TimeStateVector::const_iterator   end = mTimeStateV.end();

    while(begin != end && (*begin)->GetTime() < time)
    {
		++begin;
    }

    if(begin == end)
    {
		// Either we have no entries or all the entries have times less than the supplied time
        mTimeStateV.push_back(new TimeStatePair(time, state));
    }
    else
    {
		// begin refers to an existing pair
        if((*begin)->GetTime() > time)
        {
			mTimeStateV.insert(begin, new TimeStatePair(time, state));
        }
        else
        {
			// begin refers to a pair that has the same time as the supplied time
            (*begin)->SetState(state);
        }
    }
	
	if(mUpdateCallback)
	{
		mUpdateCallback->StateUpdatedFinished(*this, state, time);
	}
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::StateUpdated(const TStateType& state, unsigned int timePointIndex)
{
	ASSERT(timePointIndex < mTimeStateV.size());

	if(mUpdateCallback)
	{
		mUpdateCallback->StateUpdatedStarted(*this, state, timePointIndex);
	}

    TimeStateVector::iterator				begin = mTimeStateV.begin();
    const TimeStateVector::const_iterator   end = mTimeStateV.end();

	unsigned int idx = timePointIndex;

    while(idx > 0)
    {
		++begin;
		--idx;
    }

	(*begin)->SetState(state);

	if(mUpdateCallback)
	{
		mUpdateCallback->StateUpdatedFinished(*this, state, timePointIndex);
	}
}
// --------------------------------------------------------------------------
template <class TStateType>
TStateType StateRole<TStateType>::GetState(Time time) const
{
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
		return (mTimeStateV.back())->GetState();
    }
    else
    {
		// Not at the last entry
        return (*begin)->GetState();
    }
}
// --------------------------------------------------------------------------
template <class TStateType>
TStateType StateRole<TStateType>::GetState(unsigned int timePointIndex) const
{
	ASSERT(timePointIndex < mTimeStateV.size());

	if(timePointIndex < mTimeStateV.size())
    {
#if 0
		return mTimeStateV[timePointIndex].GetState();
#else
		TimeStateVector::const_iterator	begin = mTimeStateV.begin() + timePointIndex;

		return (*begin)->GetState();
#endif
    }
    else
    {
		return TStateType();
    }
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::GenerateState(Time time)
{
	ASSERT(mGenerateStateCB);

    if(CountTimePoints() == 0)
	{
		StateUpdated(TStateType(), time);
		return;
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

       	StateUpdated(mGenerateStateCB(last, last, time, initial, end), time);
    }
    else
    {
		if(begin == mTimeStateV.begin())
    	{
			// At the first entry. 
	       	StateUpdated(mGenerateStateCB(begin, begin, time, initial, end), time);
        }
        else
        {
			// Not at the first or last entries
            if((*begin)->GetTime() == time)
            {
				StateUpdated(mGenerateStateCB(begin, begin, time, initial, end), time);
            }
            else
            {
				TimeStateVector::const_iterator	previous = begin - 1;

				StateUpdated(mGenerateStateCB(previous, begin, time, initial, end), time);
            }
        }
    }
}
// --------------------------------------------------------------------------
template <class TStateType>
unsigned int StateRole<TStateType>::CountTimePoints() const
{
	return mTimeStateV.size();
}
// --------------------------------------------------------------------------
template <class TStateType>
Time StateRole<TStateType>::GetTime(unsigned int timePointIndex) const
{
	ASSERT(timePointIndex < mTimeStateV.size());

	if(timePointIndex < mTimeStateV.size())
    {
#if 0
		return mTimeStateV[timePointIndex].GetTime();
#else
		TimeStateVector::const_iterator	begin = mTimeStateV.begin();

		while(timePointIndex > 0)
		{
			++begin;
			--timePointIndex;
		}

		return (*begin)->GetTime();
#endif
    }
    else
    {
		return Time(0);
    }
}
// --------------------------------------------------------------------------
template <class TStateType>
unsigned int StateRole<TStateType>::GetIndex(Time time) const
{
	TimeStateVector::const_iterator   		begin = mTimeStateV.begin();
    const TimeStateVector::const_iterator   end = mTimeStateV.end();
	unsigned int							index = 0;

    while(begin != end)
    {
		if((*begin)->GetTime() == time)
        {
			return index;
        }

		++index;
        ++begin;
    }

	ASSERT(0 && "The time was not found");
	return 0;
}
// --------------------------------------------------------------------------
template <class TStateType>
bool StateRole<TStateType>::HasTime(Time time) const
{
	TimeStateVector::const_iterator   		begin = mTimeStateV.begin();
    const TimeStateVector::const_iterator   end = mTimeStateV.end();

    while(begin != end)
    {
		if((*begin)->GetTime() == time)
        {
			return true;
        }

        ++begin;
    }

	return false;
}
// --------------------------------------------------------------------------
template <class TStateType>
bool StateRole<TStateType>::GetNearestPreviousOrEqualTime(Time time, Time& prevOrEqualTime) const
{
    TimeStateVector::const_iterator			end = mTimeStateV.end();
	const TimeStateVector::const_iterator   begin = mTimeStateV.begin();

    while(end != begin)
    {
		--end;

		const Time	currentTime = (*end)->GetTime();

		if(currentTime <= time)
        {
			prevOrEqualTime = currentTime;

			return true;
        }
    }

	return false;
}
#if 0
// --------------------------------------------------------------------------
template <class TStateType>
bool StateRole<TStateType>::GetNearestPreviousOrEqualTime(Time time, unsigned int& prevOrEqualTimeIndex) const
{
	unsigned int	index = CountTimePoints();

	if(index == 0)
	{
		return false;
	}

	TimeStateVector::const_iterator			end = mTimeStateV.end();
	const TimeStateVector::const_iterator   begin = mTimeStateV.begin();

    while(end != begin)
    {
		--end;
		--index;

		const Time	currentTime = (*end)->GetTime();

		if(currentTime <= time)
        {
			prevOrEqualTimeIndex = index;

			return true;
        }
    }

	return false;
}
#endif
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::ChangeTime(Time currentTime, Time newTime)
{
    ASSERT(mTimeStateV.size() > 0);

	if(mUpdateCallback)
	{
		mUpdateCallback->ChangeTimeStarted(*this, currentTime, newTime);
	}

	if(currentTime != newTime)
    {
		TimeStateVector::iterator   			begin = mTimeStateV.begin();
        const TimeStateVector::const_iterator   end = mTimeStateV.end();

        while(begin != end)
        {
			if((*begin)->GetTime() == currentTime)
        	{
				TimeStateVector::value_type	value =	*begin;

      			mTimeStateV.erase(begin);

                StateUpdated(value->GetState(), newTime);

				delete value;

				if(mUpdateCallback)
				{
					mUpdateCallback->ChangeTimeFinished(*this, currentTime, newTime);
				}

				return;
            }

        	++begin;
        }

		ASSERT(0);	// Entry not found!
    }

	if(mUpdateCallback)
	{
		mUpdateCallback->ChangeTimeFinished(*this, currentTime, newTime);
	}
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::Remove(Time time)
{
	ASSERT(mTimeStateV.size() > 0);

	if(mUpdateCallback)
	{
		mUpdateCallback->RemoveStarted(*this, time);
	}

	TimeStateVector::iterator   			begin = mTimeStateV.begin();
    const TimeStateVector::const_iterator   end = mTimeStateV.end();

    while(begin != end)
    {
		TimeStateVector::value_type	value =	*begin;

		if(value->GetTime() == time)
        {
			mTimeStateV.erase(begin);
			
			delete value;

			if(mUpdateCallback)
			{
				mUpdateCallback->RemoveFinished(*this, time);
			}

            return;
        }

        ++begin;
    }

	if(mUpdateCallback)
	{
		mUpdateCallback->RemoveFinished(*this, time);
	}

	ASSERT(0);	// Entry not found!
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::Remove(unsigned int timePointIndex)
{
				
	if(mUpdateCallback)
	{
		mUpdateCallback->RemoveStarted(*this, timePointIndex);
	}

	ASSERT(mTimeStateV.size() > timePointIndex);

	TimeStateVector::iterator	begin = mTimeStateV.begin() + timePointIndex;

	TimeStateVector::value_type	value =	*begin;

	mTimeStateV.erase(begin);
	
	delete value;

	if(mUpdateCallback)
	{
		mUpdateCallback->RemoveFinished(*this, timePointIndex);
	}
}
// --------------------------------------------------------------------------
template <class TStateType>
ROSString StateRole<TStateType>::GetName(Time time) const
{
    ASSERT(mTimeStateV.size() > 0);

	TimeStateVector::const_iterator   		begin = mTimeStateV.begin();
    const TimeStateVector::const_iterator   end = mTimeStateV.end();

    while(begin != end)
    {
		if((*begin)->GetTime() == time)
        {
			return mGetStateNameCB((*begin)->GetState());
        }

        ++begin;
    }

    ASSERT(0);	// Entry not found!
    return "";
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kName, mName);
	oWiz.Put(kTimeStateVector, mTimeStateV);
    oWiz.Put(kEntryAtZero, mEntryAtZero);
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
template <class TStateType>
void StateRole<TStateType>::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kName, mName);
	iWiz.Get(kTimeStateVector, mTimeStateV);
    iWiz.Get(kEntryAtZero, mEntryAtZero);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
template <class TStateType>
std::ostream& operator<<(std::ostream& oStream, const ROS::StateRole<TStateType>::TimeStatePair& pair)
{
	pair.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
template <class TStateType>
std::istream& operator>>(std::istream& iStream, ROS::StateRole<TStateType>::TimeStatePair& pair)
{
	pair.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
template <class TStateType>
std::ostream& operator<<(std::ostream& oStream, const ROS::StateRole<TStateType>::TimeStateVector& vector)
{
	OStreamWiz<ROS::StateRole<TStateType>::TimeStateVectorField>	oWiz(oStream);

	int size = vector.size();
	
	oWiz.Put(ROS::StateRole<TStateType>::kSize, size);

	ROS::StateRole<TStateType>::TimeStateVector::const_iterator			begin = vector.begin();
	const ROS::StateRole<TStateType>::TimeStateVector::const_iterator	end = vector.end();
	unsigned int														idx = 0;

    while(begin < end)
    {
		oWiz.Put(static_cast<ROS::StateRole<TStateType>::TimeStateVectorField>(ROS::StateRole<TStateType>::kFirstTimeState + idx), **begin);
    	
		++begin;
		++idx;
    }

	return oStream;
}
// --------------------------------------------------------------------------
template <class TStateType>
std::istream& operator>>(std::istream& iStream, ROS::StateRole<TStateType>::TimeStateVector& vector)
{
	IStreamWiz<ROS::StateRole<TStateType>::TimeStateVectorField>	iWiz(iStream);

  	int size;
	
	iWiz.Get(ROS::StateRole<TStateType>::kSize, size);

    ASSERT(size >= 0);

    // Remove existing entries
	ROS::StateRole<TStateType>::TimeStateVector::iterator				cleanupBegin = vector.begin();
	const ROS::StateRole<TStateType>::TimeStateVector::const_iterator	cleanupEnd = vector.end();

    while(cleanupBegin < cleanupEnd)
    {
		delete (*cleanupBegin);
    	++cleanupBegin;
    }

    vector.resize(size);

	ROS::StateRole<TStateType>::TimeStateVector::iterator				insertBegin = vector.begin();
	const ROS::StateRole<TStateType>::TimeStateVector::const_iterator	insertEnd = vector.end();
	unsigned int														idx = 0;

    while(insertBegin < insertEnd)
    {
		(*insertBegin) = new ROS::StateRole<TStateType>::TimeStatePair;
		
		iWiz.Get(static_cast<ROS::StateRole<TStateType>::TimeStateVectorField>(ROS::StateRole<TStateType>::kFirstTimeState + idx), **insertBegin);

		++insertBegin;
		++idx;
    }

	return iStream;
}
// --------------------------------------------------------------------------
#endif
