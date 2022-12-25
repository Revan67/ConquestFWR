// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DynamicSceneEntity_h
#define DynamicSceneEntity_h
// --------------------------------------------------------------------------
#include <Memory>
#include "Links.h"
#include "StaticsState.h"
#include "DynamicSceneEntityState.h"
#include "ADynamicSceneEntity.h"
#include "AAudibleSceneEntity.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
// --------------------------------------------------------------------------
//  DynamicSceneEntity
// --------------------------------------------------------------------------
class DynamicSceneEntity: public ADynamicSceneEntity, public virtual AAudibleSceneEntity
{
    public:
		virtual void Delete();

		virtual void Goto(Time time);

        virtual void Write(std::ostream& ostreamR) const;
        virtual void Read(std::istream& ostreamR);

    protected:
        DynamicSceneEntity(const ROSString& kNameR, bool makeNameUnique, Scene& scene);
        DynamicSceneEntity(Scene& scene);

        virtual ~DynamicSceneEntity();

		virtual void StateUpdated(Update::ID id, Time time);

		virtual int GetLocationRoleIndex() const;
		virtual int GetOrientationRoleIndex() const;
		virtual int GetDynamicRoleIndex() const;
		virtual int GetAudioRoleIndex() const;

		virtual AudioPlayingFlags& GetAudioPlayingFlags();
		virtual const AudioPlayingFlags& GetAudioPlayingFlags() const;

    private:
        typedef ADynamicSceneEntity	ADynamicSceneEntityBaseClass;
		typedef AAudibleSceneEntity	AAudibleSceneEntityBaseClass;

		virtual SceneEntityState& GetSceneEntityState();
		virtual const SceneEntityState& GetSceneEntityState() const;
		virtual APhysicalState& GetPhysicalState();
		virtual const APhysicalState& GetPhysicalState() const;

		void InitializeLocationRole();
		void InitializeOrientationRole();
		void InitializeDynamicRole();
		void InitializeAudioRole();

        void WriteSubObject(std::ostream& ostreamR) const;
        void ReadSubObject(std::istream& istreamR);

        /**# :[Cardinalities = "1..1/"] */
		SceneEntityState		mSceneEntityState;
		DynamicSceneEntityState	mDynamicSceneEntityState;
        AudioPlayingFlags						mIsAudioActuallyPlaying;
        int						mLocationRoleIndex;
		int						mOrientationRoleIndex;
		int						mDynamicRoleIndex;
		int						mAudioRoleIndex;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif