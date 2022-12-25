// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include <strstream>
#include "Scene.h"
#include "ASceneEntity.h"
#include "Article.h"
#include "Actor.h"
#include "Light.h"
#include "Camera.h"
#include "DACamera.h"
#include "LiveCamera.h"
#include "StringList.h"
#include "EventIterator.h"
//#include "DATagChannel.h"
#include "Utils.h"
#include "Char.h"
#include "TimeType.h"
#include "StringType.h"
#include "ModuleVersion.h"
// --------------------------------------------------------------------------
/**# implementation Scene:: id(C_0886778586)
*/
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
enum FieldID
{
	kVersionTag,
	kName,
	kDuration,
	kTimeTags,
	kSceneEntityList,
	kHasLiveCamera,		// Added in ROS Version 2.0.7.0
	kROSDLLVersion		// Added in ROS Version 2.6.0.0
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
#ifdef SUPPORT_OLD_SCE_FILE_FORMAT
static const ROSString	kOldVersionType = "Version";
static const ROSString	kOldVersionNumber = "4.0";
static const ROSString	kOldVersionInfo = kOldVersionType + ROSString(" ") + kOldVersionNumber;
#endif

static const ROSString	kSeparator = " ";
static const ROSString	kVersionType = "StreamWiz";
static const ROSString	kVersionNumber = "1.0";
static const ROSString	kVersionInfo = kVersionType + kSeparator + kVersionNumber;

static const ROSString	kModuleName = "ROS.DLL";

SceneEntityRemapper::RemapListCollection	SceneEntityRemapper::mRemapCollection;
// --------------------------------------------------------------------------
namespace KeyTags
{
enum KeyTagValue
{
	kSceneBegin,
	kSceneEnd
};
}
// --------------------------------------------------------------------------
CPP_DEFN Scene::Scene(Callback callback, void* userData)
: mSceneEntityList(new SceneEntityList(*this)), mCurrentTime(0)/*mDAChannelP(TagChannelCreate())*/, mDuration(0), 
  mIsPerforming(false), mTimeTags(2), mUserData(userData), mCallback(callback), mUpdateGameEngine(false), 
  mName("New"), mUseInitialState(false), mHasLiveCamera(false), mPerformanceStyle(kOnce), 
  mIsFirstIteration(true)
{
	ASSERT(callback);

    //OutputDebugString("In Scene::Scene()\n");

	/*if(IsNull(mDAChannelP))
    {
		throw ExSceneCreationFailed();
    }
    */

    UpdateTimeTagDataDuration();
}
// --------------------------------------------------------------------------
CPP_DEFN Scene::~Scene()
{
	mIsPerforming = false;
    //TagChannelDestroy(mDAChannelP);
}
// --------------------------------------------------------------------------
void Scene::SetPerformanceStyle(PerformanceStyle style)
{
	mPerformanceStyle = style;
}
// --------------------------------------------------------------------------
Scene::PerformanceStyle Scene::GetPerformanceStyle() const
{
	return mPerformanceStyle;
}
// --------------------------------------------------------------------------
CPP_DEFN const ROSString& Scene::GetName() const
{
	return mName;
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::SetGameEngineUpdatingMode(bool updateGameEngine)
{
    mUpdateGameEngine = updateGameEngine;
}
// --------------------------------------------------------------------------
CPP_DEFN bool Scene::IsUpdatingGameEngine() const
{
    return mUpdateGameEngine;
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::SetUseInitialEntityState(bool use)
{
	mUseInitialState = use;
}
// --------------------------------------------------------------------------
CPP_DEFN bool Scene::IsUsingInitialEntityState() const
{
	return mUseInitialState;
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::Perform(Time startTime)
{
	mIsPerforming = true;
    Goto(startTime);

    //TagChannelStart(mDAChannelP, ROS::Begin);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::Stop()
{
	mIsPerforming = false;
    Goto(Time(0));
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::Update(Time timeDelta)
{
	Goto(mCurrentTime + timeDelta);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::Pause()
{
	mIsPerforming = false;

    //TagChannelStop(mDAChannelP);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::Resume()
{
	mIsPerforming = true;

    //TagChannelStart(mDAChannelP, ROS::Current);
}
// --------------------------------------------------------------------------
CPP_DEFN bool Scene::IsPerforming() const
{
	return mIsPerforming;
}
// --------------------------------------------------------------------------
CPP_DEFN bool Scene::IsFirstIteration() const
{
	return mIsFirstIteration;
}
// --------------------------------------------------------------------------
CPP_DEFN Time Scene::GetDuration() const
{
    return mDuration;
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::SetDuration(Time duration)
{
    ASSERT(duration.GetTime() >= 0);

	Time	currentDuration = mDuration;

	mDuration = duration;

    try
    {
		UpdateTimeTagDataDuration();
    }
    catch(...)
    {
		mDuration = currentDuration;
    	throw;
    }
}
// --------------------------------------------------------------------------
CPP_DEFN Time Scene::GetCurrentTimePoint() const
{
	return mCurrentTime;
//    return Time(TagChannelGetCurrentTime(mDAChannelP));
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::SetCurrentTimePoint(Time time)
{
	Goto(time);
//    TagChannelSetCurrentTime(mDAChannelP, time.GetTime());
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::Write(std::ostream& ostreamR) const
{
	WriteSubObject(ostreamR);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::WriteSubObject(std::ostream& ostreamR) const
{
	OStreamWiz<FieldID>	oWiz(ostreamR);

	oWiz.Put(kVersionTag, kVersionInfo);
	oWiz.Put(kName, mName);
	char buffer[128];
	
	// write the duration to the other file
	sprintf(buffer, "--%s\n\nduration = %f;\n\nentities = \n{\n", mName.c_str(), mDuration.GetTime());
	oWiz.Put(kDuration, mDuration, buffer);
	oWiz.Put(kTimeTags, mTimeTags);

	oWiz.Put(kSceneEntityList, *mSceneEntityList);
	oWiz.Put(kHasLiveCamera, mHasLiveCamera, "\n\n\tmonitor_1 = \n\t{\n\t\ttype = MONITOR\n\t}\n};\n");

	// Put the ROS.DLL version number
	const ModuleVersion	rOSVersion = GetModuleVersion(kModuleName);
	const ROSString		rOSVersionStr = rOSVersion.GetVersionString();

	oWiz.Put(kROSDLLVersion, kModuleName + kSeparator + rOSVersionStr);
    // Don't write performing flag
	// ostreamR << mIsPerforming << std::endl;
	// Don't write current time
    // mCurrentTime.Write(ostreamR);
}
// --------------------------------------------------------------------------
CPP_DEFN void* Scene::GetUserData() const
{
	return mUserData;
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::RenameSceneEntity(const ROSString& kEntityToRenameR, const ROSString& kNewNameR)
{
    mSceneEntityList->RenameSceneEntity(kEntityToRenameR, kNewNameR);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::RemoveSceneEntity(const ASceneEntity& sceneEntityR)
{
	mSceneEntityList->RemoveSceneEntity(sceneEntityR);

	mHasLiveCamera = HasLiveCameras();

	ModelNS::UpdateID	id = ModelNS::kEntityRemoved;
	void*				idP = &id;
	const void*			sceneEntityP = &sceneEntityR;

	SceneEventFlag		flags = kSEFNone;

    mCallback(*this, kSESceneEvent, "Event", "EventCategory", StringList(), &sceneEntityP, &idP, &flags);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::AddSceneEntity(ASceneEntity& sceneEntityR)
{
	mSceneEntityList->AddSceneEntity(sceneEntityR);

	if(dynamic_cast<LiveCamera*>(&sceneEntityR))
	{
		mHasLiveCamera = true;
	}

	ModelNS::UpdateID	id = ModelNS::kEntityAdded;
	void*				idP = &id;
	const void*			sceneEntityP = &sceneEntityR;

	SceneEventFlag		flags = kSEFNone;

    mCallback(*this, kSESceneEvent, "Event", "EventCategory", StringList(), &sceneEntityP, &idP, &flags);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::Read(std::istream& iStreamR, StreamType streamType)
{
	ReadSubObject(iStreamR, streamType);

    UpdateTimeTagDataDuration();
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::ReadSubObject(std::istream& iStreamR, StreamType streamType)
{
	if(streamType == kSceneData)
    {
		// Clean out carriage returns added by VSS.
		std::strstream	cleanStream;

		while(!iStreamR.eof())
		{
			char	c;

			iStreamR.get(c);

			if(static_cast<int>(c) != 13)
			{
				cleanStream.put(c);
			}
		}

		// Work with the cleaned data from here on
		ROSString	version;
		
#ifdef SUPPORT_OLD_SCE_FILE_FORMAT
		// Determine the file type, inform the reading wizard, and set stream back to original state
		const std::istream::pos_type	startPos = cleanStream.tellg();
		bool							isOld = false;

		unsigned int    stringSize;

		cleanStream >> stringSize;

		if(stringSize >= kOldVersionType.length() && stringSize <= kOldVersionInfo.length())
		{
			// We may have a valid old file
			std::auto_ptr<char>	stringBuff(new char[stringSize + 1]);
			cleanStream.read(stringBuff.get(), stringSize);
			stringBuff.get()[stringSize] = 0;

			const ROSString	string = ROSString(stringBuff.get());

			isOld = string.substr(0, kOldVersionType.length()) == kOldVersionType;
		}
		
		IStreamWizSetIsOldFileFormat(isOld);

		cleanStream.seekg(startPos);
#endif

		IStreamWiz<FieldID>	iWiz(cleanStream);

		iWiz.Get(kVersionTag, version);
		/******NOTE: THROW ON WRONG VERSION**************/
		iWiz.Get(kName, mName);
		iWiz.Get(kDuration, mDuration);
		iWiz.Get(kTimeTags, mTimeTags);

		const bool	foundDataForHasLiveCamera = iWiz.Has(kHasLiveCamera);

		if(foundDataForHasLiveCamera)
		{
			iWiz.Get(kHasLiveCamera, mHasLiveCamera);
		}
		else
		{
			mHasLiveCamera = false;
		}

		iWiz.Get(kSceneEntityList, *mSceneEntityList);

		if(!foundDataForHasLiveCamera)
		{
			// Since the kHasLiveCamera was introduced in a later version, this file doesn't have the data
			mHasLiveCamera = HasLiveCameras();
		}

		// Don't read performing flag
        // cleanStream >> mIsPerforming;
		// Don't read current time
        // mCurrentTime.Read(cleanStream);
    }
    else if(streamType == kSceneList)
    {
		std::auto_ptr<StringList> entityList(new StringList);

        entityList->ReadStrings(iStreamR);

        if(entityList->GetString(0) != "Scene" || entityList->GetString(2) != "Duration")
        {
			throw ExInvalidSceneStream();
        }

        mName = entityList->GetString(1);

        ROSString   duration = entityList->GetString(3);
        mDuration.SetTime(atof(duration.c_str()));

        int	count = entityList->GetStringCount();

        for(int idx = 4; idx < count; idx += 2)
        {
			ASceneEntity*	sceneEntity;
            ROSString	name = entityList->GetString(idx + 1);

            // TODO: USE AFactory FOR CONSTRUCTION
            if(entityList->GetString(idx) == "Camera")
            {
				sceneEntity = new Camera(name, true, *this);
            }
            else if(entityList->GetString(idx) == "DACamera")
            {
				sceneEntity = new DACamera(name, true, *this);
            }
            else if(entityList->GetString(idx) == "Light")
            {
				sceneEntity = new Light(name, true, *this);
            }
            else if(entityList->GetString(idx) == "Article")
            {
				sceneEntity = new Article(name, true, *this);
            }
            else if(entityList->GetString(idx) == "Actor")
            {
				sceneEntity = new Actor(name, true, *this);
            }
            else
            {
				// TODO: CLEAN UP OBJECTS ALLOCATED SO FAR
                throw ExInvalidSceneEntityInStream(entityList->GetString(idx));
            }
            AddSceneEntity(*sceneEntity);
        }
    }
    else
    {
		throw ExInvalidSceneStream();
    }

	RemapAllSceneEntityReferences();

	Goto(Time(0));
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::RemapAllSceneEntityReferences()
{
	const unsigned int idCount = mSceneEntityRemapper.GetIDCount();

	for(unsigned int idx = 0; idx < idCount; ++idx)
	{
		ROSString		entityName = mSceneEntityRemapper.GetID(idx);
        ASceneEntity*	entity = mSceneEntityList->GetSceneEntity(entityName);

		ASSERT(entity); // No entity with the name in the scene!

		mSceneEntityRemapper.RemapAtIndex(idx, entity);
	}

	mSceneEntityRemapper.Clear();
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::GetSceneEntities(SceneEntityCollection& sceneEntityColl) const
{
	EntityList::ConstIterator	begin = mSceneEntityList->Begin();
	const EntityList::ConstIterator	kEnd = mSceneEntityList->End();

    while(begin != kEnd)
    {
		sceneEntityColl.push_back(*begin);
    	++begin;
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::UpdateTimeTagDataDuration()
{
    mTimeTags.front().Set(Time(0), KeyTags::kSceneBegin);
    mTimeTags.back().Set(mDuration, KeyTags::kSceneEnd);

/*
	Time currentTime = mKeyTimes[mKeyTimeCount - 1];

    try
    {
		TagChannelReplaceTimeTagData(mDAChannelP, mKeyTimes, mKeyTags, mKeyTimeCount, EventHandlerFunction, this);
    }
    catch(...)
    {	
		mKeyTimes[mKeyTimeCount - 1] = currentDuration;
    }
*/
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::Goto(Time currentTime)
{
	switch(mPerformanceStyle)
	{
		case kOnce:
			if(currentTime > mDuration)
			{
				currentTime = mDuration;
			}
			break;

		case kRepeat:
			if(currentTime > mDuration)
			{
				currentTime = ROS::Time(0);
				mIsFirstIteration = false;
			}
			break;

		case kLoop:
			if(currentTime > mDuration)
			{
				currentTime -= mDuration;
				mIsFirstIteration = false;
				
				if(mDuration != Time(0))
				{
					// Won't cause infinite loop!
					Goto(Time(0));
				}
			}
			break;
		default:
			ASSERT(0);	// Unknown case!
	}

	Time    timeDelta = currentTime - mCurrentTime;

	if(timeDelta != Time(0) || (currentTime == Time(0) && timeDelta == Time(0)))
    {   
        EntityList::Iterator	begin = mSceneEntityList->Begin();
        const EntityList::Iterator	kEnd = mSceneEntityList->End();

        while(begin != kEnd)
        {
			(*begin)->GetSceneEntityStateAccessor()->Goto(currentTime);
            ++begin;
        }

        if(mUpdateGameEngine)
        {
			if(IsPerforming())
			{
				if(timeDelta >= Time(0))
				{
					GameEngineUpdate(timeDelta.GetTime());
				}
			}
        }

        mCurrentTime = currentTime;
    }
}
// --------------------------------------------------------------------------
CPP_DEFN const DACompoundObject* Scene::CreateDACompoundObject(const ROSString& sceneEntityName, const ROSString& categoryName, const StringList& descriptionStrings, void** entityData, SceneEventFlag* flags) const
{
    const void* compoundObject = NULL;

    mCallback(*this, kSECompoundSceneEntityConstruct, sceneEntityName, categoryName, descriptionStrings, &compoundObject, entityData, flags);

    return reinterpret_cast<const DACompoundObject*>(compoundObject);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::DestroyDACompoundObject(const DACompoundObject* dACompoundObj, const ROSString& sceneEntityName, const ROSString& categoryName, const StringList& descriptionStrings, void** entityData) const
{
	const void*		compoundObject = dACompoundObj;
	SceneEventFlag	flags = kSEFNone;

    mCallback(*this, kSECompoundSceneEntityDelete, sceneEntityName, categoryName, descriptionStrings, &compoundObject, entityData, &flags);

	ASSERT(flags == kSEFNone);
}
// --------------------------------------------------------------------------
CPP_DEFN const DADeformableObject* Scene::CreateDADeformableObject(const ROSString& sceneEntityName, const ROSString& categoryName, const StringList& descriptionStrings, void** entityData, SceneEventFlag* flags) const
{
    const void* deformableObject = NULL;

    mCallback(*this, kSEDeformableSceneEntityConstruct, sceneEntityName, categoryName, descriptionStrings, &deformableObject, entityData, flags);

    return reinterpret_cast<const DADeformableObject*>(deformableObject);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::DestroyDADeformableObject(const DADeformableObject* dACompoundObj, const ROSString& categoryName, const ROSString& sceneEntityName, const StringList& descriptionStrings, void** entityData) const
{
    const void*		deformableObject = dACompoundObj;
	SceneEventFlag	flags = kSEFNone;

    mCallback(*this, kSEDeformableSceneEntityDelete, sceneEntityName, categoryName, descriptionStrings, &deformableObject, entityData, &flags);

	ASSERT(flags == kSEFNone);
}
// --------------------------------------------------------------------------
CPP_DEFN const DABaseCamera* Scene::CreateDABaseCamera(const ROSString& sceneEntityName, void** entityData, SceneEventFlag* flags) const
{
    const void* baseCamera = NULL;

	if(mHasLiveCamera)
	{
		*flags |= kSEFScriptHasLiveCamera;
	}

    mCallback(*this, kSEBaseCameraConstruct, sceneEntityName, "DABaseCamera", StringList(), &baseCamera, &mUserData, flags);

    return reinterpret_cast<const DABaseCamera*>(baseCamera);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::DestroyDABaseCamera(const DABaseCamera* dABaseCamera, const ROSString& sceneEntityName, void** entityData) const
{
    const void*		baseCamera = dABaseCamera;
	SceneEventFlag	flags = kSEFNone;

	if(mHasLiveCamera)
	{
		flags |= kSEFScriptHasLiveCamera;
	}

    mCallback(*this, kSEBaseCameraDelete, sceneEntityName, "DABaseCamera", StringList(), &baseCamera, entityData, &flags);
}
// --------------------------------------------------------------------------
CPP_DEFN const DABaseCamera* Scene::CreateInternalDABaseCamera(const ROSString& sceneEntityName, void** entityData, SceneEventFlag* flags) const
{
    const void* baseCamera = NULL;

	if(mHasLiveCamera)
	{
		*flags |= kSEFScriptHasLiveCamera;
	}

    mCallback(*this, kSEInternalBaseCameraConstruct, sceneEntityName, "InternalDABaseCamera", StringList(), &baseCamera, &mUserData, flags);

    return reinterpret_cast<const DABaseCamera*>(baseCamera);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::DestroyInternalDABaseCamera(const DABaseCamera* dABaseCamera, const ROSString& sceneEntityName, void** entityData) const
{
    const void*		baseCamera = dABaseCamera;
	SceneEventFlag	flags = kSEFNone;

	if(mHasLiveCamera)
	{
		flags |= kSEFScriptHasLiveCamera;
	}

    mCallback(*this, kSEInternalBaseCameraDelete, sceneEntityName, "InternalDABaseCamera", StringList(), &baseCamera, entityData, &flags);
}
// --------------------------------------------------------------------------
CPP_DEFN const DAAudioObject* Scene::CreateDAAudioObject(const ROSString& soundName, Time startTime, const AStaticSceneEntity* staticSE) const
{
    const void*		audioObject = staticSE;
	SceneEventFlag	flags = kSEFNone;
	float			timePoint = startTime.GetTime();
	void*			time = &timePoint;

    mCallback(*this, kSEAudioObjectConstruct, soundName, "DAAudioObject", StringList(), &audioObject, &time, &flags);

    return reinterpret_cast<const DAAudioObject*>(audioObject);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::DestroyDAAudioObject(const DAAudioObject* audioObject) const
{
    const void*		audioObj = audioObject;
	SceneEventFlag	flags = kSEFNone;

    mCallback(*this, kSEAudioObjectDelete, "", "DAAudioObject", StringList(), &audioObj, NULL, &flags);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::PlayDAAudioObject(const DAAudioObject* audioObject, Time startTime) const
{
    const void*		audioObj = audioObject;
	SceneEventFlag	flags = kSEFNone;
	float			timePoint = startTime.GetTime();
	void*			time = &timePoint;

    mCallback(*this, kSEAudioObjectPlay, "", "DAAudioObject", StringList(), &audioObj, &time, &flags);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::StopDAAudioObject(const DAAudioObject* audioObject) const
{
    const void*		audioObj = audioObject;
	SceneEventFlag	flags = kSEFNone;

    mCallback(*this, kSEAudioObjectStop, "", "DAAudioObject", StringList(), &audioObj, NULL, &flags);
}
// --------------------------------------------------------------------------
CPP_DEFN void Scene::UpdateDescriptionStrings(const ROSString& entityName, EntityType entityType, ROSString& categoryName, StringList& descriptionStrings) const
{
	const void*		dummyEntity;
	void*			dummyUserData;
	SceneEventFlag	flags = kSEFNone;

    if(entityType == kCompound)
	{
		mCallback(*this, kSECompoundSceneEntityUpdateDescription, entityName, categoryName, descriptionStrings, &dummyEntity, &dummyUserData, &flags);
	}
	else if(entityType == kDeformable)
	{
		mCallback(*this, kSEDeformableSceneEntityUpdateDescription, entityName, categoryName, descriptionStrings, &dummyEntity, &dummyUserData, &flags);
	}
	else if(entityType == kAudio)
	{
		mCallback(*this, kSEAudioEntityUpdateDescription, entityName, categoryName, descriptionStrings, &dummyEntity, &dummyUserData, &flags);
	}

	ASSERT(flags == kSEFNone);
}
// --------------------------------------------------------------------------
CPP_DEFN ADynamicCamera* Scene::FindFirstCamera()
{
    EntityList::Iterator	begin = mSceneEntityList->Begin();
    const EntityList::Iterator	kEnd = mSceneEntityList->End();

    while(begin != kEnd)
    {
		ADynamicCamera* camera = dynamic_cast<ADynamicCamera*>(*begin);
        if(camera)
        {
			return camera;
        }
        else
        {
			++begin;
        }
    }

    return NULL;
}
// --------------------------------------------------------------------------
bool Scene::HasLiveCameras() const
{
    EntityList::Iterator	begin = mSceneEntityList->Begin();
    const EntityList::Iterator	kEnd = mSceneEntityList->End();

    while(begin != kEnd)
    {
		if(dynamic_cast<LiveCamera*>(*begin))
        {
			return true;
        }

		++begin;
    }

    return false;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

