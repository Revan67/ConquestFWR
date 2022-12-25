// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ADynamicCamera_h
#define ADynamicCamera_h
// --------------------------------------------------------------------------
#include "ACamera.h"
#include "ADynamicSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	ADynamicCamera
// --------------------------------------------------------------------------
class ADynamicCamera: public virtual ACamera, public virtual ADynamicSceneEntity
{
	public:
		virtual void SetState(const ADynamicCamera& camera);

    protected:
        ADynamicCamera();

		virtual void StateUpdated(Update::ID id);
		virtual void StateUpdated(Update::ID id, Time time);

        virtual void Goto(Time time);

    private:
        typedef ADynamicSceneEntity ADynamicSceneEntityBaseClass;
		typedef ACamera ACameraBaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
