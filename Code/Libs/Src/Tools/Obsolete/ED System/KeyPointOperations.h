// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef KeyPointOperations_h
#define KeyPointOperations_h
// --------------------------------------------------------------------------
#include "AOperation.h"
#include "TimeType.h"
#include "Controller.h"
#include "ASceneEntity.h"
// --------------------------------------------------------------------------
class KeyPointPositionChange: public ROS::AOperation
{
	public:
		KeyPointPositionChange(ROS::ASceneEntity& entity, unsigned int trackNumber, ROS::Time oldMarkerTime, ROS::Time newMarkerTime, Controller& controller)
		: BaseClass("Marker Position", &entity), mTrackNumber(trackNumber), mOldTime(oldMarkerTime), mNewTime(newMarkerTime), mController(controller)
		{
		}

		virtual ROS::AOperation* Perform()
		{
			ROS::ASceneEntity*	entity = GetEntity();
				
			KeyPointPositionChange* inverse = new KeyPointPositionChange(*entity, mTrackNumber, mNewTime, mOldTime, mController); // Swap the times

			ROS::ARole&	role = entity->GetSceneEntityStateAccessor()->GetRole(mTrackNumber);

			role.ChangeTime(mNewTime, mOldTime);

			mController.SetSecondarySceneEntity(entity);
			mController.SecondarySceneEntityUpdated();

			return inverse;
		}

	private:
		typedef ROS::AOperation BaseClass;

		unsigned int	mTrackNumber;
		ROS::Time		mOldTime;
		ROS::Time		mNewTime;
		Controller&		mController;
};
// --------------------------------------------------------------------------
#endif