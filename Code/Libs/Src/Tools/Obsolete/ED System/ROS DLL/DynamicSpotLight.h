// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DynamicSpotLight_h
#define DynamicSpotLight_h
// --------------------------------------------------------------------------
#include "ADynamicSpotLight.h"
#include "ROSDLL.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ADynamicSpotLightState;
// --------------------------------------------------------------------------
//  DynamicSpotLight
// --------------------------------------------------------------------------
class CPP_DECL DynamicSpotLight: public ADynamicSpotLight
{
    public:
        DynamicSpotLight(const ROSString& name, bool makeNameUnique, Scene& scene, ADynamicSpotLightState& lightState);
        DynamicSpotLight(Scene& scene, ADynamicSpotLightState& lightState);

		virtual void Delete();

        virtual ROSString GetArchetypeName() const;
        static ROSString GetDynamicSpotLightArchetypeName();

		virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

    protected:
        virtual ~DynamicSpotLight();

		virtual void SetupPosition() const;
        virtual void Render(const ROS::DABaseCamera* camera) const;
		virtual bool FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;

		virtual int GetLocationRoleIndex() const;
		virtual int GetOrientationRoleIndex() const;
		virtual int GetSpotLightRoleIndex() const;
		virtual int GetDynamicRoleIndex() const;
		virtual int GetAudioRoleIndex() const;

    private:
        typedef ADynamicSpotLight BaseClass;

		virtual SceneEntityState& GetSceneEntityState();
		virtual const SceneEntityState& GetSceneEntityState() const;
		virtual APhysicalState& GetPhysicalState();
		virtual const APhysicalState& GetPhysicalState() const;

		void InitializeLocationRole();
		void InitializeOrientationRole();
		void InitializeSpotLightRole();
		void InitializeDynamicRole();
		void InitializeAudioRole();

		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

		SceneEntityState		mSceneEntityState;
		ADynamicSpotLightState&	mLightState;
		int						mLocationRoleIndex;
		int						mOrientationRoleIndex;
		int						mSpotLightRoleIndex;
		int						mDynamicRoleIndex;
		int						mAudioRoleIndex;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif