// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef TrackController_h
#define TrackController_h
// --------------------------------------------------------------------------
#include "Controller.h"
#include "Callback.hpp"
// --------------------------------------------------------------------------
namespace ROS
{
class SceneModel;
}
// --------------------------------------------------------------------------
//	TrackController
// --------------------------------------------------------------------------
class TrackController: public Controller 
{
	public:
    	typedef CBFunctor1<int> UpdateCB;

        TrackController(ROS::SceneModel& sceneModel, const UpdateCB& updateCB);
		virtual ~TrackController();

		virtual void Update(int updateID);

	private :
		typedef Controller BaseClass;

			/**#: [Cardinalities = "1..1/"]*/
		AssPointer<ROS::SceneModel>	mSceneModelP;
		const UpdateCB              mUpdateCB;
};
// --------------------------------------------------------------------------
#endif
