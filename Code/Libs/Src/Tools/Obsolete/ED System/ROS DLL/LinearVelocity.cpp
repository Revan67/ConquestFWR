// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LinearVelocity.h"
// --------------------------------------------------------------------------
/**# implementation LinearVelocity:: id(C_0887645597)
*/
// --------------------------------------------------------------------------
enum FieldID
{
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
void LinearVelocity::Write(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
void LinearVelocity::Read(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
}
// --------------------------------------------------------------------------
LinearVelocity LinearVelocity::Interpolate(const LinearVelocity& nextLinearVelocity, float t) const
{
	return LinearVelocity();
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
