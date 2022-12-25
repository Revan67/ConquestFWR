// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef _h_MarkerData
#define _h_MarkerData
// --------------------------------------------------------------------------
#include "TimeType.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ASceneEntity;
}
// --------------------------------------------------------------------------
class MarkerData
{	public:
		MarkerData(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber, ROS::Time time);

		ROS::ASceneEntity& GetSceneEntity() const;
		unsigned int GetTrackNumber() const;
		ROS::Time GetTime() const;

		void SetTime(ROS::Time time);

	private:
		ROS::ASceneEntity&	mSceneEntity;
		unsigned int		mTrackNumber;
		ROS::Time			mTime;		
};
// --------------------------------------------------------------------------
inline MarkerData::MarkerData(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber, ROS::Time time)
:mSceneEntity(sceneEntity), mTrackNumber(trackNumber), mTime(time)
{
}
// --------------------------------------------------------------------------
inline ROS::ASceneEntity& MarkerData::GetSceneEntity() const
{
	return mSceneEntity;
}
// --------------------------------------------------------------------------
inline unsigned int MarkerData::GetTrackNumber() const
{
	return mTrackNumber;
}
// --------------------------------------------------------------------------
inline ROS::Time MarkerData::GetTime() const
{
	return mTime;
}
// --------------------------------------------------------------------------
inline void MarkerData::SetTime(ROS::Time time)
{
	mTime = time;
}
// --------------------------------------------------------------------------
#endif