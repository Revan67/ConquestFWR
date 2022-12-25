// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef HardPoint_h
#define HardPoint_h
// --------------------------------------------------------------------------
#include "StringType.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ACompoundSceneEntity;
class HardPointHost;
// --------------------------------------------------------------------------
class HardPoint
{
	public:
		explicit HardPoint(ACompoundSceneEntity* aCompoundSE = NULL, const HardPointHost* hardPointHost = NULL, const ROSString& hardPointName = "")
		: mACompoundSE(aCompoundSE), mHardPointHost(hardPointHost), mHardPointName(hardPointName)
		{
		}

		ACompoundSceneEntity* GetACompoundSceneEntity() const
		{
			return mACompoundSE;
		}

		const HardPointHost* GetHardPointHost() const
		{
			return mHardPointHost;
		}

		ROSString GetHardPointName() const
		{
			return mHardPointName;
		}

	private:
		ACompoundSceneEntity*	mACompoundSE;
		const HardPointHost*	mHardPointHost;
		ROSString				mHardPointName;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif