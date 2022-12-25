// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ACamera_h
#define ACamera_h
// --------------------------------------------------------------------------
#include "ASceneEntity.h"
#include "ConstCameraStateAccessor.h"
#include "CameraStateAccessor.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	ACamera
// --------------------------------------------------------------------------
class ACamera: public virtual ASceneEntity
{
	friend ConstCameraStateAccessor;
	friend CameraStateAccessor;

	public:
		virtual const std::auto_ptr<ConstCameraStateAccessor> GetConstCameraStateAccessor() const;
		virtual std::auto_ptr<CameraStateAccessor> GetCameraStateAccessor();

    protected:
        ACamera();

		virtual void StateUpdated(Update::ID id);

		virtual void StateUpdated(Update::ID id, Time time);

        virtual void Goto(Time time);

		virtual void CameraStateUpdated(Time time);

		virtual int GetCameraRoleIndex() const = 0;

    private:
		typedef ASceneEntity BaseClass;

        void GotoForCameraRole(Time time);
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
