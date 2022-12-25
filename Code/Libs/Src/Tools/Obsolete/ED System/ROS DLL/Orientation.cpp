// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "Orientation.h"
#include "Char.h"
// --------------------------------------------------------------------------
/**# implementation Orientation:: id(C_0886790584)
*/
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
Orientation Interpolate(const Orientation& previousOrientation, const Orientation& nextOrientation, float t)
{
    Orientation orientAtT;

	DAInterpolateOrientation(previousOrientation, nextOrientation, t, orientAtT);

    return orientAtT;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

