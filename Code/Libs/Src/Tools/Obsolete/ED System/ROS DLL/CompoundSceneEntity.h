// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef CompoundSceneEntityH
#define CompoundSceneEntityH
// --------------------------------------------------------------------------
#include <iostream>
#include "StringType.h"
#include "ACompoundSceneEntity.h"
#include "AAudibleSceneEntity.h"
#include "TimeTag.h"
#include "TimeTag.h"
#include "ROSDLL.h"
#include "StringList.h"
#include "SceneEntityState.h"
#include "AStaticsState.h"
#include "da_vector"
// --------------------------------------------------------------------------
namespace ROS
{
class DACompoundObject;
class DAMotionObject;
class DAHardPoints;
class Scene;
// --------------------------------------------------------------------------
//  DeformableSceneEntity
// --------------------------------------------------------------------------
class CPP_DECL CompoundSceneEntity : public virtual ACompoundSceneEntity, public virtual AAudibleSceneEntity
{
	friend class CompoundStateAccessor;
	friend class ConstCompoundStateAccessor;
	friend class MotionRoleCallback;

    public:
        class ExConstructionFailed: public std::exception
		{
        	public:
				ExConstructionFailed(const ROSString& entityName)
				: mMessage(ROSString("Failed to create Compound Scene Entity: ") + entityName)
				{
				}

				virtual const char* what() const throw()
				{
				  return mMessage.c_str();
				}

			private:
				ROSString	mMessage;
		};

        class ExMotionCreationFailed: public std::exception
		{
        	public:
				ExMotionCreationFailed(const ROSString& motionName)
				: mMessage(ROSString("Failed to create motion: ") + motionName)
				{
				}

				virtual const char* what() const throw()
				{
				  return mMessage.c_str();
				}

			private:
				ROSString	mMessage;
		};

        CompoundSceneEntity(const ROSString& entityName, const ROS::ROSString& categoryName, const StringList& descriptionStrings, Scene& scene);
        explicit CompoundSceneEntity(Scene& scene);

		virtual void Delete();

        virtual ROSString GetArchetypeName()  const;
		static ROSString GetCompoundSceneEntityArchetypeName();

        virtual const std::auto_ptr<ConstCompoundStateAccessor> GetConstCompoundStateAccessor() const;
        virtual std::auto_ptr<CompoundStateAccessor> GetCompoundStateAccessor();

        virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

	protected:
        ~CompoundSceneEntity();

		virtual void Respond(const SceneEntityEvent& event);

		virtual void StateUpdated(Update::ID id, Time time);

		virtual void Goto(Time time);

		virtual int GetLocationRoleIndex() const;
		virtual int GetOrientationRoleIndex() const;
		virtual int GetMotionRoleIndex() const;
		virtual int GetAudioRoleIndex() const;
		virtual int GetParentRoleIndex() const;

        virtual void SetupPosition() const;
        virtual void Render(const DABaseCamera* camera) const;
		virtual bool FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;

        Location GetDAObjectLocation() const;
        Orientation GetDAObjectOrientation() const;

        void SetDAObjectLocation(const Location& location);
        void SetDAObjectOrientation(const Orientation& orient);

        virtual void LocationStateUpdated(Time time);
        virtual void OrientationStateUpdated(Time time);

		virtual AudioPlayingFlags& GetAudioPlayingFlags();
		virtual const AudioPlayingFlags& GetAudioPlayingFlags() const;

		virtual ParentEventFlags&		GetParentEventFlags();
		virtual const ParentEventFlags&	GetParentEventFlags() const;

    private:
        typedef ACompoundSceneEntity	ACompoundSceneEntityBaseClass;
		typedef AAudibleSceneEntity		AAudibleSceneEntityBaseClass;

    	typedef	std::vector<Time> MotionPlayingFlags;
		
		struct TimeDAMotionPair
		{
			explicit TimeDAMotionPair(Time time = kTime0, const DAMotionObject* motionObject = NULL)
			: mTime(time), mMotionObject(motionObject)
			{
			}

			Time					mTime;
			const DAMotionObject*	mMotionObject;
		};

		typedef std::vector<TimeDAMotionPair>	MotionObjects;

		virtual SceneEntityState&		GetSceneEntityState();
		virtual const SceneEntityState&	GetSceneEntityState() const;
		virtual APhysicalState&			GetPhysicalState();
		virtual const APhysicalState&	GetPhysicalState() const;

        virtual void					ShowSkeleton(bool show) { }
        virtual bool					IsSkeletonShowing() const { return false; }

        virtual void					ShowHardPoints(bool show);
        virtual bool					AreHardPointsShowing() const;
        virtual unsigned int			GetHardPointCount() const;
        virtual ROSString				GetHardPointName(unsigned int idx) const;
        virtual Location				GetHardPointLocation(unsigned int idx) const;
        virtual Orientation				GetHardPointOrientation(unsigned int idx) const;
		virtual const HardPointHost*	GetHardPointHost(unsigned int idx) const;
		virtual void					AttachHardPointToParent(unsigned int childHardPointIndex, const HardPointHost* parentHardPointHost, const ROSString& parentHardPointName);
		virtual void					DetachHardPointFromParent(unsigned int childHardPointIndex, const HardPointHost* parentHardPointHost, const ROSString& parentHardPointName);

		virtual unsigned int			GetMotionCount() const;
		virtual ROSString				GetMotionName(int motionIdx) const;
        virtual Time					GetMotionLength(const ROSString& motionName) const;
										
		virtual ROSString				GetCurrentMotionName() const;
        virtual void					SetCurrentMotionName(const ROSString& motionName);
        virtual Time					GetCurrentMotionTime() const;
										
        virtual void    				Start(const ROSString& motionName, Time startTime, Time transition);
        virtual void    				Loop(const ROSString& motionName, Time startTime, Time transition);
        virtual void    				Pause(const ROSString& motionName);
        virtual void    				Resume(const ROSString& motionName);
        virtual void    				Stop(const ROSString& motionName);
										
		virtual void					StartIK(const ROSString& endEffectorName, unsigned int countToRootEffector, AStaticSceneEntity& targetEntity, Time transition);
		virtual IKState					GetIKState(Time startTime) const;
		virtual void					SetIKState(const IKState& iKState, Time startTime);

		virtual long					GetRootEngineIndex () const;

		void							InitializeLocationRole(bool useStateInScript);
		void							InitializeOrientationRole(bool useStateInScript);
		void							InitializeMotionRole(bool useStateInScript);
		void							InitializeAudioRole(bool useStateInScript);
		void							InitializeParentRole(bool useStateInScript);

		virtual void					GotoForLocationRole(Time time);
		virtual void					GotoForOrientationRole(Time time);
        virtual void					GotoForMotionRole(Time time);

								
        unsigned int					GetCameraCount() const;
        ROSString						GetCameraName(unsigned int idx) const;
        Location						GetCameraLocation(unsigned int idx) const;
        Orientation						GetCameraOrientation(unsigned int idx) const;
        float							GetCameraHorizontalFOV(unsigned int idx) const;
        float							GetCameraVerticalFOV(unsigned int idx) const;
										
		void							WriteSubObject(std::ostream& oStreamR) const;
		void							ReadSubObject(std::istream& iStreamR);
										
		void							SetupDACompoundObject(SceneEventFlag& flag);
										
		void							InitMotionPlayingFlags();
		bool							IsMotionActuallyPlayingFlagSet(Time startTime);
		void							SetMotionActuallyPlayingFlag(Time startTime, bool isPlaying);
		Time							GetGreatestMotionPlayingFlagTime() const;
										
		const DAMotionObject*			GetDAMotionObject(Time time);
		void							StopAllMotions();
		void							CreateAllMotions();
		void							DestroyAllMotions();
		void							RemoveMotion(Time motionTime);
		void							ChangeMotionTime(Time currentTime, Time newTime);

        /**#: [Cardinalities = "1..1/"]*/
		SceneEntityState			mSceneEntityState;
        AggAPointer<AStaticsState>	mState;

		ROSString					mCategoryName;
        StringList 					mDescriptionStrings;
        const DACompoundObject*		mDACompoundObject;
		bool						mUseInitialTransition;
        MotionPlayingFlags			mIsMotionActuallyPlaying;
        AudioPlayingFlags			mIsAudioActuallyPlaying;
		ParentEventFlags			mIsParentEventHandled;
		MotionObjects				mMotionObjects;
		StringList					mMotionNames;
        const DAHardPoints*			mDAHardPoints;
        bool    					mShowHardPoints;
        ROSString					mCurrentMotionName;
		int							mLocationRoleIndex;
		int							mOrientationRoleIndex;
		int							mMotionRoleIndex;
		int							mAudioRoleIndex;
		int							mParentRoleIndex;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
