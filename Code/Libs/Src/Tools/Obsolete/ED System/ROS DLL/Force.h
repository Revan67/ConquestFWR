// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Force_h
#define Force_h

#include <iostream>
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	Force
// --------------------------------------------------------------------------

class Force
{
	public:
		Force Interpolate(const Force& nextForce, float t) const;

        void Write(std::ostream& ostreamR) const;
        void Read(std::istream& istreamR);

        Force operator+(const Force& force)
        {
        	return Force();
        }

        Force operator*(float factor)
        {
        	return Force();
        }
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::Force& force)
{
	force.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::Force& force)
{
	force.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif