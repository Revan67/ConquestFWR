// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include <iostream>
#include <Memory>
#include "GLUtils.h"
#include "DeformableSceneEntity.h"
#include "MatrixUtil.h"
#include "DADeformableObject.h"
#include "TimeTag.h"
#include "CompoundStaticsState.h"
#include "MotionState.h"
#include "Role.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "MotionRole.h"
#include "ParentRole.h"
#include "Scene.h"
#include "RayMeshCollision.h"
#include "StringUtils.h"
#include "ConstMotionStateAccessor.h"
#include "DABaseCamera.h"

#if 1
// Temporary include
#include <fstream>
#endif

#if 0
#include "EventIterator.h"
#endif

#include "Utils.h"


// The following automatically adds a stop IK state when a start IK state is added
#define STOP_IK_KEY_POINT


// --------------------------------------------------------------------------
namespace DeformableSceneEntity_cpp
{
enum TimeTagListFieldID
{
	kTimeTagList	
};
}
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// IKRecord methods
// --------------------------------------------------------------------------
void DeformableSceneEntity::IKRecord::Write(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);


	oWiz.Put(kIKState, mIKState);
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::IKRecord::Read(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kIKState, mIKState);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::DeformableSceneEntity::IKRecord& iKRecord)
{
	iKRecord.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::DeformableSceneEntity::IKRecord& iKRecord)
{
	iKRecord.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// IKRecordsWriter
// --------------------------------------------------------------------------
class IKRecordsWriter
{
	public:
		IKRecordsWriter(const DeformableSceneEntity::IKRecordCollection& iKRecordCollection)
		: mIKRecords(iKRecordCollection)
		{
		}

		void Write(std::ostream& oStream) const
		{
			OStreamWiz<FieldID>	oWiz(oStream);

			unsigned int	count = mIKRecords.size();

			oWiz.Put(kRecordCount, count);


			for(unsigned int idx = 0; idx < count; ++idx)
			{
				oWiz.Put(static_cast<FieldID>(kFirstRecord + idx), mIKRecords[idx]);
			}
		}

	private:
		enum FieldID
		{
			kRecordCount,
			kFirstRecord
		};

		const DeformableSceneEntity::IKRecordCollection&	mIKRecords;
};
// --------------------------------------------------------------------------
// IKRecordsReader
// --------------------------------------------------------------------------
class IKRecordsReader
{
	public:
		IKRecordsReader(DeformableSceneEntity::IKRecordCollection& iKRecordCollection)
		: mIKRecords(iKRecordCollection)
		{
		}

		void Read(std::istream& iStream)
		{
			IStreamWiz<FieldID>	iWiz(iStream);

			unsigned int	count;

			iWiz.Get(kRecordCount, count);

			mIKRecords.resize(count);

			for(unsigned int idx = 0; idx < count; ++idx)
			{
				iWiz.Get(static_cast<FieldID>(kFirstRecord + idx), mIKRecords[idx]);
			}
		}

	private:
		enum FieldID
		{
			kRecordCount,
			kFirstRecord
		};

		DeformableSceneEntity::IKRecordCollection&	mIKRecords;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::IKRecordsWriter& iKRecMan)
{
	iKRecMan.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::IKRecordsReader& iKRecMan)
{
	iKRecMan.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// MotionRoleCallbackForDeformable
// --------------------------------------------------------------------------
class MotionRoleCallbackForDeformable: public MotionRole::UpdateCallback
{
	public:
		MotionRoleCallbackForDeformable(DeformableSceneEntity& deformableSE)
		: mDeformableSE(deformableSE), mIKMotionStateRemoved(false)
		{
		}

		virtual void RemoveStarted(const MotionRole::UpdateCallback::RoleType& role, Time time) 
		{
			const MotionState	state = role.GetState(time);

			if(state.GetMotionEvent() == MotionState::kStartIK || state.GetMotionEvent() == MotionState::kStopIK)
			{
				mIKMotionStateRemoved = true;
				mLastIKRecordIndexRemoved = state.GetIKRecordIndex();;
			}
			else
			{
				mIKMotionStateRemoved = false;
			}
		}

		virtual void RemoveFinished(const MotionRole::UpdateCallback::RoleType& role, Time time) 
		{
			RemoveMotion();
		}
		
		virtual void RemoveStarted(const MotionRole::UpdateCallback::RoleType& role, unsigned int timePointIndex) 
		{
			const MotionState	state = role.GetState(timePointIndex);

			if(state.GetMotionEvent() == MotionState::kStartIK || state.GetMotionEvent() == MotionState::kStopIK)
			{
				mIKMotionStateRemoved = true;
				mLastIKRecordIndexRemoved = state.GetIKRecordIndex();;
			}
			else
			{
				mIKMotionStateRemoved = false;
			}
		}
		
		virtual void RemoveFinished(const MotionRole::UpdateCallback::RoleType& role, unsigned int timePointIndex) 
		{
			RemoveMotion();
		}

	private:
		void RemoveMotion()
		{
			if(mIKMotionStateRemoved)
			{
				mDeformableSE.RemoveIKRecord(mLastIKRecordIndexRemoved);
			}
		}

		DeformableSceneEntity&	mDeformableSE;
		bool					mIKMotionStateRemoved;
		unsigned int			mLastIKRecordIndexRemoved;
};
// --------------------------------------------------------------------------
#if 0
// --------------------------------------------------------------------------
void EventHandlerFunction(unsigned int channel_id, void* user_supplied, const EventIterator& event_iter)
{
    for (int idx = 0; idx < event_iter.get_event_count (); ++idx)
    {
        float time = event_iter.get_event_time (idx);
        unsigned int type = event_iter.get_event_type (idx);
        int tag = *((int*)event_iter.get_event_data (idx));

        Beep(100, 100);
    }
}
#endif
// --------------------------------------------------------------------------
CPP_DEFN DeformableSceneEntity::DeformableSceneEntity(const ROSString& entityName, const ROSString& categoryName, const StringList& descriptionStrings, Scene& scene)
:mSceneEntityState(*this, entityName, false), mDADeformableObject(NULL), mState(NULL),
mShowHardPoints(false), mCategoryName(categoryName), mDescriptionStrings(descriptionStrings), 
mCurrentStateTime(0), mShowSkeleton(false)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));

	mSceneEntityState.SetScene(&scene);

	SceneEventFlag	flag = kSEFNone;

	SetupDADeformableObject(flag);

	const bool	useStateInScript = (flag & kSEFUseStateInScriptAsInitialState) != 0;
	const bool	useFloorHeightInScript = (flag & kSEFUseFloorHeightInScript) != 0;
	
	mUseInitialTransition = (flag & kSEFUseTransitionInScriptTheFirstTime) != 0;

	InitializeLocationRole(useStateInScript);
	InitializeOrientationRole(useStateInScript);

	mMotionRoleIndex = mSceneEntityState.AddRole(*new MotionRole(false, Interpolate, Interpolate, GetStateName, new MotionRoleCallbackForDeformable(*this)));
    InitializeMotionRole(useStateInScript);

	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, new AudioRoleCallback(*this)));
    InitializeAudioRole(useStateInScript);

	mParentRoleIndex = mSceneEntityState.AddRole(*new ParentRole(false, Interpolate, Interpolate, GetStateName, new ParentRoleCallback(*this)));
    InitializeParentRole(useStateInScript);

	InitLocationAppliedFlags();
	InitOrientationAppliedFlags();
	InitMotionPlayingFlags();

	Goto(scene.GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
CPP_DEFN DeformableSceneEntity::DeformableSceneEntity(Scene& scene)
: mSceneEntityState(*this, "Deformable Scene Entity", false), mDADeformableObject(NULL), 
mShowHardPoints(false), mState(NULL), mUseInitialTransition(true), 
mCurrentStateTime(0), mShowSkeleton(false)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mMotionRoleIndex = mSceneEntityState.AddRole(*new MotionRole(false, Interpolate, Interpolate, GetStateName, new MotionRoleCallbackForDeformable(*this)));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, new AudioRoleCallback(*this)));
	mParentRoleIndex = mSceneEntityState.AddRole(*new ParentRole(false, Interpolate, Interpolate, GetStateName, new ParentRoleCallback(*this)));

	mSceneEntityState.SetScene(&scene);

    //OutputDebugString("In DeformableSceneEntity::DeformableSceneEntity\n");
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Delete()
{
    ACompoundSceneEntityBaseClass::Delete();
	AAudibleSceneEntityBaseClass::Delete();	

	delete this;
}
// --------------------------------------------------------------------------
CPP_DEFN DeformableSceneEntity::~DeformableSceneEntity()
{
	if(IsNotNull(mDADeformableObject))
	{
#if 0
		//************* NOTE: Wonder why this fails????? **********
		DeformableObjectStopMotion(mDADeformableObject);
#endif

		void*	userData = mSceneEntityState.GetUserData();

		mSceneEntityState.GetScene().DestroyDADeformableObject(mDADeformableObject, mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, &userData);

		mSceneEntityState.SetUserData(userData);

		DeformableEntityStaticsState*	state = dynamic_cast<DeformableEntityStaticsState*>(mState.get());
		ASSERT(state);

		state->SetDADeformableObject(NULL);

		//OutputDebugString("In ~DeformableSceneEntity(). DA deformable destroyed by client.\n");
	}
	else
	{
		//OutputDebugString("In ~DeformableSceneEntity(). Destroyed.\n");
	}
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::Replace(const ROSString& entityName, const ROSString& categoryName, const StringList& descriptionStrings)
{
	void*						userData = mSceneEntityState.GetUserData();
	SceneEventFlag				flag = kSEFNone;
	const DADeformableObject*	dADeformableObject = mSceneEntityState.GetScene().CreateDADeformableObject(entityName, categoryName, descriptionStrings, &userData, &flag);

	mSceneEntityState.SetUserData(userData);

	if(dADeformableObject)
	{
		mSceneEntityState.GetScene().DestroyDADeformableObject(mDADeformableObject, mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, NULL);
		
		mSceneEntityState.SetName(entityName);
		mDADeformableObject = dADeformableObject;
		mDescriptionStrings.Replace(descriptionStrings);
		mCategoryName = categoryName;

		DeformableEntityStaticsState*	state = dynamic_cast<DeformableEntityStaticsState*>(mState.get());
		ASSERT(state);
		state->SetDADeformableObject(dADeformableObject);
	}
	else
	{
		throw ExConstructionFailed(entityName);
	}
}
// --------------------------------------------------------------------------
bool DeformableSceneEntity::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{	
	if(mDADeformableObject)
	{
		return DeformableObjectIntersect(mDADeformableObject, intersectInfo, distance);
	}
	else
	{
		return false;
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::InitializeMotionRole(bool useStateInScript)
{
	const int	motionRoleIndex = GetMotionRoleIndex();
	ASSERT(motionRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);

    aRole->SetName("Motion");
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::InitializeAudioRole(bool useStateInScript)
{
	const int	audioRoleIndex = GetAudioRoleIndex();
	ASSERT(audioRoleIndex >= 0);
    ARole&		aRole = mSceneEntityState.GetRole(audioRoleIndex);

    aRole.SetName("Audio");
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::InitializeParentRole(bool useStateInScript)
{
	const int	parentRoleIndex = GetParentRoleIndex();
	ASSERT(parentRoleIndex >= 0);
    ARole&		aRole = mSceneEntityState.GetRole(parentRoleIndex);

    aRole.SetName("Parent");
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetupDADeformableObject(SceneEventFlag& flag)
{
	void*	userData = mSceneEntityState.GetUserData();

	mDADeformableObject = mSceneEntityState.GetScene().CreateDADeformableObject(mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, &userData, &flag);

	mSceneEntityState.SetUserData(userData);

    if(IsNotNull(mDADeformableObject))
	{	
#if 0
		try
		{
			DeformableObjectAddArchetypeTimeTagChannel(mDADeformableObject, 16);

			const unsigned int count = 4;

			float time[count] = {1, 2, 3, 4};
			int tag[count] = {1, 2, 3, 4};

			DeformableObjectReplaceArchetypeTimeTagChannelData(mDADeformableObject, 16, time, tag, count);
		}
		catch(...)
		{
			void*	userData = mSceneEntityState.GetUserData();

			mSceneEntityState.GetScene().DestroyDADeformableObject(mDADeformableObject, mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, &userData);

			mSceneEntityState.SetUserData(userData);

			throw ExConstructionFailed(mSceneEntityState.GetName());
		}
#endif
		try
		{
			mState = AggAPointer<ACompoundSceneEntityState>(new DeformableEntityStaticsState(mDADeformableObject));
		}
		catch(...)
		{
			void*	userData = mSceneEntityState.GetUserData();

			mSceneEntityState.GetScene().DestroyDADeformableObject(mDADeformableObject, mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, &userData);

			mSceneEntityState.SetUserData(userData);

			throw ExConstructionFailed(mSceneEntityState.GetName());
		}
	}
	else
	{
		try
		{
			mState = AggAPointer<ACompoundSceneEntityState>(new CompoundStaticsState);

			flag = kSEFUseStateInScriptAsInitialState;
		}
		catch(...)
		{
			throw ExConstructionFailed(mSceneEntityState.GetName());
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString DeformableSceneEntity::GetArchetypeName()  const
{
    return GetDeformableSceneEntityArchetypeName();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString DeformableSceneEntity::GetDeformableSceneEntityArchetypeName()
{
    return "DeformableSceneEntity";
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetupPosition() const
{
    // This method is a noop on purpose, since DeformableSceneEntity::Render() takes care of transforms
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Render(const DABaseCamera* camera) const
{
	if(mDADeformableObject)
	{
//		if (!mShowSkeleton)
		{
			DeformableObjectRender(mDADeformableObject, camera);
		}
	
		if(mShowHardPoints)
		{
			DeformableObjectRenderHardpoints(mDADeformableObject, camera);
		}

		if(mShowSkeleton)
		{
			DeformableObjectRenderSkeleton(mDADeformableObject, camera, 0.3);

			// Render the end effector for each of the active IK bones
			const unsigned int	iKCount = mIKRecords.size();

			for(unsigned int iKIndex = 0; iKIndex < iKCount; ++iKIndex)
			{
				if(mIKRecords[iKIndex].mDAIK != NULL)
				{
					// Find the current position of the end effector.
					Vector endPos;
					DeformableObjectGetEndEffectorPosition (mDADeformableObject, mIKRecords[iKIndex].mIKState, endPos);
					const Transform tr (mIKRecords[iKIndex].mOrientToTarget, endPos);
					Transform modelView = CameraGetTransform(camera).get_inverse() * tr;
					GL::DrawCoordinateFrame (modelView, 1.0f, 1.0f);
				}
			}
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::ShowSkeleton(bool show)
{
	mShowSkeleton = show;
}
// --------------------------------------------------------------------------
CPP_DEFN bool DeformableSceneEntity::IsSkeletonShowing() const
{
	return mShowSkeleton;
}
// --------------------------------------------------------------------------
CPP_DEFN bool DeformableSceneEntity::AreHardPointsShowing() const
{
    return mShowHardPoints;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::ShowHardPoints(bool show)
{
    mShowHardPoints = show;
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int DeformableSceneEntity::GetHardPointCount() const
{
    return DeformableObjectGetHardpointCount(mDADeformableObject);
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString DeformableSceneEntity::GetHardPointName(unsigned int idx) const
{
	ROS::ROSString	name;

    DeformableObjectGetHardPointName(mDADeformableObject, idx, name);

	return name;
}
// --------------------------------------------------------------------------
CPP_DEFN Location DeformableSceneEntity::GetHardPointLocation(unsigned int idx) const
{
    Vector    location;

    DeformableObjectGetHardPointPosition(mDADeformableObject, idx, location);

    return Location(location);
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation DeformableSceneEntity::GetHardPointOrientation(unsigned int idx) const
{
    Orientation    orient;

    DeformableObjectGetHardPointOrientation(mDADeformableObject, idx, orient);

    return orient;
}
// --------------------------------------------------------------------------
CPP_DEFN const HardPointHost* DeformableSceneEntity::GetHardPointHost(unsigned int idx) const
{
	return DeformableObjectGetHardPointHost(mDADeformableObject, idx);
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Start(const ROSString& motionName, Time startTime, Time transition)
{
	if(!Performing())
    {
		Time		currentTime = GetCurrentTimePoint();
		const int	motionRoleIndex = GetMotionRoleIndex();
		ASSERT(motionRoleIndex >= 0);
		ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
        MotionRole*	motionRole = dynamic_cast<MotionRole*>(aRole);
		ASSERT(motionRole);

		motionRole->StateUpdated(MotionState(MotionState::kStartMotion, motionName, startTime, transition), currentTime);
    }
#if 0
    else
    {    
		if(mDADeformableObject)
		{
			DeformableObjectStartMotion(mDADeformableObject, motionName, false, 0.0, transition.GetTime());
		}
    }
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Loop(const ROSString& motionName, Time startTime, Time transition)
{
	if(!Performing())
    {   
		Time		currentTime = GetCurrentTimePoint();
		const int	motionRoleIndex = GetMotionRoleIndex();
		ASSERT(motionRoleIndex >= 0);
		ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
        MotionRole* motionRole = dynamic_cast<MotionRole*>(aRole);
		ASSERT(motionRole);

		motionRole->StateUpdated(MotionState(MotionState::kLoopMotion, motionName, startTime, transition), currentTime);
    }
#if 0
    else
    {    
		if(mDADeformableObject)
		{
			DeformableObjectStartMotion(mDADeformableObject, motionName, true, 0.0, transition.GetTime());
		}
    }
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Pause(const ROSString& motionName)
{
    if(!Performing())
    {   
		Time		currentTime = GetCurrentTimePoint();
		const int	motionRoleIndex = GetMotionRoleIndex();
		ASSERT(motionRoleIndex >= 0);
		ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
        MotionRole* motionRole = dynamic_cast<MotionRole*>(aRole);
		ASSERT(motionRole);

		motionRole->StateUpdated(MotionState(MotionState::kPauseMotion, motionName, kTime0, kTime0), currentTime);
    }
#if 0
    else
	{   
		if(mDADeformableObject)
		{
			DeformableObjectPauseMotion(mDADeformableObject);
		}
    }
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Resume(const ROSString& motionName)
{
	if(!Performing())
    {   
		Time		currentTime = GetCurrentTimePoint();
		const int	motionRoleIndex = GetMotionRoleIndex();
		ASSERT(motionRoleIndex >= 0);
		ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
        MotionRole* motionRole = dynamic_cast<MotionRole*>(aRole);
		ASSERT(motionRole);

		motionRole->StateUpdated(MotionState(MotionState::kResumeMotion, motionName, kTime0, kTime0), currentTime);
    }
#if 0
    else
	{
		if(mDADeformableObject)
		{
			DeformableObjectResumeMotion(mDADeformableObject);
		}
    }
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Stop(const ROSString& motionName)
{
	if(!Performing())
    {   
		Time		currentTime = GetCurrentTimePoint();
		const int	motionRoleIndex = GetMotionRoleIndex();
		ASSERT(motionRoleIndex >= 0);
		ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
        MotionRole* motionRole = dynamic_cast<MotionRole*>(aRole);
		ASSERT(motionRole);

		motionRole->StateUpdated(MotionState(MotionState::kStopMotion, motionName, kTime0, Time(0)), currentTime);
    }
#if 0
    else
	{
		if(mDADeformableObject)
		{
	        DeformableObjectStopMotion(mDADeformableObject);
		}
    }
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN long DeformableSceneEntity::GetRootEngineIndex() const
{
	if(mDADeformableObject)
	{
	    return DeformableObjectGetRoot(mDADeformableObject);
	}
	else
	{
		// *** NOTE: This should really be including the DA engine.h header file and return
		// *** INVALID_ENGINE_INDEX. It is defined as -1, so we will return that here.
		return (long) -1;
	}
}

// --------------------------------------------------------------------------
CPP_DEFN unsigned int DeformableSceneEntity::GetMotionCount() const
{
	if(mDADeformableObject)
	{
	    return DeformableObjectGetMotionCount(mDADeformableObject);
	}
	else
	{
		return 0;
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString DeformableSceneEntity::GetMotionName(int motionIdx) const
{
	ASSERT(mDADeformableObject);	// The number of motions is 0

    return DeformableObjectGetMotionName(mDADeformableObject, motionIdx);
}
// --------------------------------------------------------------------------
CPP_DEFN Time DeformableSceneEntity::GetMotionLength(const ROSString& motionName) const
{
	ASSERT(mDADeformableObject);	// The number of motions is 0

    return Time(DeformableObjectGetMotionLength(mDADeformableObject, motionName));
}
// --------------------------------------------------------------------------
CPP_DEFN Time DeformableSceneEntity::GetCurrentMotionTime() const
{
	ASSERT(mDADeformableObject);	// The number of motions is 0

#ifdef NEW_ANIM_LIB_COMPLETE
    return Time(DeformableObjectGetCurrentMotionTime(mDADeformableObject));
#else
    return kTime0;
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::StartIK(const ROSString& endEffectorName, unsigned int countToRootEffector, AStaticSceneEntity& targetEntity, Time transition)
{
	if(!Performing())
    {
		// Update the role
		Time		currentTime = GetCurrentTimePoint();
		const int	motionRoleIndex = GetMotionRoleIndex();
		ASSERT(motionRoleIndex >= 0);
		ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
        MotionRole* motionRole = dynamic_cast<MotionRole*>(aRole);
		ASSERT(motionRole);

		const ROSString motName = endEffectorName;
		MotionState	state(MotionState::kStartIK, motName, kTime0, transition);

		state.SetIKRecordIndex(mIKRecords.size());

		motionRole->StateUpdated(state, currentTime);

#ifdef STOP_IK_KEY_POINT
		MotionState	stopState(MotionState::kStopIK, motName, kTime0, kTime0);
		stopState.SetIKRecordIndex(mIKRecords.size());

		motionRole->StateUpdated(stopState, currentTime + Time(5));
#endif

		// Update the IK records
		IKRecord	iKRecord(endEffectorName, countToRootEffector, targetEntity);

		mIKRecords.push_back(iKRecord);

		targetEntity.GetSceneEntityStateAccessor()->AddListener(*this);
		GetSceneEntityState().AddSource(targetEntity);
    }
#if 0
    else
    {    
		if(mDADeformableObject)
		{
			DeformableObjectStartMotion(mDADeformableObject, motionName, false, 0.0, transition.GetTime());
		}
    }
#endif
}
// --------------------------------------------------------------------------
IKState DeformableSceneEntity::GetIKState(Time startTime) const
{
	const int			motionRoleIndex = GetMotionRoleIndex();
	ASSERT(motionRoleIndex >= 0);
	const ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
    const MotionRole*	motionRole = dynamic_cast<const MotionRole*>(aRole);
	ASSERT(motionRole);

	const MotionState	state = motionRole->GetState(startTime);

	ASSERT(state.GetMotionEvent() == MotionState::kStartIK);

	const unsigned int	iKRecordIndex = state.GetIKRecordIndex();

	return mIKRecords[iKRecordIndex].mIKState;
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::SetIKState(const IKState& iKState, Time startTime)
{
	const int			motionRoleIndex = GetMotionRoleIndex();
	ASSERT(motionRoleIndex >= 0);
	const ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
    const MotionRole*	motionRole = dynamic_cast<const MotionRole*>(aRole);
	ASSERT(motionRole);

	const MotionState	state = motionRole->GetState(startTime);

	ASSERT(state.GetMotionEvent() == MotionState::kStartIK);

	const unsigned int	iKRecordIndex = state.GetIKRecordIndex();

	mIKRecords[iKRecordIndex].mIKState = iKState;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::RemoveIKRecord(unsigned int iKRecordIndex)
{
	// If none of the motion role states are using this iKRecord, then remove it
	const int	motionRoleIndex = GetMotionRoleIndex();
	ASSERT(motionRoleIndex >= 0);
	ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
    MotionRole*	motionRole = dynamic_cast<MotionRole*>(aRole);
	ASSERT(motionRole);

	const unsigned int timePointCount = motionRole->CountTimePoints();

	for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		const MotionState	state = motionRole->GetState(timePointIdx);

		if(state.GetMotionEvent() == MotionState::kStartIK || state.GetMotionEvent() == MotionState::kStopIK)
		{
			if(state.GetIKRecordIndex() == iKRecordIndex)
			{
				break;
			}
		}
	}

	if(timePointIdx == timePointCount)
	{
		// No state uses the IK record index. Remove the IK record. But first decrement all higher indices in the motion states
		for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
		{
			MotionState	state = motionRole->GetState(timePointIdx);

			if(state.GetMotionEvent() == MotionState::kStartIK || state.GetMotionEvent() == MotionState::kStopIK)
			{
				if(state.GetIKRecordIndex() > iKRecordIndex)
				{
					state.SetIKRecordIndex(state.GetIKRecordIndex() - 1);

					motionRole->StateUpdated(state, timePointIdx);
				}
			}
		}

		// Now remove the IK record. Make sure that the IK is stopped in case it is currently playing
		if(Performing())
		{
			const DAIK*	dAIK = mIKRecords[iKRecordIndex].mDAIK;

			if(dAIK != NULL)
			{
				DeformableObjectStopIK(mDADeformableObject, dAIK);

				mIKRecords[iKRecordIndex].mDAIK = NULL;
			}
		}

		// Finally remove the record
		IKRecordCollection::iterator	iter = mIKRecords.begin() + iKRecordIndex;

		mIKRecords.erase(iter);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetTimeTags(const ROSString& motionName, const TimeTagList& timeTagList)
{
#if 1
    // Temporary solution so that the DeformableWithTimeTags class can be tested
    ROSString timeTagFilename(mSceneEntityState.GetName());

    timeTagFilename += "_";

    timeTagFilename += motionName;

    timeTagFilename += ".ttl";

	std::ofstream    timeTagFile(timeTagFilename.c_str(), std::ios::out|std::ios::trunc);

	OStreamWiz<DeformableSceneEntity_cpp::TimeTagListFieldID>	oWiz(timeTagFile);

	oWiz.Put(DeformableSceneEntity_cpp::kTimeTagList, timeTagList, "DeformableSceneEntity.timeTagList");
#endif
}
/*// --------------------------------------------------------------------------
CPP_DEFN Location DeformableSceneEntity::GetDAObjectLocation() const
{
	ASSERT(mDADeformableObject);

    Vector	location;

    DeformableObjectGetPosition(mDADeformableObject, location);

    return Location(location);
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation DeformableSceneEntity::GetDAObjectOrientation() const
{
	ASSERT(mDADeformableObject);

    Orientation orientation;

    DeformableObjectGetOrientation(mDADeformableObject, orientation);

    return orientation;
}
*/
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetDAObjectLocation(const Location& location)
{
	ASSERT(mDADeformableObject);

    DeformableObjectSetPosition(mDADeformableObject, location.GetVector());
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetDAObjectOrientation(const Orientation& orient)
{
	ASSERT(mDADeformableObject);

    DeformableObjectSetOrientation(mDADeformableObject, orient);
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Goto(Time time)
{
#if 0
	if(time <= mCurrentStateTime)
	{
		// We've looped back!
		mCurrentStateTime -= mSceneEntityState.GetScene().GetDuration();

		if(mCurrentStateTime == kTime0)
		{
			// Don't want to skip events at time 0
			mCurrentStateTime = Time(-0.001);
		}
	}
#endif

	ACompoundSceneEntityBaseClass::Goto(time);
	AAudibleSceneEntityBaseClass::Goto(time);

#if 0
	mCurrentStateTime = time;
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::GotoForLocationRole(Time time)
{
	const int		locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    ARole*			aRole = &mSceneEntityState.GetRole(locationRoleIndex);
	LocationRole*	lRole = dynamic_cast<LocationRole*>(aRole);
	ASSERT(lRole);

	// Let's find the time point in the role that is at the specified time or
	// is closest to it and before it.
	Time	nearestPreviousTime(0);

	const bool	timePointFound = lRole->GetNearestPreviousOrEqualTime(time, nearestPreviousTime);

	Location	location = lRole->GetState(nearestPreviousTime);

    if(Performing())
    {   
		if(time == kTime0)
        {   
			InitLocationAppliedFlags();
        }
    }
    else
    {   
		// Not performing
        InitLocationAppliedFlags();
    }

    if(IsLocationAppliedFlagSet(nearestPreviousTime))
    {
		return;
    }
    else
    {   
		SetLocationAppliedFlag(nearestPreviousTime, true);
    }

	if(mDADeformableObject)
	{
		Vector	objLocation;

		DeformableObjectGetPosition(mDADeformableObject, objLocation);
		
		if(location.GetX() != objLocation.x || location.GetZ() != objLocation.z)
		{
			location.SetY(objLocation.y);

			DeformableObjectSetPositionOnly(mDADeformableObject, location.GetVector());
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::GotoForOrientationRole(Time time)
{
	const int			orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
    ARole*				aRole = &mSceneEntityState.GetRole(orientationRoleIndex);
	OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
	ASSERT(oRole);

	// Let's find the time point in the role that is at the specified time or
	// is closest to it and before it.
	Time	nearestPreviousTime(0);

	const bool	timePointFound = oRole->GetNearestPreviousOrEqualTime(time, nearestPreviousTime);

	const Orientation	orientation = oRole->GetState(nearestPreviousTime);

    if(Performing())
    {   
		if(time == kTime0)
        {   
			InitOrientationAppliedFlags();
        }
    }
    else
    {   
		// Not performing
        InitOrientationAppliedFlags();
    }

    if(IsOrientationAppliedFlagSet(nearestPreviousTime))
    {
		return;
    }
    else
    {   
		SetOrientationAppliedFlag(nearestPreviousTime, true);
    }

	if(mDADeformableObject)
	{
		SetDAObjectOrientation(orientation);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::GotoForMotionRole(Time time)
{
	const int	motionRoleIndex = GetMotionRoleIndex();
	ASSERT(motionRoleIndex >= 0);
	ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
	MotionRole*	motionRole = dynamic_cast<MotionRole*>(aRole);
	ASSERT(motionRole);

	Time		lastKeyTime(0);
	const bool	hasEvent = motionRole->GetNearestPreviousOrEqualTime(time, lastKeyTime);

	if(!hasEvent || time == kTime0)
    {
		InitMotionPlayingFlags();

		InitIKRecords();

		if(mDADeformableObject)
		{
			DeformableObjectStopMotion(mDADeformableObject);
		}

		if(!hasEvent)
		{
			return;
		}
    }

	// We have at least one event to process

	if(Performing())
	{
		unsigned int	firstStateIndex;

		if(mIsMotionActuallyPlaying.empty())
		{
			firstStateIndex = 0;
		}
		else
		{
			const Time	greatestTime = GetGreatestMotionPlayingFlagTime();

			firstStateIndex = motionRole->GetIndex(greatestTime) + 1;
		}

		const unsigned int	lastStateIndex = motionRole->GetIndex(lastKeyTime);

		for(unsigned int idx = firstStateIndex; idx <= lastStateIndex; ++idx)
		{
			const Time							motionTime = motionRole->GetTime(idx);
			const MotionState					motionState = motionRole->GetState(idx);
			const MotionState::MotionEventID	motionEvent = motionState.GetMotionEvent();
			const ROSString						motionName = motionState.GetMotionName();
			const Time							motionStart = motionState.GetStartTime();
			Time								motionTransition = motionState.GetTransitionTime();
			const DAIK*							dAIK;
			unsigned int						iKRecordIndex;
		
			switch(motionEvent)
			{
				case MotionState::kStartMotion:
					SetMotionActuallyPlayingFlag(motionTime, true);

					if(mDADeformableObject)
					{
						if(motionTime == kTime0 && !mUseInitialTransition && mSceneEntityState.GetScene().IsFirstIteration())
						{
							motionTransition = kTime0;
						}

						const Time	delay = time - motionTime;
						const Time	transition = delay < motionTransition ? motionTransition - delay : kTime0;
						
						DeformableObjectStartMotion(mDADeformableObject, motionName, false, (motionStart + delay).GetTime(), transition.GetTime());
						DeformableObjectUpdate(mDADeformableObject);

#if 0
//OutputDebugString("Starting motion: ");
//OutputDebugString(motionName.c_str());
//OutputDebugString("\n");
#endif
					}

					break;

				case MotionState::kLoopMotion:
					SetMotionActuallyPlayingFlag(motionTime, true);

					if(mDADeformableObject)
					{
						if(motionTime == kTime0 && !mUseInitialTransition && mSceneEntityState.GetScene().IsFirstIteration())
						{
							motionTransition = kTime0;
						}

						const Time	delay = time - motionTime;
						const Time	transition = delay < motionTransition ? motionTransition - delay : kTime0;
						
						DeformableObjectStartMotion(mDADeformableObject, motionName, true, (motionStart + delay).GetTime(), transition.GetTime());
						DeformableObjectUpdate(mDADeformableObject);

#if 0
//OutputDebugString("Starting loop: ");
//OutputDebugString(motionName.c_str());
//OutputDebugString("\n");
#endif
					}

					break;

				case MotionState::kPauseMotion:
					if(mDADeformableObject)
					{
						DeformableObjectPauseMotion(mDADeformableObject);
						DeformableObjectUpdate(mDADeformableObject);
					}

					SetMotionActuallyPlayingFlag(motionTime, false);

					break;

				case MotionState::kResumeMotion:
					if(mDADeformableObject)
					{
						DeformableObjectResumeMotion(mDADeformableObject);
						DeformableObjectUpdate(mDADeformableObject);
					}

					SetMotionActuallyPlayingFlag(motionTime, true);
					
					break;

				case MotionState::kStopMotion:
					if(mDADeformableObject)
					{
						DeformableObjectStopMotion(mDADeformableObject);
						DeformableObjectUpdate(mDADeformableObject);
					}

					SetMotionActuallyPlayingFlag(motionTime, false);

					break;

				case MotionState::kStartIK:
					SetMotionActuallyPlayingFlag(motionTime, true);

					if(mDADeformableObject)
					{
						if(motionTime == kTime0 && !mUseInitialTransition && mSceneEntityState.GetScene().IsFirstIteration())
						{
							motionTransition = kTime0;
						}

						const Time	delay = time - motionTime;
						const Time	transition = delay < motionTransition ? motionTransition - delay : kTime0;

						iKRecordIndex = motionState.GetIKRecordIndex();

						// Before starting the IK, ensure that the initial position is valid.
						// *** This arrives from the fact that the IK relationships are updated after the call
						// *** to DeformableObjectUpdate below.
						mIKRecords[iKRecordIndex].UpdateVectorToTarget();
						Vector endPos;
						DeformableObjectGetEndEffectorPosition (mDADeformableObject, mIKRecords[iKRecordIndex].mIKState, endPos);
						mIKRecords[iKRecordIndex].UpdateOrientToTarget(endPos);

						dAIK =
							DeformableObjectStartIK
							(
								mDADeformableObject,
								mIKRecords[iKRecordIndex].mIKState,
								mIKRecords[iKRecordIndex].mVectorToTarget,
								mIKRecords[iKRecordIndex].mOrientToTarget,
								transition.GetTime()
							);

						mIKRecords[iKRecordIndex].mDAIK = dAIK;

						DeformableObjectUpdate(mDADeformableObject);
#if 0
//OutputDebugString("Starting IK\n");
#endif
					}

					break;

				case MotionState::kStopIK:
					SetMotionActuallyPlayingFlag(motionTime, true);

					if(mDADeformableObject)
					{
						iKRecordIndex = motionState.GetIKRecordIndex();

						DeformableObjectStopIK(mDADeformableObject, mIKRecords[iKRecordIndex].mDAIK);

						mIKRecords[iKRecordIndex].mDAIK = NULL;

						DeformableObjectUpdate(mDADeformableObject);
#if 0
//OutputDebugString("Starting IK\n");
#endif
					}

					break;

				case MotionState::kInternalMotion:
					// Don't do anything!
					break;
				default:
					ASSERT(0);	// Unhandled case
					return;
			}
		}

		// Update all active IK relationships
		const unsigned int	iKCount = mIKRecords.size();

		for(unsigned int iKIndex = 0; iKIndex < iKCount; ++iKIndex)
		{
			if(mIKRecords[iKIndex].mDAIK != NULL)
			{
				// Update target information
				mIKRecords[iKIndex].UpdateVectorToTarget();

				// Find the current position of the end effector.
				Vector endPos;
				DeformableObjectGetEndEffectorPosition (mDADeformableObject, mIKRecords[iKIndex].mIKState, endPos);
				mIKRecords[iKIndex].UpdateOrientToTarget(endPos);
			}
		}
    }
    else
    {   // Not performing
        InitMotionPlayingFlags();
		if(mDADeformableObject)
		{
			DeformableObjectStopMotion(mDADeformableObject);
		}

#if 0
		// Create a scrub effect!
		const unsigned int timePointCount = motionRole->CountTimePoints();

		for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
		{
			const Time	lastKeyTime = motionRole->GetTime(timePointIdx);

			if(lastKeyTime > time)
			{
				// Nothing more to do!
				return;
			}

			const MotionState					motionState = motionRole->GetState(timePointIdx);
			const MotionState::MotionEventID	motionEvent = motionState.GetMotionEvent();
			const const ROSString				motionName = motionState.GetMotionName();
			const Time							motionTransition = motionState.GetTransitionTime();

			float	durationForEvent;

			if((timePointIdx + 1) < timePointCount)
			{
				// There is another marker after this one
				const Time	nextMotionTime = motionRole->GetTime(timePointIdx + 1);

				if(nextMotionTime <= time)
				{
					// We will include the next marker in the scrub
					durationForEvent = (nextMotionTime - lastKeyTime).GetTime();
				}
				else
				{
					// The next marker will not be in the scrub
					durationForEvent = (time - lastKeyTime).GetTime();
				}
			}
			else
			{
				// We are at the last marker before or at the specified time
				durationForEvent = (time - lastKeyTime).GetTime();
			}

			switch(motionEvent)
			{
				case MotionState::kStartMotion:
					SetMotionActuallyPlayingFlag(lastKeyTime, true);
					
					if(mDADeformableObject)
					{
						DeformableObjectStartMotion(mDADeformableObject, motionName, false, durationForEvent, motionTransition.GetTime());
						DeformableObjectUpdate(mDADeformableObject);
					}
					break;

				case MotionState::kLoopMotion:
					SetMotionActuallyPlayingFlag(lastKeyTime, true);
					
					if(mDADeformableObject)
					{
						DeformableObjectStartMotion(mDADeformableObject, motionName, true, durationForEvent, motionTransition.GetTime());
						DeformableObjectUpdate(mDADeformableObject);
					}
					break;

				case MotionState::kPauseMotion:
					
					if(mDADeformableObject)
					{
						DeformableObjectPauseMotion(mDADeformableObject);
						DeformableObjectUpdate(mDADeformableObject);
					}

					SetMotionActuallyPlayingFlag(lastKeyTime, false);
					break;

				case MotionState::kResumeMotion:
					if(mDADeformableObject)
					{
						DeformableObjectResumeMotion(mDADeformableObject);
						DeformableObjectUpdate(mDADeformableObject);
					}

					SetMotionActuallyPlayingFlag(lastKeyTime, true);
					break;

				case MotionState::kStopMotion:
					if(mDADeformableObject)
					{
						DeformableObjectStopMotion(mDADeformableObject);
						DeformableObjectUpdate(mDADeformableObject);
					}

					SetMotionActuallyPlayingFlag(lastKeyTime, false);
					break;

				case MotionState::kStartIK:
					break;

				case MotionState::kStopIK:
					break;

				case MotionState::kInternalMotion:
					// Don't do anything!
					break;

				default:
					ASSERT(0);	// Unhandled case
					return;
			}
		}
#endif
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::InitializeLocationRole(bool useStateInScript)
{
	const int		locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    ARole*			aRole = &mSceneEntityState.GetRole(locationRoleIndex);

    aRole->SetName("Location");

    LocationRole*   lRole = dynamic_cast<LocationRole*>(aRole);
    ASSERT(lRole);

    if(!(mSceneEntityState.GetScene().IsUsingInitialEntityState() || useStateInScript))
    {   
		Location	location = mState->GetPosition().GetLocation();

        lRole->StateUpdated(FlaggedLocation(location, lRole->GetState(kTime0).GetInterpolationType()), kTime0);
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::InitializeOrientationRole(bool useStateInScript)
{
	const int	orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(orientationRoleIndex);

    aRole->SetName("Orientation");

    OrientationRole*   oRole = dynamic_cast<OrientationRole*>(aRole);
    ASSERT(oRole);

    if(!(mSceneEntityState.GetScene().IsUsingInitialEntityState() || useStateInScript))
    {   
		const FlaggedOrientation	currentOrientation = oRole->GetState(kTime0);

		oRole->StateUpdated(FlaggedOrientation(mState->GetPosition().GetOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), kTime0);
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Write(std::ostream& oStream) const
{
	ACompoundSceneEntityBaseClass::Write(oStream);
	AAudibleSceneEntityBaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::Read(std::istream& iStream)
{
	ACompoundSceneEntityBaseClass::Read(iStream);
	AAudibleSceneEntityBaseClass::Read(iStream);

	ReadSubObject(iStream);

	mSceneEntityState.SetVisible(true);	// Force this entity visible

	mSceneEntityState.GetScene().UpdateDescriptionStrings(mSceneEntityState.GetName(), Scene::kDeformable, mCategoryName, mDescriptionStrings);

	SceneEventFlag	flag = kSEFNone;

	SetupDADeformableObject(flag);

	const bool	useStateInScript = (flag & kSEFUseStateInScriptAsInitialState) != 0;
	const bool	useFloorHeightInScript = (flag & kSEFUseFloorHeightInScript) != 0;

	mUseInitialTransition = (flag & kSEFUseTransitionInScriptTheFirstTime) != 0;

	if(useFloorHeightInScript || !mUseInitialTransition)
	{	
		// The floor height has to be figured out, or the deformable has to be instantaneously set to the initial posture
		if(mDADeformableObject)
		{
			// Set position, floor height, start motion and update

			// Set position and floor height
			const int		locationRoleIndex = GetLocationRoleIndex();
			ASSERT(locationRoleIndex >= 0);
			ARole*			aRole = &mSceneEntityState.GetRole(locationRoleIndex);
			LocationRole*   lRole = dynamic_cast<LocationRole*>(aRole);
			ASSERT(lRole);

			const Location	location = lRole->GetState(kTime0);

			DeformableObjectSetPositionViaEngine(mDADeformableObject, location.GetVector());

			// Start motion
			const int	motionRoleIndex = GetMotionRoleIndex();
			ASSERT(motionRoleIndex >= 0);
			
			aRole = &mSceneEntityState.GetRole(motionRoleIndex);

			MotionRole*	mRole = dynamic_cast<MotionRole*>(aRole);
			MotionState	state = mRole->GetState(kTime0);
			ROSString	motionName = state.GetMotionName();

			/*****FL GAMESTOCK HACK***********/
			if(motionName == "Sc_Nomotion" || motionName == "Sc_nomotion")
			{
				state = mRole->GetState(1);
				motionName = state.GetMotionName();
			}

			const bool	loop = state.GetMotionEvent() == MotionState::kLoopMotion;

			DeformableObjectStartMotion(mDADeformableObject, motionName, loop, 0, 0);

			// Update!
			DeformableObjectUpdate(mDADeformableObject);

			// Finally, stop the motion
			DeformableObjectStopMotion(mDADeformableObject);
		}
	}

	InitLocationAppliedFlags();
	InitOrientationAppliedFlags();
	InitMotionPlayingFlags();

	InitializeLocationRole(useStateInScript);
	InitializeOrientationRole(useStateInScript);
    InitializeMotionRole(useStateInScript);
    InitializeAudioRole(useStateInScript);
    InitializeParentRole(useStateInScript);

#if 0
	{
		//OutputDebugString("DeformableSceneEntity read!\n");
		unsigned int	count = GetMotionCount();
		char			c[20];
		itoa(count, c, 10);
		//OutputDebugString("Number of motions: ");
		//OutputDebugString(c);
		//OutputDebugString("\n");

		if(mDADeformableObject)
		{
			for(int m = 0; m < count; ++m)
			{   
				ROSString   name = DeformableObjectGetMotionName(mDADeformableObject, m);
				//OutputDebugString(name.c_str());
				//OutputDebugString("\n");
			}
		}
	}
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	ROS::Time timeZero(0);
	char buffer[1024 * 20];
	int j;
	// write the lua enity table for this entity
	j = sprintf(buffer,			"\n");
	j+= sprintf(buffer + j,		"%s = \n{\n", mSceneEntityState.GetName().c_str());
	j+= sprintf(buffer + j,		"\ttype = DEFORMABLE,\n\tflags = 0,\n");
	j+= sprintf(buffer + j,		"\tspatialprops = \n\t{\n");
	j+= sprintf(buffer + j,		"\t\tpos = {%f, %f, %f},\n", GetLocation(timeZero).GetX(), GetLocation(timeZero).GetY(), GetLocation(timeZero).GetZ());
	j+= sprintf(buffer + j,		"\t\torient = { {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f} }\n",
			GetOrientation(timeZero).GetI().x, GetOrientation(timeZero).GetI().y, GetOrientation(timeZero).GetI().z, 
			GetOrientation(timeZero).GetJ().x, GetOrientation(timeZero).GetJ().y, GetOrientation(timeZero).GetJ().z, 
			GetOrientation(timeZero).GetK().x, GetOrientation(timeZero).GetK().y, GetOrientation(timeZero).GetK().z
		);
	j+= sprintf(buffer + j,		"\t},\n");
	j+= sprintf(buffer + j,		"\tuserprops = \n\t{\n");
	j+= sprintf(buffer + j,		"\t\tcategory = \"%s\"\n", mCategoryName.c_str());
	j+= sprintf(buffer + j,		"\t}\n");
	j+= sprintf(buffer + j,		"},\n");

	// write the orientation animation data
	if (GetOrientationRoleIndex() >= 0)
	{
		ARole*				aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetOrientationRoleIndex()));
		OrientationRole*	role = dynamic_cast<OrientationRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
		}
	}
	// write the location animation data
	if (GetLocationRoleIndex() >= 0)
	{
		ARole*			aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetLocationRoleIndex()));
		LocationRole*	role = dynamic_cast<LocationRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
		}
	}
	// write the motion event data
	if (GetMotionRoleIndex() >= 0)
	{
		ARole*		aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetMotionRoleIndex()));
		MotionRole*	role = dynamic_cast<MotionRole*>(aRole);
		if (role)
		{
			char newbuffer[1024*10];
			int i = 0;
			ROSString stateString;
			ROS::Time time(0);
			const unsigned int timePointCount = role->CountTimePoints();

			if (timePointCount > 0)
			{
				for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
				{
					time = role->GetTime(timePointIdx);
					
					MotionState state = role->GetState(time);
					bool IKMotion = false;
					switch (state.GetMotionEvent())
					{
						case MotionState::kStartMotion:
    						stateString = "START";
    						break;
    					case MotionState::kPauseMotion:
    						stateString = "START";
        					break;
    					case MotionState::kLoopMotion:
    						stateString = "LOOP";
    						break;
						case MotionState::kResumeMotion:
    						stateString = "RESUME";
        					break;
						case MotionState::kStopMotion:
    						stateString = "STOP";
        					break;
						case MotionState::kStartIK:
							{
	   						stateString = "START_IK";
							const IKState * ikstate = &mIKRecords[state.GetIKRecordIndex()].mIKState;
							sprintf(newbuffer + i, "\nEVENT[{%f, START_IK, {\"%s\"}, {end_effector = %s, count_to_root = %d, damping = %f, point_at = &d, move_to = %d}}]",
									time.GetTime(),
									mSceneEntityState.GetName().c_str(),
									ikstate->GetEndEffectorName().c_str(),
									ikstate->GetCountToRootEffector(),
									ikstate->GetDampingFactor(),
//									ikstate->GetEndEffectorAxis().x,ikstate->GetEndEffectorAxis().x,ikstate->GetEndEffectorAxis().x,
//									ikstate->GetEndEffectorAxis().x,ikstate->GetEndEffectorAxis().x,ikstate->GetEndEffectorAxis().x,
									ikstate->GetPointAtFlag(),
									ikstate->GetMoveToFlag()
								);
							IKMotion = true;
							}
							break;
						case MotionState::kStopIK:
							{
    						stateString = "STOP_IK";
							const IKState * ikstate = &mIKRecords[state.GetIKRecordIndex()].mIKState;
							sprintf(newbuffer + i, "\nEVENT[{%f, START_IK, {\"%s\"}, {end_effector = %s, count_to_root = %d, damping = %f, point_at = &d, move_to = %d}}]",
									time.GetTime(),
									mSceneEntityState.GetName().c_str(),
									ikstate->GetEndEffectorName().c_str(),
									ikstate->GetCountToRootEffector(),
									ikstate->GetDampingFactor(),
//									ikstate->GetEndEffectorAxis().x,ikstate->GetEndEffectorAxis().x,ikstate->GetEndEffectorAxis().x,
//									ikstate->GetEndEffectorAxis().x,ikstate->GetEndEffectorAxis().x,ikstate->GetEndEffectorAxis().x,
									ikstate->GetPointAtFlag(),
									ikstate->GetMoveToFlag()
								);
							IKMotion = true;
							}
							break;
					}

					if (!IKMotion)
					{
						// add an event line
						i += sprintf(newbuffer + i, "\nEVENT[{%f, %s, {\"%s\"}, {animation = \"%s\", start_time = %f, trans_time = %f}}]",
										time.GetTime(),
										stateString.c_str(),
										mSceneEntityState.GetName().c_str(),
										state.GetMotionName().c_str(),
										state.GetStartTime().GetTime(),
										state.GetTransitionTime().GetTime()
									);
					}
				}
			}
			j+= sprintf ( buffer + j, newbuffer);
		}
	}
	// write the audio event data
	if (GetAudioRoleIndex() >= 0)
	{
		ARole*				aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetAudioRoleIndex()));
		AudioRole*	role = dynamic_cast<AudioRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
		}
	}

// write out the hardpoints for used by this object
//	for (int i = 0; i < GetHardPointCount(); i++)
//	{
//		j+= sprintf(buffer + j, "hardpoint:%s\n", GetHardPointName(i).c_str());
//		j+= sprintf(buffer + j, "\t pos = {%f, %f, %f}\n", GetHardPointLocation(i).GetX(), GetHardPointLocation(i).GetY(), GetHardPointLocation(i).GetZ());
//		j+= sprintf(buffer + j, "\t orient = { {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f} }\n", 
//			GetHardPointOrientation(i).GetI().x, GetHardPointOrientation(i).GetI().y, GetHardPointOrientation(i).GetI().z, 
//			GetHardPointOrientation(i).GetJ().x, GetHardPointOrientation(i).GetJ().y, GetHardPointOrientation(i).GetJ().z, 
//			GetHardPointOrientation(i).GetK().x, GetHardPointOrientation(i).GetK().y, GetHardPointOrientation(i).GetK().z);
//	}

	//	oStream << mIsMotionActuallyPlaying << endl;    // Don't want to write or read this

	oWiz.Put(kDescriptionStrings, mDescriptionStrings);
	oWiz.Put(kSceneEntityState, mSceneEntityState);
	oWiz.Put(kIKRecords, IKRecordsWriter(mIKRecords));
	oWiz.Put(kCategoryName, mCategoryName, buffer);
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

//	iStream >> mIsMotionActuallyPlaying;    // Don't want to read this -- wasn't written either

	iWiz.Get(kDescriptionStrings, mDescriptionStrings);
	iWiz.Get(kSceneEntityState, mSceneEntityState);

	if(iWiz.Has(kIKRecords))
	{
		iWiz.Get(kIKRecords, IKRecordsReader(mIKRecords));
	}
	else
	{
		mIKRecords.clear();
	}

	if(iWiz.Has(kCategoryName))
	{
		iWiz.Get(kCategoryName, mCategoryName);
	}
	else
	{
		mCategoryName = "<Unknown>";
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString DeformableSceneEntity::GetCurrentMotionName() const
{
    return mCurrentMotionName;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetCurrentMotionName(const ROSString& motionName)
{
    mCurrentMotionName = motionName;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::InitMotionPlayingFlags()
{
    mIsMotionActuallyPlaying.resize(0, kTime0);
}
// --------------------------------------------------------------------------
CPP_DEFN bool DeformableSceneEntity::IsMotionActuallyPlayingFlagSet(Time startTime) const
{
    ASSERT(GetMotionRoleIndex() >= 0 && mSceneEntityState.GetRoleCount() > GetMotionRoleIndex());
    
	MotionPlayingFlags::const_iterator begin = mIsMotionActuallyPlaying.begin();
    const MotionPlayingFlags::const_iterator end = mIsMotionActuallyPlaying.end();

    while(begin != end)
    {   
		if(*begin == startTime)
        {
			return true;
        }
        
		++begin;
    }

    //  The motion name is not in the list!
    return false;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetMotionActuallyPlayingFlag(Time startTime, bool isPlaying)
{
    ASSERT(GetMotionRoleIndex() >= 0 && mSceneEntityState.GetRoleCount() > GetMotionRoleIndex());
    
	MotionPlayingFlags::iterator				begin = mIsMotionActuallyPlaying.begin();
    const MotionPlayingFlags::const_iterator	end = mIsMotionActuallyPlaying.end();

    while(begin != end)
    {   
		if(*begin == startTime)
        {
			if(isPlaying)
            {
				// nothing to do; motion name already in list
            }
            else
            {   
				// motion object has to be removed from the list
                mIsMotionActuallyPlaying.erase(begin);
            }
            
			return;
        }
        
		++begin;
    }

    //  The motion name is not in the list!
    if(isPlaying)
    {   
		mIsMotionActuallyPlaying.push_back(startTime);
    }
}
// --------------------------------------------------------------------------
Time DeformableSceneEntity::GetGreatestMotionPlayingFlagTime() const
{
	ASSERT(!mIsMotionActuallyPlaying.empty());

	MotionPlayingFlags::const_iterator			begin = mIsMotionActuallyPlaying.begin();
    const MotionPlayingFlags::const_iterator	end = mIsMotionActuallyPlaying.end();

	Time	time = *begin;

	++begin;

    while(begin != end)
    {   
		if(*begin > time)
		{
			time = *begin;
		}

		++begin;
	}

	return time;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::InitLocationAppliedFlags()
{
    mIsLocationApplied.clear();
}
// --------------------------------------------------------------------------
CPP_DEFN bool DeformableSceneEntity::IsLocationAppliedFlagSet(Time time) const
{
    LocationAppliedFlags::const_iterator begin = mIsLocationApplied.begin();
    const LocationAppliedFlags::const_iterator end = mIsLocationApplied.end();

    while(begin != end)
    {   
		if(*begin == time)
        {
			return true;
        }

        ++begin;
    }

    //  The location change is not in the list!
    return false;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetLocationAppliedFlag(Time time, bool isApplied)
{
	LocationAppliedFlags::iterator begin = mIsLocationApplied.begin();
    const LocationAppliedFlags::const_iterator end = mIsLocationApplied.end();

    while(begin != end)
    {   
		if(*begin == time)
        {
			if(isApplied)
            {
				// nothing to do; location change already in list
            }
            else
            {   
				// location change has to be removed from the list
                mIsLocationApplied.erase(begin);
            }

            return;
        }
        ++begin;
    }

    //  The location change is not in the list!
    if(isApplied)
    {   
		mIsLocationApplied.push_back(time);
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::InitOrientationAppliedFlags()
{
    mIsOrientationApplied.clear();
}
// --------------------------------------------------------------------------
CPP_DEFN bool DeformableSceneEntity::IsOrientationAppliedFlagSet(Time time) const
{
    OrientationAppliedFlags::const_iterator begin = mIsOrientationApplied.begin();
    const OrientationAppliedFlags::const_iterator end = mIsOrientationApplied.end();

    while(begin != end)
    {   
		if(*begin == time)
        {
			return true;
        }

        ++begin;
    }

    //  The orientation change is not in the list!
    return false;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetOrientationAppliedFlag(Time time, bool isApplied)
{
	OrientationAppliedFlags::iterator begin = mIsOrientationApplied.begin();
    const OrientationAppliedFlags::const_iterator end = mIsOrientationApplied.end();

    while(begin != end)
    {   
		if(*begin == time)
        {
			if(isApplied)
            {
				// nothing to do; orientation change already in list
            }
            else
            {   
				// orientation change has to be removed from the list
                mIsOrientationApplied.erase(begin);
            }

            return;
        }
        ++begin;
    }

    //  The orientation change is not in the list!
    if(isApplied)
    {   
		mIsOrientationApplied.push_back(time);
    }
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::InitIKRecords()
{
	IKRecordCollection::iterator				begin = mIKRecords.begin();
	const IKRecordCollection::const_iterator	end = mIKRecords.end();

	while(begin != end)
	{
		if(begin->mDAIK)
		{
			begin->mDAIK = NULL;
		}

		++begin;
	}
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::StateUpdated(Update::ID id, Time time)
{
	ACompoundSceneEntityBaseClass::StateUpdated(id, time);
	AAudibleSceneEntityBaseClass::StateUpdated(id, time);
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::LocationStateUpdated(Time time)
{
	const int		locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
	ARole*			aRole = &mSceneEntityState.GetRole(locationRoleIndex);
    LocationRole*   lRole = dynamic_cast<LocationRole*>(aRole);
	ASSERT(lRole);

	const bool	existingKeyPoint = lRole->HasTime(time);

    lRole->StateUpdated(FlaggedLocation(mState->GetPosition().GetLocation(), lRole->GetState(time).GetInterpolationType()), time);

	if(!existingKeyPoint)
	{
		UpdateMotionPathMarkers();
	}
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::OrientationStateUpdated(Time time)
{
	const int			orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
	ARole*				aRole = &mSceneEntityState.GetRole(orientationRoleIndex);
    OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
	ASSERT(oRole);

	const bool	existingKeyPoint = oRole->HasTime(time);

	const FlaggedOrientation	currentOrientation = oRole->GetState(time);

	oRole->StateUpdated(FlaggedOrientation(mState->GetPosition().GetOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), time);

	if(!existingKeyPoint)
	{
		UpdateMotionPathMarkers();
	}
}
// --------------------------------------------------------------------------
CPP_DEFN Time DeformableSceneEntity::GetTransitionTime(Time startTime) const
{
	const int		motionRoleIndex = GetMotionRoleIndex();
	ASSERT(motionRoleIndex >= 0);
    const ARole*	aRole = &mSceneEntityState.GetRole(motionRoleIndex);

    if(aRole->CountTimePoints() > 0)
    {   
		const MotionRole*   motionRole = dynamic_cast<const MotionRole*>(aRole);

        if(motionRole)
        {
			MotionState	motionState = motionRole->GetState(startTime);
        	return motionState.GetTransitionTime();
        }
    }

    ASSERT(0);	// We have been provided a non-existant key point start time
    
    return kTime0;
}
// --------------------------------------------------------------------------
CPP_DEFN void DeformableSceneEntity::SetTransitionTime(Time startTime, Time transitionTime)
{
	const int	motionRoleIndex = GetMotionRoleIndex();
	ASSERT(motionRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);

    if(aRole->CountTimePoints() > 0)
    {   
		MotionRole*   motionRole = dynamic_cast<MotionRole*>(aRole);

        if(motionRole)
        {   
			MotionState	motionState = motionRole->GetState(startTime);

        	motionState.SetTransitionTime(transitionTime);

            motionRole->StateUpdated(motionState, startTime);
        }
    }
	else
	{	
		ASSERT(0);	// We have been provided a non-existant key point start time
	}
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::Respond(const SceneEntityEvent& event)
{
    ACompoundSceneEntityBaseClass::Respond(event);
	AAudibleSceneEntityBaseClass::Respond(event);

	if(event.GetID() == SceneEntityEvent::kSourceEntityDeleted)
	{
		ASceneEntity*		entity = &event.GetSourceEntity();
		AStaticSceneEntity*	aStaticSE = dynamic_cast<AStaticSceneEntity*>(entity);

		if(aStaticSE)
		{
			const int		motionRoleIndex = GetMotionRoleIndex();
			ASSERT(motionRoleIndex >= 0);
			ARole*			aRole = &GetSceneEntityState().GetRole(motionRoleIndex);
			MotionRole*		motRole = dynamic_cast<MotionRole*>(aRole);
			ASSERT(motRole);

			const unsigned int	originalTimePointCount = motRole->CountTimePoints();
			unsigned int		timePointCount = originalTimePointCount;
			unsigned int		idx = 0;

			while(idx < timePointCount)
			{
				const MotionState	state = motRole->GetState(idx);

				if(state.GetMotionEvent() == MotionState::kStartIK || state.GetMotionEvent() == MotionState::kStopIK)
				{
					if(&(mIKRecords[state.GetIKRecordIndex()].mIKState.GetTargetEntity()) == aStaticSE)
					{
						motRole->Remove(idx);

						--timePointCount;

						continue;	// Skip ++idx below
					}
				}

				++idx;
			}

			if(originalTimePointCount != motRole->CountTimePoints())
			{
				RoleUpdated();
			}
		}
	}
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::AttachHardPointToParent(unsigned int childHardPointIndex, const HardPointHost* parentHardPointHost, const ROSString& parentHardPointName)
{
	DeformableObjectAttachHardPointToParent(mDADeformableObject, childHardPointIndex, parentHardPointHost, parentHardPointName);
}
// --------------------------------------------------------------------------
void DeformableSceneEntity::DetachHardPointFromParent(unsigned int childHardPointIndex, const HardPointHost* parentHardPointHost, const ROSString& parentHardPointName)
{
	DeformableObjectDetachHardPointFromParent(mDADeformableObject, childHardPointIndex, parentHardPointHost, parentHardPointName);
}
// --------------------------------------------------------------------------
SceneEntityState& DeformableSceneEntity::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& DeformableSceneEntity::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& DeformableSceneEntity::GetPhysicalState()
{
	return *mState;
}
// --------------------------------------------------------------------------
const APhysicalState& DeformableSceneEntity::GetPhysicalState() const
{
	return *mState;
}
// --------------------------------------------------------------------------
int DeformableSceneEntity::GetLocationRoleIndex() const
{
	return mLocationRoleIndex;
}
// --------------------------------------------------------------------------
int DeformableSceneEntity::GetOrientationRoleIndex() const
{
	return mOrientationRoleIndex;
}
// --------------------------------------------------------------------------
int DeformableSceneEntity::GetMotionRoleIndex() const
{
	return mMotionRoleIndex;
}
// --------------------------------------------------------------------------
int DeformableSceneEntity::GetAudioRoleIndex() const
{
	return mAudioRoleIndex;
}
// --------------------------------------------------------------------------
int DeformableSceneEntity::GetParentRoleIndex() const
{
	return mParentRoleIndex;
}
// --------------------------------------------------------------------------
DeformableSceneEntity::AudioPlayingFlags& DeformableSceneEntity::GetAudioPlayingFlags()
{
	return mIsAudioActuallyPlaying;
}
// --------------------------------------------------------------------------
const DeformableSceneEntity::AudioPlayingFlags& DeformableSceneEntity::GetAudioPlayingFlags() const
{
	return mIsAudioActuallyPlaying;
}
// --------------------------------------------------------------------------
DeformableSceneEntity::ParentEventFlags& DeformableSceneEntity::GetParentEventFlags()
{
	return mIsParentEventHandled;
}
// --------------------------------------------------------------------------
const DeformableSceneEntity::ParentEventFlags& DeformableSceneEntity::GetParentEventFlags() const
{
	return mIsParentEventHandled;
}
// --------------------------------------------------------------------------
const IKState &DeformableSceneEntity::GetIKState (unsigned int ikIndex)
{
	try 
	{
		return mIKRecords[ikIndex].mIKState;
	}
	catch (...)
	{
		assert (false && "Invalid IK record index");
		return mIKRecords[0].mIKState;
	}
}
// --------------------------------------------------------------------------
}
