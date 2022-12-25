// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneController_h
#define SceneController_h
// --------------------------------------------------------------------------
#include "AController.h"
#include "Links.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class SceneModel;
class SceneView;
// --------------------------------------------------------------------------
//	SceneController
// --------------------------------------------------------------------------
class CPP_DECL SceneController : public AController
{
	public:
        SceneController(SceneModel& sceneModelR, SceneView& sceneViewR);
        /**# :[Description = "Calls Model::Detach()"] */
        virtual ~SceneController();
        virtual void Update();
        virtual void HandleEvent(const Event& kEventR);
    private :
        /**#: [Cardinalities = "1..1/"]*/
        AssPointer<SceneModel>	mSceneModelP;
        /**# message <SceneModel> messageSceneModel_ */

    protected :
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
