// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LiveCamera_h
#define LiveCamera_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "ADynamicCamera.h"
#include "LiveCameraDynamicsState.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
class CameraStateAccessor;
class ConstCameraStateAccessor;
class ADynamicCamera;
// --------------------------------------------------------------------------
//  LiveCamera
// --------------------------------------------------------------------------
class LiveCamera: public ADynamicCamera
{
    public:
        LiveCamera(const ROSString& name, bool makeNameUnique, Scene& scene);
        explicit LiveCamera(Scene& scene);

		virtual void Delete();

        virtual const std::auto_ptr<ConstSceneEntityStateAccessor> GetConstSceneEntityStateAccessor() const;
        virtual std::auto_ptr<SceneEntityStateAccessor> GetSceneEntityStateAccessor();

		virtual const std::auto_ptr<ConstCameraStateAccessor> GetConstCameraStateAccessor() const;
		virtual std::auto_ptr<CameraStateAccessor> GetCameraStateAccessor();

        virtual const std::auto_ptr<ConstStaticsStateAccessor> GetConstStaticsStateAccessor() const;
        virtual std::auto_ptr<StaticsStateAccessor> GetStaticsStateAccessor();

        virtual const std::auto_ptr<ConstDynamicsStateAccessor> GetConstDynamicsStateAccessor() const;
        virtual std::auto_ptr<DynamicsStateAccessor> GetDynamicsStateAccessor();

        virtual ROSString GetArchetypeName()  const;
        static ROSString GetLiveCameraArchetypeName();

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream); 

    protected:
		virtual ~LiveCamera();

		virtual void Respond(const SceneEntityEvent& event);

        virtual int GetLocationRoleIndex() const;
		virtual int GetOrientationRoleIndex() const;
		virtual int GetDynamicRoleIndex() const;
		virtual int GetCameraRoleIndex() const;
		virtual int GetLiveCameraRoleIndex() const;

        virtual void Goto(Time time);

        virtual void Render(const ROS::DABaseCamera* camera) const;

		virtual ASceneEntity* GetDependentEntity();

    private:
        typedef ADynamicCamera BaseClass;

        void GotoForLiveCameraRole(Time time);

		virtual SceneEntityState& GetSceneEntityState();
		virtual const SceneEntityState& GetSceneEntityState() const;
		virtual APhysicalState& GetPhysicalState();
		virtual const APhysicalState& GetPhysicalState() const;

		virtual void RoleUpdated();

		void UpdateSourceRollingCameras();

		void InitializeLiveCameraRole();

		void SetInternalCameraStateToLiveCameraState();

		void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

		SceneEntityState	mSceneEntityState;
		ADynamicCamera*		mInternalCamera;
		ADynamicCamera*		mLiveCamera;
		int					mLiveCameraRoleIndex;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
