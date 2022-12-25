// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef APhysicalState_h
#define APhysicalState_h
// --------------------------------------------------------------------------
#include <iostream>

#include "ROSDLL.h"
// --------------------------------------------------------------------------
//	SceneState
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class CPP_DECL APhysicalState
{
	public:
		virtual void Write(std::ostream& ostreamR) const = 0;
		virtual void Read(std::istream& istreamR) = 0;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::APhysicalState& state)
{
	state.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::APhysicalState& state)
{
	state.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif
