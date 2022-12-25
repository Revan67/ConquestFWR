// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Torque_h
#define Torque_h
// --------------------------------------------------------------------------
#include <iostream>
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	Torque
// --------------------------------------------------------------------------
class Torque
{
	public:
		Torque Interpolate(const Torque& nextTorque, float t) const;

        void Write(std::ostream& ostreamR) const;
        void Read(std::istream& istreamR);

        Torque operator+(const Torque& torque)
        {
        	return Torque();
        }

        Torque operator*(float factor)
        {
        	return Torque();
        }
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::Torque& torque)
{
	torque.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::Torque& torque)
{
	torque.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif