// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LinearVelocity_h
#define LinearVelocity_h

#include <iostream>
// --------------------------------------------------------------------------
//	LinearVelocity
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class LinearVelocity
{
	public:
		LinearVelocity Interpolate(const LinearVelocity& nextLinearVelocity, float t) const;

        void Write(std::ostream& oStream) const;
        void Read(std::istream& iStream);

        LinearVelocity operator+(const LinearVelocity& linearVelocity)
        {
        	return LinearVelocity();
        }

        LinearVelocity operator*(float factor)
        {
        	return LinearVelocity();
        }
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::LinearVelocity& velocity)
{
	velocity.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::LinearVelocity& velocity)
{
	velocity.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif