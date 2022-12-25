// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Location_h
#define Location_h
// --------------------------------------------------------------------------
#include <istream>
#include <ostream>

#if 1
#include "Vector.h"
#endif
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	Location
// --------------------------------------------------------------------------
#if 0
class CPP_DECL Location
{
	public:
    	Location(float x = 0, float y = 0, float z = 0)
        : mX(x), mY(y), mZ(z)
        {
        }

        void SetX(float x) { mX = x; };
        void SetY(float y) { mY = y; };
        void SetZ(float z) { mZ = z; };

        float GetX() { return mX; };
        float GetY() { return mY; };
        float GetZ() { return mZ; };

		void Read(std::istream& iStream);
		void Write(std::ostream& oStream) const;

    private:
    	float mX;
        float mY;
        float mZ;
};
#else
class CPP_DECL Location
{
	public:
    	explicit Location(float x = 0, float y = 0, float z = 0)
        : mVector(x, y, z)
        {
        }

    	explicit Location(const Vector& location)
        : mVector(location)
        {
        }

    	virtual ~Location()
        {
        }

		Location Interpolate(const Location& nextLocation, float t) const;

        Vector GetVector() const
        {
        	return mVector;
        }

        void SetX(float x) { mVector.x = x; };
        void SetY(float y) { mVector.y = y; };
        void SetZ(float z) { mVector.z = z; };

        float GetX() const { return mVector.x; };
        float GetY() const { return mVector.y; };
        float GetZ() const{ return mVector.z; };

        Location& operator+=(const Location& location) { mVector += location.mVector; return *this; };
        Location& operator-=(const Location& location) { mVector -= location.mVector; return *this; };

        Location operator+(const Location& location) const { return Location(mVector + location.mVector); };
        Location operator-(const Location& location) const { return Location(mVector - location.mVector); };

        virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

    private :
        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

    	Vector mVector;
};
#endif
// --------------------------------------------------------------------------
inline Location Location::Interpolate(const Location& nextLocation, float t) const
{
	float diff = 1 - t;

	return Location((GetX() * diff) + nextLocation.GetX() * t,
    				(GetY() * diff) + nextLocation.GetY() * t,
                    (GetZ() * diff) + nextLocation.GetZ() * t);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::Location& location)
{
	location.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::Location& location)
{
	location.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif
