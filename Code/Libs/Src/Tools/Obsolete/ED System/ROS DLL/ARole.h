// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ARole_h
#define ARole_h

#include "StringType.h"
#include "TimeType.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
class AStateVariable;
// --------------------------------------------------------------------------
//  ARole
// --------------------------------------------------------------------------
class CPP_DECL ARole
{
    public:
        virtual 				~ARole() = 0;

        virtual ROSString 		GetName() const = 0;
        virtual void			SetName(const ROSString& name) = 0;

        virtual unsigned int	CountTimePoints() const = 0;
        virtual Time            GetTime(unsigned int timePointIndex) const = 0;
		virtual unsigned int	GetIndex(Time time) const = 0;
		virtual ROSString 		GetName(Time time) const = 0;
        virtual bool            HasTime(Time time) const = 0;
		virtual bool			GetNearestPreviousOrEqualTime(Time time, Time& prevOrEqualTime) const = 0;
//		virtual bool			GetNearestPreviousOrEqualTime(Time time, unsigned int& prevOrEqualTimeIndex) const = 0;
        virtual void            ChangeTime(Time currentTime, Time newTime) = 0;
        virtual void			Remove(Time time) = 0;
        virtual void			Remove(unsigned int timePointIndex) = 0;

        virtual void			Write(std::ostream& oStream) const = 0;
        virtual void			Read(std::istream& iStream) = 0;
};
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ARole& role)
{
	role.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ARole& role)
{
	role.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
