//---------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include <iostream>
#include <memory>
#include "GLUtils.h"
#include "CompoundSceneEntity.h"
#include "CompoundEntityStaticsState.h"
#include "StaticsState.h"
#include "ConstCompoundStateAccessor.h"
#include "ConstMotionStateAccessor.h"
#include "CompoundStateAccessor.h"
#include "MatrixUtil.h"
#include "TimeTag.h"
#include "Utils.h"
#include "DACompoundObject.h"
#include "EventIterator.h"
#include "Role.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "AudioRole.h"
#include "Scene.h"
#include "MotionRole.h"
#include "DAHardPoints.h"
#include "HardPoint.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kDescriptionStrings,
	kSceneEntityState,
	kCategoryName		// Added in ROS Version 2.9.1.0
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// MotionRoleCallback
// --------------------------------------------------------------------------
class MotionRoleCallback: public MotionRole::UpdateCallback
{
	public:
		MotionRoleCallback(CompoundSceneEntity& compoundSE)
		: mCompoundSE(compoundSE), mLastTimeRemoved(kTime0)
		{
		}

		virtual void ChangeTimeFinished(const MotionRole::UpdateCallback::RoleType& role, Time currentTime, Time newTime) 
		{
			ChangeTime(currentTime, newTime);
		}
		
		virtual void RemoveStarted(const MotionRole::UpdateCallback::RoleType& role, Time time) 
		{
			mLastTimeRemoved = time;
		}

		virtual void RemoveFinished(const MotionRole::UpdateCallback::RoleType& role, Time time) 
		{
			RemoveMotion();
		}
		
		virtual void RemoveStarted(const MotionRole::UpdateCallback::RoleType& role, unsigned int timePointIndex) 
		{
			mLastTimeRemoved = role.GetTime(timePointIndex);
		}
		
		virtual void RemoveFinished(const MotionRole::UpdateCallback::RoleType& role, unsigned int timePointIndex) 
		{
			RemoveMotion();
		}

	private:
		void RemoveMotion()
		{
			mCompoundSE.RemoveMotion(mLastTimeRemoved);
		}

		void ChangeTime(Time currentTime, Time newTime)
		{
			mCompoundSE.ChangeMotionTime(currentTime, newTime);
		}

		CompoundSceneEntity&	mCompoundSE;
		Time					mLastTimeRemoved;
};
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
// --------------------------------------------------------------------------
CPP_DEFN CompoundSceneEntity::CompoundSceneEntity(const ROSString& entityName, const ROS::ROSString& categoryName, const StringList& descriptionStrings, Scene& scene)
: mSceneEntityState(*this, entityName, false), mDACompoundObject(NULL)
, mState(NULL), mDAHardPoints(NULL)
, mShowHardPoints(false), mCategoryName(categoryName), mDescriptionStrings(descriptionStrings)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mMotionRoleIndex = mSceneEntityState.AddRole(*new MotionRole(false, Interpolate, Interpolate, GetStateName, new MotionRoleCallback(*this)));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));
	mParentRoleIndex = mSceneEntityState.AddRole(*new ParentRole(false, Interpolate, Interpolate, GetStateName, new ParentRoleCallback(*this)));
   	
	mSceneEntityState.SetScene(&scene);

	SceneEventFlag	flag = kSEFNone;
	
	SetupDACompoundObject(flag);

	const bool	useStateInScript = (flag & kSEFUseStateInScriptAsInitialState) != 0;

	mUseInitialTransition = (flag & kSEFUseTransitionInScriptTheFirstTime) != 0;

	InitializeLocationRole(useStateInScript);
	InitializeOrientationRole(useStateInScript);
	InitializeMotionRole(useStateInScript);
	InitializeAudioRole(useStateInScript);
    InitializeParentRole(useStateInScript);
	
	InitMotionPlayingFlags();

	Goto(scene.GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
CPP_DEFN CompoundSceneEntity::CompoundSceneEntity(Scene& scene)
: mSceneEntityState(*this, "Compound Scene Entity", false), mDACompoundObject(NULL)
, mState(NULL), mDAHardPoints(NULL)
, mShowHardPoints(false)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mMotionRoleIndex = mSceneEntityState.AddRole(*new MotionRole(false, Interpolate, Interpolate, GetStateName, new MotionRoleCallback(*this)));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));
	mParentRoleIndex = mSceneEntityState.AddRole(*new ParentRole(false, Interpolate, Interpolate, GetStateName, new ParentRoleCallback(*this)));

	mSceneEntityState.SetScene(&scene);
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::Delete()
{
	ACompoundSceneEntityBaseClass::Delete();
	AAudibleSceneEntityBaseClass::Delete();

	delete this;
}
// --------------------------------------------------------------------------
CPP_DEFN CompoundSceneEntity::~CompoundSceneEntity()
{
    if(IsNotNull(mDAHardPoints))
    {   
		HardPointsDestroy(mDAHardPoints);
        mDAHardPoints = NULL;
    }

	DestroyAllMotions();

	if(IsNotNull(mDACompoundObject))
	{
		void*	userData = mSceneEntityState.GetUserData();

		mSceneEntityState.GetScene().DestroyDACompoundObject(mDACompoundObject, mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, &userData);

		mSceneEntityState.SetUserData(userData);

		CompoundEntityStaticsState*	state = dynamic_cast<CompoundEntityStaticsState*>(mState.get());
		ASSERT(state);

		state->SetDACompoundObject(NULL);
	}
}
// --------------------------------------------------------------------------
bool CompoundSceneEntity::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	if(IsNotNull(mDACompoundObject))
	{
		return CompoundObjectIntersect(mDACompoundObject, intersectInfo, distance);
	}
	else
	{
		return false;
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::SetupDACompoundObject(SceneEventFlag& flag)
{
	ASSERT(IsNull(mDACompoundObject));

	void*	userData = mSceneEntityState.GetUserData();
    
	mDACompoundObject = mSceneEntityState.GetScene().CreateDACompoundObject(mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, &userData, &flag);

	mSceneEntityState.SetUserData(userData);

    if(IsNotNull(mDACompoundObject))
    {   
		mDAHardPoints = HardPointsCreate(mDACompoundObject);
        
		if(IsNotNull(mDAHardPoints))
        {
			// Everything is in place. Set up the state object
			try
			{
				CompoundObjectGetMotionNames(mDACompoundObject, mMotionNames);

				mState = AggAPointer<AStaticsState>(new CompoundEntityStaticsState(mDACompoundObject));
			}
			catch(...)
			{
				void*	userData = mSceneEntityState.GetUserData();

				mSceneEntityState.GetScene().DestroyDACompoundObject(mDACompoundObject, mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, &userData);

				mSceneEntityState.SetUserData(userData);

				throw ExConstructionFailed(mSceneEntityState.GetName());
			}
        }
		else
		{
			void*	userData = mSceneEntityState.GetUserData();

			mSceneEntityState.GetScene().DestroyDACompoundObject(mDACompoundObject, mSceneEntityState.GetName(), mCategoryName, mDescriptionStrings, &userData);

			mSceneEntityState.SetUserData(userData);

			throw ExConstructionFailed(mSceneEntityState.GetName());
		}
    }
	else
	{
		// Compound didn't get created. Set up some place holder state object
		try
		{
			mState = AggAPointer<AStaticsState>(new StaticsState);

			flag = kSEFUseStateInScriptAsInitialState;
		}
		catch(...)
		{
			throw ExConstructionFailed(mSceneEntityState.GetName());
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString CompoundSceneEntity::GetArchetypeName() const
{
    return GetCompoundSceneEntityArchetypeName();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString CompoundSceneEntity::GetCompoundSceneEntityArchetypeName()
{
    return "CompoundSceneEntity";
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::Write(std::ostream& oStream) const
{
    ACompoundSceneEntityBaseClass::Write(oStream);
	AAudibleSceneEntityBaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::Read(std::istream& iStream)
{
	ACompoundSceneEntityBaseClass::Read(iStream);
	AAudibleSceneEntityBaseClass::Read(iStream);

    ReadSubObject(iStream);

	mSceneEntityState.SetVisible(true);	// Force this entity visible

	mSceneEntityState.GetScene().UpdateDescriptionStrings(mSceneEntityState.GetName(), Scene::kCompound, mCategoryName, mDescriptionStrings);

	SceneEventFlag	flag = kSEFNone;

	SetupDACompoundObject(flag);

	const bool	useStateInScript = (flag & kSEFUseStateInScriptAsInitialState) != 0;

	mUseInitialTransition = (flag & kSEFUseTransitionInScriptTheFirstTime) != 0;

	InitializeLocationRole(useStateInScript);
	InitializeOrientationRole(useStateInScript);
	InitializeMotionRole(useStateInScript);
	InitializeAudioRole(useStateInScript);
    InitializeParentRole(useStateInScript);

	InitMotionPlayingFlags();

	CreateAllMotions();
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	char buffer[1024 * 20];
	int j;
	// write the lua enity table for this entity
	j = sprintf(buffer,			"\n");
	j+= sprintf(buffer + j,		"%s = \n{\n", mSceneEntityState.GetName().c_str());
	j+= sprintf(buffer + j,		"\ttype = COMPOUND,\n\tflags = 0,\n");
	j+= sprintf(buffer + j,		"\tspatialprops = \n\t{\n");
	j+= sprintf(buffer + j,		"\t\tpos = {%f, %f, %f},\n", GetDAObjectLocation().GetX(), GetDAObjectLocation().GetY(), GetDAObjectLocation().GetZ());
	j+= sprintf(buffer + j,		"\t\torient = { {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f} }\n",
			GetDAObjectOrientation().GetI().x, GetDAObjectOrientation().GetI().y, GetDAObjectOrientation().GetI().z, 
			GetDAObjectOrientation().GetJ().x, GetDAObjectOrientation().GetJ().y, GetDAObjectOrientation().GetJ().z, 
			GetDAObjectOrientation().GetK().x, GetDAObjectOrientation().GetK().y, GetDAObjectOrientation().GetK().z
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
		ARole*				aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetMotionRoleIndex()));
		MotionRole*	role = dynamic_cast<MotionRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
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

	oWiz.Put(kDescriptionStrings, mDescriptionStrings);
	oWiz.Put(kSceneEntityState, mSceneEntityState);
	oWiz.Put(kCategoryName, mCategoryName, buffer);
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	
	iWiz.Get(kDescriptionStrings, mDescriptionStrings);
	iWiz.Get(kSceneEntityState, mSceneEntityState);

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
CPP_DEFN void CompoundSceneEntity::SetupPosition() const
{
    // This method is a noop on purpose, since CompoundSceneEntity::Render() takes care of transforms
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::Render(const DABaseCamera* camera) const
{
	if(IsNotNull(mDACompoundObject))
	{
		CompoundObjectRenderObject(mDACompoundObject, camera);

		if(mShowHardPoints)
		{
			HardPointsDraw(mDAHardPoints, camera);
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN const std::auto_ptr<ConstCompoundStateAccessor> CompoundSceneEntity::GetConstCompoundStateAccessor() const
{
    return std::auto_ptr<ConstCompoundStateAccessor>(new ConstCompoundStateAccessor(*this));
}
// --------------------------------------------------------------------------
CPP_DEFN std::auto_ptr<CompoundStateAccessor> CompoundSceneEntity::GetCompoundStateAccessor()
{
    return std::auto_ptr<CompoundStateAccessor>(new CompoundStateAccessor(*this));
}
// --------------------------------------------------------------------------
CPP_DEFN bool CompoundSceneEntity::AreHardPointsShowing() const
{
    return mShowHardPoints;
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::ShowHardPoints(bool show)
{
    mShowHardPoints = show;
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int CompoundSceneEntity::GetHardPointCount() const
{
    return HardPointsGetCount(mDAHardPoints);
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString CompoundSceneEntity::GetHardPointName(unsigned int idx) const
{
    return HardPointsGetHardPointName(mDAHardPoints, idx);
}
// --------------------------------------------------------------------------
CPP_DEFN Location CompoundSceneEntity::GetHardPointLocation(unsigned int idx) const
{
    Vector    location;

    HardPointsGetHardPointPosition(mDAHardPoints, idx, location);

    return Location(location);
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation CompoundSceneEntity::GetHardPointOrientation(unsigned int idx) const
{
    Orientation    orient;

    HardPointsGetHardPointOrientation(mDAHardPoints, idx, orient);

    return orient;
}
// --------------------------------------------------------------------------
CPP_DEFN const HardPointHost* CompoundSceneEntity::GetHardPointHost(unsigned int idx) const
{
	return HardPointsGetHardPointHost(mDAHardPoints, idx);
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::AttachHardPointToParent(unsigned int childHardPointIndex, const HardPointHost* parentHardPointHost, const ROSString& parentHardPointName)
{
	HardPointsAttachHardPointToParent(mDAHardPoints, childHardPointIndex, parentHardPointHost, parentHardPointName);
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::DetachHardPointFromParent(unsigned int childHardPointIndex, const HardPointHost* parentHardPointHost, const ROSString& parentHardPointName)
{
	HardPointsDetachHardPointFromParent(mDAHardPoints, childHardPointIndex, parentHardPointHost, parentHardPointName);
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::Respond(const SceneEntityEvent& event)
{
    ACompoundSceneEntityBaseClass::Respond(event);
	AAudibleSceneEntityBaseClass::Respond(event);	
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int CompoundSceneEntity::GetCameraCount() const
{
	if(IsNotNull(mDACompoundObject))
	{
	    return CamerasGetCount(mDACompoundObject);
	}
	else
	{
		return 0;
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString CompoundSceneEntity::GetCameraName(unsigned int idx) const
{
	ASSERT(mDACompoundObject);	// The number of cameras is 0
	
    return CamerasGetCameraName(mDACompoundObject, idx);
}
// --------------------------------------------------------------------------
CPP_DEFN Location CompoundSceneEntity::GetCameraLocation(unsigned int idx) const
{
	ASSERT(mDACompoundObject);	// The number of cameras is 0

	Vector    location;

	CamerasGetCameraPosition(mDACompoundObject, idx, location);

	return Location(location);
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation CompoundSceneEntity::GetCameraOrientation(unsigned int idx) const
{
	ASSERT(mDACompoundObject);	// The number of cameras is 0

    Orientation    orient;

    CamerasGetCameraOrientation(mDACompoundObject, idx, orient);

    return orient;
}
// --------------------------------------------------------------------------
CPP_DEFN float CompoundSceneEntity::GetCameraHorizontalFOV(unsigned int idx) const
{
	ASSERT(mDACompoundObject);	// The number of cameras is 0

    return CamerasGetCameraHorizontalFOV(mDACompoundObject, idx);
}
// --------------------------------------------------------------------------
CPP_DEFN float CompoundSceneEntity::GetCameraVerticalFOV(unsigned int idx) const
{
	ASSERT(mDACompoundObject);	// The number of cameras is 0

    return CamerasGetCameraVerticalFOV(mDACompoundObject, idx);
}
// --------------------------------------------------------------------------
CPP_DEFN Location CompoundSceneEntity::GetDAObjectLocation() const
{
    Vector	location;

    CompoundObjectGetPosition(mDACompoundObject, location);

    return Location(location);
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation CompoundSceneEntity::GetDAObjectOrientation() const
{
    Orientation orientation;

    CompoundObjectGetOrientation(mDACompoundObject, orientation);

    return orientation;
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::SetDAObjectLocation(const Location& location)
{
    CompoundObjectSetPosition(mDACompoundObject, location.GetVector());
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::SetDAObjectOrientation(const Orientation& orient)
{
    CompoundObjectSetOrientation(mDACompoundObject, orient);
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::StateUpdated(Update::ID id, Time time)
{
	ACompoundSceneEntityBaseClass::StateUpdated(id, time);
	AAudibleSceneEntityBaseClass::StateUpdated(id, time);
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::LocationStateUpdated(Time time)
{
	const int		locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    ARole*			aRole = &mSceneEntityState.GetRole(locationRoleIndex);
    LocationRole*   lRole = dynamic_cast<LocationRole*>(aRole);
    ASSERT(lRole);
	
	const bool	existingKeyPoint = lRole->HasTime(time);

    lRole->StateUpdated(FlaggedLocation(GetDAObjectLocation(), lRole->GetState(time).GetInterpolationType()), time);

	if(!existingKeyPoint)
	{
		UpdateMotionPathMarkers();
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::OrientationStateUpdated(Time time)
{
	const int			orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
    ARole*				aRole = &mSceneEntityState.GetRole(orientationRoleIndex);
    OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
    ASSERT(oRole);

	const bool	existingKeyPoint = oRole->HasTime(time);

	const FlaggedOrientation	currentOrientation = oRole->GetState(time);

   	oRole->StateUpdated(FlaggedOrientation(GetDAObjectOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), time);

	if(!existingKeyPoint)
	{
		UpdateMotionPathMarkers();
	}
}
// --------------------------------------------------------------------------
unsigned int CompoundSceneEntity::GetMotionCount() const
{
	if(IsNotNull(mDACompoundObject))
	{
		return mMotionNames.GetStringCount();
	}
	else
	{
		return 0;
	}
}
// --------------------------------------------------------------------------
ROSString CompoundSceneEntity::GetMotionName(int motionIdx) const
{
	ASSERT(motionIdx < GetMotionCount());

	if(IsNotNull(mDACompoundObject))
	{
		return mMotionNames.GetString(motionIdx);
	}
	else
	{
		return "";
	}
}
// --------------------------------------------------------------------------
Time CompoundSceneEntity::GetMotionLength(const ROSString& motionName) const
{
	ASSERT(0);	// Not implemented

	return kTime0;
}
// --------------------------------------------------------------------------
ROSString CompoundSceneEntity::GetCurrentMotionName() const
{
    return mCurrentMotionName;
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::SetCurrentMotionName(const ROSString& motionName)
{
    mCurrentMotionName = motionName;
}
// --------------------------------------------------------------------------
Time CompoundSceneEntity::GetCurrentMotionTime() const
{
	ASSERT(mDACompoundObject);	// The number of motions is 0

#ifdef NEW_ANIM_LIB_COMPLETE
    return Time(CompoundObjectGetCurrentMotionTime(mDACompoundObject));
#else
    return kTime0;
#endif
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::Start(const ROSString& motionName, Time startTime, Time transition)
{
	if(!Performing())
    {
		const Time	currentTime = GetCurrentTimePoint();

		if(mDACompoundObject)
		{
			const DAMotionObject*	motionObject = CompoundObjectCreateMotionObject(mDACompoundObject, motionName);

			if(!motionObject)
			{
				throw ExMotionCreationFailed(motionName);
			}

			mMotionObjects.push_back(TimeDAMotionPair(currentTime, motionObject));
		}

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
		if(motionObject)
		{
			MotionObjectStart(motionObject, false, 0.0, transition.GetTime());
		}
    }
#endif
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::Loop(const ROSString& motionName, Time startTime, Time transition)
{
	if(!Performing())
    {   
		Time	currentTime = GetCurrentTimePoint();

		if(mDACompoundObject)
		{
			const DAMotionObject*	motionObject = CompoundObjectCreateMotionObject(mDACompoundObject, motionName);

			if(!motionObject)
			{
				throw ExMotionCreationFailed(motionName);
			}

			mMotionObjects.push_back(TimeDAMotionPair(currentTime, motionObject));
		}

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
		if(motionObject)
		{
			MotionObjectStart(motionObject, motionName, true, 0.0, transition.GetTime());
		}
    }
#endif
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::Pause(const ROSString& motionName)
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
		if(motionObject)
		{
			MotionObjectPause(motionObject);
		}
    }
#endif
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::Resume(const ROSString& motionName)
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
		if(motionObject)
		{
			MotionObjectResume(motionObject);
		}
    }
#endif
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::Stop(const ROSString& motionName)
{
	if(!Performing())
    {   
		Time		currentTime = GetCurrentTimePoint();
		const int	motionRoleIndex = GetMotionRoleIndex();
		ASSERT(motionRoleIndex >= 0);
		ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
        MotionRole* motionRole = dynamic_cast<MotionRole*>(aRole);
		ASSERT(motionRole);

		motionRole->StateUpdated(MotionState(MotionState::kStopMotion, motionName, kTime0, kTime0), currentTime);
    }
#if 0
    else
	{
		if(motionObject)
		{
	        MotionObjectStopMotion(motionObject);
		}
    }
#endif
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::StartIK(const ROSString& endEffectorName, unsigned int countToRootEffector, AStaticSceneEntity& targetEntity, Time transition)
{
	ASSERT(0 && "Not implemented");
}
// --------------------------------------------------------------------------
IKState CompoundSceneEntity::GetIKState(Time startTime) const
{
	ASSERT(0 && "Not implemented");

	return IKState();		// Keeps the compiler satisfied
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::SetIKState(const IKState& iKState, Time startTime)
{
	ASSERT(0 && "Not implemented");
}
// --------------------------------------------------------------------------
long CompoundSceneEntity::GetRootEngineIndex() const
{
	// *** NOTE: This should really be including the DA engine.h header file and return
	// *** INVALID_ENGINE_INDEX. It is defined as -1, so we will return that here.
	return (long) -1;
}

// --------------------------------------------------------------------------
CPP_DEFN void CompoundSceneEntity::InitMotionPlayingFlags()
{
    mIsMotionActuallyPlaying.resize(0, kTime0);
}
// --------------------------------------------------------------------------
CPP_DEFN bool CompoundSceneEntity::IsMotionActuallyPlayingFlagSet(Time startTime)
{
    ASSERT(GetMotionRoleIndex() >= 0 && mSceneEntityState.GetRoleCount() > GetMotionRoleIndex());
    
	MotionPlayingFlags::iterator begin = mIsMotionActuallyPlaying.begin();
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
CPP_DEFN void CompoundSceneEntity::SetMotionActuallyPlayingFlag(Time startTime, bool isPlaying)
{
    ASSERT(GetMotionRoleIndex() >= 0 && mSceneEntityState.GetRoleCount() > GetMotionRoleIndex());
    
	MotionPlayingFlags::iterator begin = mIsMotionActuallyPlaying.begin();
    const MotionPlayingFlags::const_iterator end = mIsMotionActuallyPlaying.end();

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
Time CompoundSceneEntity::GetGreatestMotionPlayingFlagTime() const
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
void CompoundSceneEntity::Goto(Time time)
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
void CompoundSceneEntity::GotoForLocationRole(Time time)
{
	const int			locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    const ARole*		aRole = &mSceneEntityState.GetRole(locationRoleIndex);
	const LocationRole*	lRole = dynamic_cast<const LocationRole*>(aRole);
	ASSERT(lRole);

    SetDAObjectLocation(lRole->GetState(time));
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::GotoForOrientationRole(Time time)
{
	const int				orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
    const ARole*  aRole = &mSceneEntityState.GetRole(orientationRoleIndex);

	const OrientationRole*   oRole = dynamic_cast<const OrientationRole*>(aRole);
	ASSERT(oRole);

    SetDAObjectOrientation(oRole->GetState(time));
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::InitializeLocationRole(bool useStateInScript)
{
	const int	locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(locationRoleIndex);

    aRole->SetName("Location");

    LocationRole*   lRole = dynamic_cast<LocationRole*>(aRole);
    ASSERT(lRole);

    if(!(mSceneEntityState.GetScene().IsUsingInitialEntityState() || useStateInScript))
    {	
        lRole->StateUpdated(FlaggedLocation(GetDAObjectLocation(), lRole->GetState(kTime0).GetInterpolationType()), kTime0);
    }
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::GotoForMotionRole(Time time)
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

		if(mDACompoundObject)
		{
			StopAllMotions();
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
			const MotionState					motionState = motionRole->GetState(time);
			const MotionState::MotionEventID	motionEvent = motionState.GetMotionEvent();
			const ROSString						motionName = motionState.GetMotionName();
			const Time							motionStart = motionState.GetStartTime();
			Time								motionTransition = motionState.GetTransitionTime();
			const DAMotionObject*				motionObject = GetDAMotionObject(motionTime);

			switch(motionEvent)
			{
				case MotionState::kStartMotion:
					SetMotionActuallyPlayingFlag(motionTime, true);

					if(mDACompoundObject && motionObject)
					{
						if(motionTime == kTime0 && !mUseInitialTransition && mSceneEntityState.GetScene().IsFirstIteration())
						{
							motionTransition = kTime0;
						}

						const Time	delay = time - motionTime;
						const Time	transition = delay < motionTransition ? motionTransition - delay : kTime0;

						MotionObjectStart(motionObject, false, (motionStart + delay).GetTime(), transition.GetTime());

#if 0
//OutputDebugString("Starting motion: ");
//OutputDebugString(motionName.c_str());
//OutputDebugString("\n");
#endif
					}

					break;

				case MotionState::kLoopMotion:
					SetMotionActuallyPlayingFlag(motionTime, true);

					if(mDACompoundObject && motionObject)
					{
						if(motionTime == kTime0 && !mUseInitialTransition && mSceneEntityState.GetScene().IsFirstIteration())
						{
							motionTransition = kTime0;
						}

						const Time	delay = time - motionTime;
						const Time	transition = delay < motionTransition ? motionTransition - delay : kTime0;

						MotionObjectStart(motionObject, true, (motionStart + delay).GetTime(), transition.GetTime());

#if 0
//OutputDebugString("Starting loop: ");
//OutputDebugString(motionName.c_str());
//OutputDebugString("\n");
#endif
					}

					break;

				case MotionState::kPauseMotion:
					if(mDACompoundObject && motionObject)
					{
						MotionObjectPause(motionObject);
					}

					SetMotionActuallyPlayingFlag(motionTime, false);

					break;

				case MotionState::kResumeMotion:
					if(mDACompoundObject && motionObject)
					{
						MotionObjectResume(motionObject);
					}

					SetMotionActuallyPlayingFlag(motionTime, true);

					break;

				case MotionState::kStopMotion:
					if(mDACompoundObject && motionObject)
					{
						MotionObjectStop(motionObject);
					}

					SetMotionActuallyPlayingFlag(motionTime, false);

					break;

				case MotionState::kInternalMotion:
					// Don't do anything!
					break;
				default:
					ASSERT(0);	// Unhandled case
					return;
			}
		}
    }
    else
    {   
		// Not performing
        InitMotionPlayingFlags();

		if(mDACompoundObject)
		{
			StopAllMotions();
		}

#if 0
		// Create a scrub effect!
		const unsigned int timePointCount = motionRole->CountTimePoints();

		for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
		{
			const Time							motionStartTime = motionRole->GetTime(timePointIdx);

			if(motionStartTime > time)
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
					durationForEvent = (nextMotionTime - motionStartTime).GetTime();
				}
				else
				{
					// The next marker will not be in the scrub
					durationForEvent = (time - motionStartTime).GetTime();
				}
			}
			else
			{
				// We are at the last marker before or at the specified time
				durationForEvent = (time - motionStartTime).GetTime();
			}

			switch(motionEvent)
			{
				case MotionState::kStartMotion:
					SetMotionActuallyPlayingFlag(motionStartTime, true);
					
					if(mDACompoundObject)
					{
						DeformableObjectStartMotion(mDACompoundObject, motionName, false, durationForEvent, motionTransition.GetTime());
						DeformableObjectUpdate(mDACompoundObject);
					}
					break;

				case MotionState::kLoopMotion:
					SetMotionActuallyPlayingFlag(motionStartTime, true);
					
					if(mDACompoundObject)
					{
						DeformableObjectStartMotion(mDACompoundObject, motionName, true, durationForEvent, motionTransition.GetTime());
						DeformableObjectUpdate(mDACompoundObject);
					}
					break;

				case MotionState::kPauseMotion:
					
					if(mDACompoundObject)
					{
						DeformableObjectPauseMotion(mDACompoundObject);
						DeformableObjectUpdate(mDACompoundObject);
					}

					SetMotionActuallyPlayingFlag(motionStartTime, false);
					break;

				case MotionState::kResumeMotion:
					if(mDACompoundObject)
					{
						DeformableObjectResumeMotion(mDACompoundObject);
						DeformableObjectUpdate(mDACompoundObject);
					}

					SetMotionActuallyPlayingFlag(motionStartTime, true);
					break;

				case MotionState::kStopMotion:
					if(mDACompoundObject)
					{
						DeformableObjectStopMotion(mDACompoundObject);
						DeformableObjectUpdate(mDACompoundObject);
					}

					SetMotionActuallyPlayingFlag(motionStartTime, false);
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
void CompoundSceneEntity::InitializeOrientationRole(bool useStateInScript)
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

		oRole->StateUpdated(FlaggedOrientation(GetDAObjectOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), kTime0);
    }
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::InitializeMotionRole(bool useStateInScript)
{
	const int	motionRoleIndex = GetMotionRoleIndex();
	ASSERT(motionRoleIndex >= 0);
    ARole*  aRole = &mSceneEntityState.GetRole(motionRoleIndex);

    aRole->SetName("Motion");
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::InitializeAudioRole(bool useStateInScript)
{
	const int	audioRoleIndex = GetAudioRoleIndex();
	ASSERT(audioRoleIndex >= 0);
    ARole&		aRole = mSceneEntityState.GetRole(audioRoleIndex);

    aRole.SetName("Audio");
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::InitializeParentRole(bool useStateInScript)
{
	const int	parentRoleIndex = GetParentRoleIndex();
	ASSERT(parentRoleIndex >= 0);
    ARole&		aRole = mSceneEntityState.GetRole(parentRoleIndex);

    aRole.SetName("Parent");
}
// --------------------------------------------------------------------------
SceneEntityState& CompoundSceneEntity::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& CompoundSceneEntity::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& CompoundSceneEntity::GetPhysicalState()
{
	return *mState;
}
// --------------------------------------------------------------------------
const APhysicalState& CompoundSceneEntity::GetPhysicalState() const
{
	return *mState;
}
// --------------------------------------------------------------------------
int CompoundSceneEntity::GetLocationRoleIndex() const
{
	return mLocationRoleIndex;
}
// --------------------------------------------------------------------------
int CompoundSceneEntity::GetOrientationRoleIndex() const
{
	return mOrientationRoleIndex;
}
// --------------------------------------------------------------------------
int CompoundSceneEntity::GetMotionRoleIndex() const
{
	return mMotionRoleIndex;
}
// --------------------------------------------------------------------------
int CompoundSceneEntity::GetAudioRoleIndex() const
{
	return mAudioRoleIndex;
}
// --------------------------------------------------------------------------
int CompoundSceneEntity::GetParentRoleIndex() const
{
	return mParentRoleIndex;
}
// --------------------------------------------------------------------------
const DAMotionObject* CompoundSceneEntity::GetDAMotionObject(Time time)
{
	MotionObjects::iterator			begin = mMotionObjects.begin();
	const MotionObjects::iterator	end = mMotionObjects.end();

	while(begin != end)
	{
		if(begin->mTime == time)
		{
			return begin->mMotionObject;
		}

		++begin;
	}

	return NULL;
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::StopAllMotions()
{
	MotionObjects::iterator			begin = mMotionObjects.begin();
	const MotionObjects::iterator	end = mMotionObjects.end();

	while(begin != end)
	{
		MotionObjectStop(begin->mMotionObject);

		++begin;
	}
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::DestroyAllMotions()
{
	MotionObjects::iterator			begin = mMotionObjects.begin();
	const MotionObjects::iterator	end = mMotionObjects.end();

	while(begin != end)
	{
		MotionObjectStop(begin->mMotionObject);
		MotionObjectDestroy(begin->mMotionObject);

		++begin;
	}

	mMotionObjects.resize(0);
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::CreateAllMotions()
{
	ASSERT(mMotionObjects.empty());

	if(mDACompoundObject)
	{
		const int	motionRoleIndex = GetMotionRoleIndex();
		ASSERT(motionRoleIndex >= 0);
		ARole*		aRole = &mSceneEntityState.GetRole(motionRoleIndex);
		MotionRole* motionRole = dynamic_cast<MotionRole*>(aRole);
		ASSERT(motionRole);

		const unsigned int	motionCount = motionRole->CountTimePoints();

		for(unsigned int motionIdx; motionIdx < motionCount; ++motionIdx)
		{
			const MotionState state = motionRole->GetState(motionIdx);
				
			const DAMotionObject*	motionObject = CompoundObjectCreateMotionObject(mDACompoundObject, state.GetMotionName());

			if(motionObject)
			{
				mMotionObjects.push_back(TimeDAMotionPair(motionRole->GetTime(motionIdx), motionObject));
			}
		}
	}
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::RemoveMotion(Time motionTime)
{
	MotionObjects::iterator			begin = mMotionObjects.begin();
	const MotionObjects::iterator	end = mMotionObjects.end();

	while(begin != end)
	{
		if(begin->mTime == motionTime)
		{
			MotionObjectStop(begin->mMotionObject);
			MotionObjectDestroy(begin->mMotionObject);

			mMotionObjects.erase(begin);

			return;
		}

		++begin;
	}
}
// --------------------------------------------------------------------------
void CompoundSceneEntity::ChangeMotionTime(Time currentTime, Time newTime)
{
	if(currentTime == newTime)
	{
		return;
	}

	MotionObjects::iterator	begin = mMotionObjects.begin();
	MotionObjects::iterator	end = mMotionObjects.end();

	while(begin != end)
	{
		const Time	motionTime = begin->mTime;

		if(motionTime == currentTime)
		{
			begin->mTime = newTime;
		}
		else if(motionTime == newTime)
		{
			MotionObjectStop(begin->mMotionObject);
			MotionObjectDestroy(begin->mMotionObject);

			begin = mMotionObjects.erase(begin);
			
			end = mMotionObjects.end();

			continue;	// Don't want ++begin to execute
		}

		++begin;
	}
}
// --------------------------------------------------------------------------
CompoundSceneEntity::AudioPlayingFlags& CompoundSceneEntity::GetAudioPlayingFlags()
{
	return mIsAudioActuallyPlaying;
}
// --------------------------------------------------------------------------
const CompoundSceneEntity::AudioPlayingFlags& CompoundSceneEntity::GetAudioPlayingFlags() const
{
	return mIsAudioActuallyPlaying;
}
// --------------------------------------------------------------------------
CompoundSceneEntity::ParentEventFlags& CompoundSceneEntity::GetParentEventFlags()
{
	return mIsParentEventHandled;
}
// --------------------------------------------------------------------------
const CompoundSceneEntity::ParentEventFlags& CompoundSceneEntity::GetParentEventFlags() const
{
	return mIsParentEventHandled;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

