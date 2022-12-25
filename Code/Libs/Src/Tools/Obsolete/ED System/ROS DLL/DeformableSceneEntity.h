//---------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DeformableSceneEntityH
#define DeformableSceneEntityH

#include <iostream>
#ifdef __BCPLUSPLUS__
#include <include\vector>
#else
#include <vector>
#endif
#include "StringType.h"
#include "ACompoundSceneEntity.h"
#include "AAudibleSceneEntity.h"
#include "TimeTag.h"
#include "ROSDLL.h"
#include "DeformableEntityStaticsState.h"
#include "StringList.h"
#include "TimeType.h"
#include "SceneEntityState.h"
#include "ACompoundSceneEntityState.h"
#include "IKState.h"
#include <matrix.h>
// --------------------------------------------------------------------------
namespace ROS
{
class DADeformableObject;
class Scene;
class DAHardPoints;
class DAIK;
// --------------------------------------------------------------------------
//  DeformableSceneEntity
// --------------------------------------------------------------------------
class CPP_DECL DeformableSceneEntity: public virtual ACompoundSceneEntity, public virtual AAudibleSceneEntity
{
	friend class MotionRoleCallbackForDeformable;

    public:
        class ExConstructionFailed: public std::exception
		{
        	public:
				ExConstructionFailed(const ROSString& entityName)
				: mMessage(ROSString("Failed to create Deformable Scene Entity: ") + entityName)
				{
				}

				virtual const char* what() const throw()
				{
				  return mMessage.c_str();
				}

			private:
				ROSString	mMessage;
		};

        DeformableSceneEntity(const ROSString& entityName, const ROS::ROSString& categoryName, const StringList& descriptionStrings, Scene& scene);
        explicit DeformableSceneEntity(Scene& scene);

		virtual void Delete();

		virtual void Replace(const ROSString& name, const ROSString& categoryName, const StringList& descriptionStrings);

        virtual ROSString GetArchetypeName()  const;
		static ROSString GetDeformableSceneEntityArchetypeName();

        virtual void Goto(Time time);

        virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

		Time GetTransitionTime(Time startTime) const;
		void SetTransitionTime(Time startTime, Time transitionTime);

		const IKState &GetIKState (unsigned int ikIndex);

    protected:
        virtual ~DeformableSceneEntity();

		virtual void Respond(const SceneEntityEvent& event);

		virtual void StateUpdated(Update::ID id, Time time);

		virtual void LocationStateUpdated(Time time);
		virtual void OrientationStateUpdated(Time time);

        virtual void SetupPosition() const;
        virtual void Render(const DABaseCamera* camera) const;
		virtual bool FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;

/*        Location GetDAObjectLocation() const;
        Orientation GetDAObjectOrientation() const;
*/
        void SetDAObjectLocation(const Location& location);
        void SetDAObjectOrientation(const Orientation& orient);

		virtual int GetLocationRoleIndex() const;
		virtual int GetOrientationRoleIndex() const;
		virtual int GetMotionRoleIndex() const;
		virtual int GetAudioRoleIndex() const;
		virtual int GetParentRoleIndex() const;

		virtual AudioPlayingFlags& GetAudioPlayingFlags();
		virtual const AudioPlayingFlags& GetAudioPlayingFlags() const;

		virtual ParentEventFlags&		GetParentEventFlags();
		virtual const ParentEventFlags&	GetParentEventFlags() const;

    private:
        typedef ACompoundSceneEntity	ACompoundSceneEntityBaseClass;
		typedef AAudibleSceneEntity		AAudibleSceneEntityBaseClass;
    	typedef	std::vector<Time>		MotionPlayingFlags;
		typedef std::vector<Time>		LocationAppliedFlags;
		typedef std::vector<Time>		OrientationAppliedFlags;

	public:
		class IKRecord
		{
			public:
				IKRecord()
				{
				}

				IKRecord(const ROSString& endEffectorName, unsigned int countToRootEffector, const AStaticSceneEntity& targetEntity)
				: mIKState(endEffectorName, countToRootEffector, targetEntity), mDAIK(NULL)
				{
					UpdateVectorToTarget();

					// *** We can't to a valid update here unless we can get information about the end effector's
					// *** position. I suppose we could get it from the name, but for now, we will just
					// *** use the origin for now.

					UpdateOrientToTarget(Vector(0,0,0));
				}

				void UpdateVectorToTarget()
				{
					mVectorToTarget = mIKState.GetTargetEntity().GetConstStaticsStateAccessor()->GetLocation().GetVector();
				}

				void UpdateOrientToTarget(const Vector &endEffectorPosition)
				{
					// *** Yes, I know that this code is ugly. Feel free to make it cleaner, if you like. -TNB

					// Get the vector from the given position to the target. This is the alignment vector.
					Vector vk =
						mIKState.GetTargetEntity().GetConstStaticsStateAccessor()->GetLocation().GetVector() - 
						endEffectorPosition;
					vk.normalize();

					// Calculate the other two vectors that define the frame. The assumption here is that world up
					// is generally up.

					// *** TODO: Check for degenerate case
					Vector vi = cross_product (Vector(0,1,0), vk);
					Vector vj = cross_product (vk, vi);
					vj.normalize();

					// Where these vectors get stored in the matrix is determined by the axis in the state.
					ROS::IKState::Axis frontAxis = mIKState.GetEndEffectorAxis();
					ROS::IKState::Axis upAxis = mIKState.GetEndEffectorUpAxis();

					ASSERT(frontAxis != upAxis && "IK up and front vectors cannot be the same");

					bool iSet = false, jSet = false, kSet = false;

					switch(frontAxis)
					{
					case ROS::IKState::kXAxis:
						// Orient the X axis with vk
						iSet = true;
						mOrientToTarget.set_i(vk);
						break;

					case ROS::IKState::kYAxis:
						// Orient the Y axis with vk.
						jSet = true;
						mOrientToTarget.set_j(vk);
						break;

					default:
						ASSERT(0 && "Unknown IK Front axis");
						// NOTE: Fall through to Z axis as default behavior here.

					case ROS::IKState::kZAxis:
						// Orient the Z axis with vk.
						kSet = true;
						mOrientToTarget.set_k(vk);
						break;

					case ROS::IKState::kNXAxis:
						// Orient the X axis with -vk
						iSet = true;
						mOrientToTarget.set_i(-vk);
						break;

					case ROS::IKState::kNYAxis:
						// Orient the Y axis with -vk.
						jSet = true;
						mOrientToTarget.set_j(-vk);
						break;

					case ROS::IKState::kNZAxis:
						// Orient the Z axis with -vk.
						kSet = true;
						mOrientToTarget.set_k(-vk);
						break;
					}

					switch(upAxis)
					{
					case ROS::IKState::kXAxis:
						// Orient the X axis with vj
						iSet = true;
						mOrientToTarget.set_i(vj);
						break;

					default:
						ASSERT(0 && "Unknown IK Up axis");
						// NOTE: Fall through to Y axis as default behavior here.

					case ROS::IKState::kYAxis:
						// Orient the Y axis with vj.
						jSet = true;
						mOrientToTarget.set_j(vj);
						break;

					case ROS::IKState::kZAxis:
						// Orient the Z axis with vj.
						kSet = true;
						mOrientToTarget.set_k(vj);
						break;

					case ROS::IKState::kNXAxis:
						// Orient the X axis with vj
						iSet = true;
						mOrientToTarget.set_i(-vj);
						break;

					case ROS::IKState::kNYAxis:
						// Orient the Y axis with vj.
						jSet = true;
						mOrientToTarget.set_j(-vj);
						break;

					case ROS::IKState::kNZAxis:
						// Orient the Z axis with vj.
						kSet = true;
						mOrientToTarget.set_k(-vj);
						break;
					}

					if (!iSet)
					{
						vi = cross_product (mOrientToTarget.get_j(), mOrientToTarget.get_k());
						vi.normalize();
						mOrientToTarget.set_i(vi);
					}
					else if (!jSet)
					{
						vi = cross_product (mOrientToTarget.get_k(), mOrientToTarget.get_i());
						vi.normalize();
						mOrientToTarget.set_j(vi);
					}
					else if (!kSet)
					{
						vi = cross_product (mOrientToTarget.get_i(), mOrientToTarget.get_j());
						vi.normalize();
						mOrientToTarget.set_k(vi);
					}
					else
					{
						ASSERT (0 && "No unset axis!");
					}
				}

				void Write(std::ostream& oStream) const;
				void Read(std::istream& iStream);

				IKState		mIKState;
				const DAIK*	mDAIK;				// Not persistent
				Vector		mVectorToTarget;	// Not persistent
				::Matrix    mOrientToTarget;    // Not persistent
			
			private:
				enum FieldID
				{
					kIKState
				};
		};

		typedef std::vector<IKRecord>	IKRecordCollection;

	private:
		enum FieldID
		{
			kDescriptionStrings,
			kSceneEntityState,
			kIKRecords,			// Added in ROS Version 2.9.0.0
			kCategoryName		// Added in ROS Version 2.9.1.0
		};

		virtual SceneEntityState&		GetSceneEntityState();
		virtual const					SceneEntityState& GetSceneEntityState() const;
		virtual APhysicalState&			GetPhysicalState();
		virtual const APhysicalState&	GetPhysicalState() const;

        virtual void					ShowSkeleton(bool show);
        virtual bool					IsSkeletonShowing() const;

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
										
        virtual void    				Start(const ROSString& motionName, Time startTime, Time transition);
        virtual void    				Loop(const ROSString& motionName, Time startTime, Time transition);
        virtual void    				Pause(const ROSString& motionName);
        virtual void    				Resume(const ROSString& motionName);
        virtual void    				Stop(const ROSString& motionName);

		virtual void					StartIK(const ROSString& endEffectorName, unsigned int countToRootEffector, AStaticSceneEntity& targetEntity, Time transition);
		virtual void					SetIKState(const IKState& iKState, Time startTime);
		virtual IKState					GetIKState(Time startTime) const;
										
		void							RemoveIKRecord(unsigned int iKRecordIndex);

		virtual long					GetRootEngineIndex () const;
										
		virtual void					GotoForLocationRole(Time time);
		virtual void					GotoForOrientationRole(Time time);
        virtual void					GotoForMotionRole(Time time);
						
        void							WriteSubObject(std::ostream& oStream) const;
        void							ReadSubObject(std::istream& iStream);
										
        Time							GetCurrentMotionTime() const;
										
        void    						SetTimeTags(const ROSString& motionName, const TimeTagList& timeTagList);
										
		void 							SetupDADeformableObject(SceneEventFlag& flag);
		void							InitializeLocationRole(bool useStateInScript);
		void							InitializeOrientationRole(bool useStateInScript);
		void 							InitializeMotionRole(bool useStateInScript);
		void 							InitializeAudioRole(bool useStateInScript);
		void 							InitializeParentRole(bool useStateInScript);
										
		void							RemoveParent(ACompoundSceneEntity& aCompoundSE);
										
		void 							InitMotionPlayingFlags();
        void							SetMotionActuallyPlayingFlag(Time startTime, bool isPlaying);
        bool							IsMotionActuallyPlayingFlagSet(Time startTime) const;
		Time							GetGreatestMotionPlayingFlagTime() const;
										
		void 							InitLocationAppliedFlags();
        void							SetLocationAppliedFlag(Time time, bool isApplied);
        bool							IsLocationAppliedFlagSet(Time time) const;
										
		void 							InitOrientationAppliedFlags();
        void							SetOrientationAppliedFlag(Time time, bool isApplied);
        bool							IsOrientationAppliedFlagSet(Time time) const;

		void							InitIKRecords();
//		MotionState						GetInitialMotionState() const;

//      mDefObject;
        const DADeformableObject*				mDADeformableObject;
        MotionPlayingFlags						mIsMotionActuallyPlaying;
        AudioPlayingFlags						mIsAudioActuallyPlaying;
		LocationAppliedFlags					mIsLocationApplied;
		OrientationAppliedFlags					mIsOrientationApplied;
		ParentEventFlags						mIsParentEventHandled;
		IKRecordCollection						mIKRecords;
		ROSString								mCategoryName;
        StringList								mDescriptionStrings;
        ROSString								mCurrentMotionName;
		SceneEntityState						mSceneEntityState;
        AggAPointer<ACompoundSceneEntityState>	mState;
		Time									mCurrentStateTime;
		bool									mUseInitialTransition;
        bool    								mShowHardPoints;
		int										mLocationRoleIndex;
		int										mOrientationRoleIndex;
		int										mMotionRoleIndex;
		int										mAudioRoleIndex;
		int										mParentRoleIndex;
		bool                                    mShowSkeleton;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
