// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "AngularVelocity.h"
// --------------------------------------------------------------------------
/**# implementation AngularVelocity:: id(C_0887645612)
*/
// --------------------------------------------------------------------------
enum FieldID
{
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
void AngularVelocity::Write(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
void AngularVelocity::Read(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
}
// --------------------------------------------------------------------------
AngularVelocity AngularVelocity::Interpolate(const AngularVelocity& nextAngularVelocity, float t) const
{
	return AngularVelocity();
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
