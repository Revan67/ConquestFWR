// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef _h_TrigUtil
#define _h_TrigUtil
// --------------------------------------------------------------------------
inline float DegreeToRadian(float angle)
{
	const float degreeToRadian = (float)(3.1416 / 180.0);

	return angle * degreeToRadian;
}
// --------------------------------------------------------------------------
#endif
