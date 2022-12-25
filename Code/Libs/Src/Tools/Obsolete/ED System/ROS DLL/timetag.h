// Author: Shaival Varma
//---------------------------------------------------------------------------
#ifndef TimeTagH
#define TimeTagH
//---------------------------------------------------------------------------
#include <iostream>
#ifdef _MSC_VER
#include <vector>
#else
#include <Include\vector>
#endif

#include "TimeType.h"
#include "OStreamWiz.h"
#include "IStreamWiz.h"
//---------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
class TimeTag
{
    public:
        TimeTag()
        : mTime(0), mTag(-1)
        {
        }

        TimeTag(Time time, int tag)
        : mTime(time), mTag(tag)
        {
        }

        Time GetTime() const
        {
            return mTime;
        }

        int GetTag() const
        {
            return mTag;
        }

        void SetTime(Time time)
        {
            mTime = time;
        }

        void SetTag(int tag)
        {
            mTag = tag;
        }

        void Set(Time time, int tag)
        {
        	SetTime(time);
            SetTag(tag);
        }

        void Write(std::ostream& oStream) const
        {
			OStreamWiz<FieldID>	oWiz(oStream);

			oWiz.Put(kTime, mTime);
			oWiz.Put(kTag, mTag);
        }

        void Read(std::istream& iStream)
        {
			IStreamWiz<FieldID>	iWiz(iStream);

			iWiz.Get(kTime, mTime);
			iWiz.Get(kTag, mTag);
        }

    private:
		enum FieldID
		{
			kTime,
			kTag
		};

        Time	mTime;
        int     mTag;
};
//---------------------------------------------------------------------------
typedef std::vector<TimeTag>	TimeTagList;
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::TimeTag& timeTag)
{
	timeTag.Write(oStream);

	return oStream;
}
//---------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::TimeTag& timeTag)
{
 	timeTag.Read(iStream);

	return iStream;
}
//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& oStream, const ROS::TimeTagList& list);
std::istream& operator>>(std::istream& iStream, ROS::TimeTagList& list);
//---------------------------------------------------------------------------
#endif
