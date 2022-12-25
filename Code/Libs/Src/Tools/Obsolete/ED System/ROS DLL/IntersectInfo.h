// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef IntersectInfo_h
// --------------------------------------------------------------------------
#include "Vector.h"
// --------------------------------------------------------------------------
struct BaseCamera;
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class IntersectInfo
{
	public:
		IntersectInfo(const BaseCamera* camera, int winX, int winY, const Vector& rayStart, const Vector& rayDirection)
		: mCamera(camera), mWinX(winX), mWinY(winY), mRayStart(rayStart), mRayDirection(rayDirection)
		{
		}

		const BaseCamera* GetCamera() const
		{	
			return mCamera;
		}
		
		int GetWindowX() const
		{	
			return mWinX;
		}

		int GetWindowY() const
		{
			return mWinY;
		}

		Vector GetRayStart() const
		{
			return mRayStart;
		}

		Vector GetRayDirection() const
		{
			return mRayDirection;		
		}

	private:
		const BaseCamera*	mCamera;
		int					mWinX;
		int					mWinY;
		Vector				mRayStart;
		Vector				mRayDirection;		
};
// --------------------------------------------------------------------------
}	// namespace ROS
// --------------------------------------------------------------------------
#endif