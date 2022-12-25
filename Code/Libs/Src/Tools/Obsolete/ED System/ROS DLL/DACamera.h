// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DACamera_h
#define DACamera_h

// --------------------------------------------------------------------------
#include "StringType.h"
#include "ADynamicCamera.h"
#include "SceneEntityState.h"
#include "ACameraDynamicsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
class ConstCameraStateAccessor;
class CameraStateAccessor;
// --------------------------------------------------------------------------
//  DACamera
// --------------------------------------------------------------------------
class DACamera: public ADynamicCamera
{   
    public:
        class ExCameraCreationFailed: public std::exception
        {   public:
                ExCameraCreationFailed(const ROSString& cameraName)
				: mMessage(ROSString("Failed to create camera: ") + cameraName)
                {
                };

                virtual const char* what() const throw()
                {
                  return mMessage.c_str();
                }

			private:
				ROSString	mMessage;
        };

        DACamera(const ROSString& name, bool makeNameUnique, Scene& scene, bool isInternal = false);
        explicit DACamera(Scene& scene);

		virtual void Delete();

        virtual ROSString GetArchetypeName()  const;
        static ROSString GetDACameraArchetypeName();

        virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

    protected:
        virtual ~DACamera();

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

		void InitializeLocationRole(bool useStateInScript);
		void InitializeOrientationRole(bool useStateInScript);
		void InitializeDynamicRole(bool useStateInScript);
		void InitializeCameraRole(bool useStateInScript);
		void InitializeAudioRole(bool useStateInScript);

		void UpdateSourceTargetEntities();

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

        void SetupDACamera(SceneEventFlag& flag);

        unsigned int						mWidth;
        unsigned int						mHeight;
		SceneEntityState					mSceneEntityState;
        AggAPointer<ACameraDynamicsState>	mCameraState;
		int									mLocationRoleIndex;
		int									mOrientationRoleIndex;
		int									mDynamicRoleIndex;
		int									mCameraRoleIndex;
		bool								mIsInternal;
		int									mAudioRoleIndex;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
