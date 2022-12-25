// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Time_h
#define Time_h

#include <iostream>
#include "ROSDLL.h"
// --------------------------------------------------------------------------
//	Time
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class CPP_DECL Time
{
	public:
        explicit Time(float time);

        void SetTime(float time);
        float GetTime() const;

        Time  operator+(Time time) const;
        Time& operator+=(Time time);
        Time  operator-(Time time) const;
        Time& operator-=(Time time);

        bool operator<(Time time) const;
        bool operator<=(Time time) const;
        bool operator>(Time time) const;
        bool operator>=(Time time) const;
        bool operator==(Time time) const;
        bool operator!=(Time time) const;

		void Write(std::ostream& ostreamR) const;
		void Read(std::istream& istreamR);

    private:
        float mTime;
};
// --------------------------------------------------------------------------
const Time	kTime0(0);
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const Time& time)
{
	time.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, Time& time)
{
	time.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
inline Time operator*(float factor, Time time)
{
	Time	scaledTime(factor * time.GetTime());

 	return	scaledTime;
}
// --------------------------------------------------------------------------
inline Time::Time(float time)
: mTime(time)
{
}
// --------------------------------------------------------------------------
inline void Time::SetTime(float time)
{
    mTime = time;
}
// --------------------------------------------------------------------------
inline float Time::GetTime() const
{
    return mTime;
}
// --------------------------------------------------------------------------
inline Time Time::operator+(Time time) const
{
	return Time(mTime + time.mTime);
}
// --------------------------------------------------------------------------
inline Time& Time::operator+=(Time time)
{
	mTime += time.mTime;

    return *this;
}
// --------------------------------------------------------------------------
inline Time Time::operator-(Time time) const
{
	return Time(mTime - time.mTime);
}
// --------------------------------------------------------------------------
inline Time& Time::operator-=(Time time)
{
	mTime -= time.mTime;

    return *this;
}
// --------------------------------------------------------------------------
inline bool Time::operator<(Time time) const
{
	return mTime < time.mTime;
}
// --------------------------------------------------------------------------
inline bool Time::operator<=(Time time) const
{
	return mTime <= time.mTime;
}
// --------------------------------------------------------------------------
inline bool Time::operator>(Time time) const
{
	return mTime > time.mTime;
}
// --------------------------------------------------------------------------
inline bool Time::operator>=(Time time) const
{
	return mTime >= time.mTime;
}
// --------------------------------------------------------------------------
inline bool Time::operator==(Time time) const
{
	return mTime == time.mTime;
}
// --------------------------------------------------------------------------
inline bool Time::operator!=(Time time) const
{
	return mTime != time.mTime;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif