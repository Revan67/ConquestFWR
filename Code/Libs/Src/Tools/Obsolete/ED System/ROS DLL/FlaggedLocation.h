// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef FlaggedLocation_h
#define FlaggedLocation_h

#include "Location.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// FlaggedLocation
// --------------------------------------------------------------------------
class CPP_DECL FlaggedLocation: public Location
{
	public:
		enum InterpolationType
		{
			kLinearFixed,
			kSplineFixed,
			kLinearBlend,
			kSplineBlend
		};

		FlaggedLocation()
		:mInterpolation(kLinearFixed)
		{
		}

		FlaggedLocation(const FlaggedLocation& location)
		:BaseClass(location), mInterpolation(location.GetInterpolationType())
		{
		}

		FlaggedLocation(const Location& location, InterpolationType type)
		:BaseClass(location), mInterpolation(type)
		{
		}

		void SetInterpolationType(InterpolationType type)
		{
			mInterpolation = type;
		}

		InterpolationType GetInterpolationType() const
		{
			return mInterpolation;
		}

        virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& ostreamR);

	private:
		typedef Location BaseClass;

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

		InterpolationType	mInterpolation;
};
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::FlaggedLocation& location)
{
	location.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::FlaggedLocation& location)
{
	location.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif