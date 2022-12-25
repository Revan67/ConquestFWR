// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AngularVelocity_h
#define AngularVelocity_h

#include <iostream>
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	AngularVelocity
// --------------------------------------------------------------------------
class AngularVelocity
{
	public:
		AngularVelocity Interpolate(const AngularVelocity& nextAngularVelocity, float t) const;

        void Write(std::ostream& oStream) const;
        void Read(std::istream& iStream);

        AngularVelocity operator+(const AngularVelocity& angularVelocity)
        {
        	return AngularVelocity();
        }

        AngularVelocity operator*(float factor)
        {
        	return AngularVelocity();
        }
// --------------------------------------------------------------------------
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::AngularVelocity& velocity)
{
	velocity.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::AngularVelocity& velocity)
{
	velocity.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif