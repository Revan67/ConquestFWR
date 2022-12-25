// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Camera_h
#define Camera_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "ADynamicCamera.h"
#include "CameraDynamicsState.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
class CameraStateAccessor;
class ConstCameraStateAccessor;
// --------------------------------------------------------------------------
//  Camera
// --------------------------------------------------------------------------
class Camera: public ADynamicCamera
{
    public:
        Camera(const ROSString& name, bool makeNameUnique, Scene& scene);
        explicit Camera(Scene& scene);

		virtual void Delete();

        virtual ROSString GetArchetypeName()  const;
        static ROSString GetCameraArchetypeName();

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);
 
    protected:
        // The following constructors are for use by descendants
        Camera(const ROSString& name, bool makeNameUnique, ACameraDynamicsState& aCameraState, Scene& scene);
        Camera(std::istream& iStream, ACameraDynamicsState& aCameraState, Scene& scene);

		virtual ~Camera();

		virtual void SetupPosition() const;
        virtual void Render(const ROS::DABaseCamera* camera) const;
		virtual bool FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;

		virtual void RoleUpdated();

		virtual void Respond(const SceneEntityEvent& event);

        virtual int GetLocationRoleIndex() const;
		virtual int GetOrientationRoleIndex() const;
		virtual int GetDynamicRoleIndex() const;
		virtual int GetCameraRoleIndex() const;
		virtual int GetAudioRoleIndex() const;

    private:
        typedef ADynamicCamera BaseClass;

		virtual SceneEntityState& GetSceneEntityState();
		virtual const SceneEntityState& GetSceneEntityState() const;
		virtual APhysicalState& GetPhysicalState();
		virtual const APhysicalState& GetPhysicalState() const;

		void InitializeLocationRole();
		void InitializeOrientationRole();
		void InitializeDynamicRole();
		void InitializeCameraRole();
		void InitializeAudioRole();

		void UpdateSourceTargetEntities();

		void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

		SceneEntityState					mSceneEntityState;
		AggAPointer<CameraDynamicsState>	mCameraState;
		int									mLocationRoleIndex;
		int									mOrientationRoleIndex;
		int									mDynamicRoleIndex;
		int									mCameraRoleIndex;
		int									mAudioRoleIndex;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
