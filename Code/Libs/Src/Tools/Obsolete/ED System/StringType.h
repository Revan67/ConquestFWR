// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef StringType_h
#define StringType_h
// --------------------------------------------------------------------------
#include <string>
#include <iostream>

#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef std::string ROSString;
// --------------------------------------------------------------------------
ROSString TrimLeadingWhiteSpace(const ROSString& string);
ROSString TrimTrailingWhiteSpace(const ROSString& string);
void Write(std::ostream& oStream, const ROSString& string);
void Read(std::istream& iStream, ROSString& string);
void ReadLine(std::istream& iStream, ROSString& string);
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::ROSString& string)
{
	ROS::Write(oStream, string);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::ROSString& string)
{
	ROS::Read(iStream, string);

	return iStream;
}
// --------------------------------------------------------------------------
#endif
