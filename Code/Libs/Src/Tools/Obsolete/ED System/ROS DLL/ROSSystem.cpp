// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>

#include "ROSSystem.h"
#include "TimeType.h"
#include "StringList.h"
#include "EntityDescription.h"
#include "SceneModel.h"
#include "Vector.h"
#include "Matrix.h"
#include "AStaticSceneEntity.h"

#if 1
#include "DADeformableObject.h"
#endif

//---------------------------------------------------------------------------
namespace ROSSystem
{
// --------------------------------------------------------------------------
class UserData
{	
	public:
		UserData(Callback clientCallback, void* clientUserData)
		: mClientCallback(clientCallback), mClientUserData(clientUserData)
		{
		}

		Callback	GetClientCallback() const { return mClientCallback; }
		void*		GetClientUserData() const { return mClientUserData; }

		void		SetClientUserData(void* clientUserData) { mClientUserData = clientUserData; }

	private:
		Callback	mClientCallback;
		void*		mClientUserData;
};
// --------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
// --------------------------------------------------------------------------
static ROS::SceneModel::PerformanceStyle gSceneModelPerformanceStyle[] =
	{
		ROS::SceneModel::kOnce,
		ROS::SceneModel::kRepeat,
		ROS::SceneModel::kLoop
	};

static ROSSystem::PerformanceStyle gROSSystemPerformanceStyle[] =
	{
		ROSSystem::kPTOnce,
		ROSSystem::kPTRepeat,
		ROSSystem::kPTLoop
	};
// --------------------------------------------------------------------------
CPP_DEFN bool __cdecl startup(IDAComponent* system, IEngine* engine, HWND wndH)
{
    return ROSSystemStartup(system, engine, wndH);
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl shutdown()
{
    ROSSystemShutdown();
}
// --------------------------------------------------------------------------
static ROS::SceneModel* GetSceneModel(const Scene* scene)
{
	return const_cast<ROS::SceneModel*>(reinterpret_cast<const ROS::SceneModel*>(scene));
}
// --------------------------------------------------------------------------
static const Scene* GetScene(const ROS::SceneModel* scene)
{
	return reinterpret_cast<const Scene*>(scene);
}
// --------------------------------------------------------------------------
static ROS::ASceneEntity* GetASceneEntity(const SceneEntity* sceneEntity)
{
	return const_cast<ROS::ASceneEntity*>(reinterpret_cast<const ROS::ASceneEntity*>(sceneEntity));
}
// --------------------------------------------------------------------------
static const SceneEntity* GetSceneEntity(const ROS::ASceneEntity* aSceneEntity)
{
	return reinterpret_cast<const SceneEntity*>(aSceneEntity);
}
// --------------------------------------------------------------------------
static void SceneCallback(ROS::SceneModel& sceneModel, ROS::SceneEvent sceneEvent, const ROS::ROSString& sceneEntityName, const ROS::ROSString& entityCategoryName, const ROS::StringList& descriptionStrings, const void** object, void** entityUserData, ROS::SceneEventFlag* flag)
{
	{
		int		sE = sceneEvent;
		char	ev[20];

		itoa(sE, ev, 10);

		//OutputDebugString("In SceneCallback. SceneEvent: ");
		//OutputDebugString(ev);
		//OutputDebugString("\n");
		//OutputDebugString("Starting callback to client\n");
	}

	UserData*	userDataObj = reinterpret_cast<UserData*>(sceneModel.GetUserData());
	ASSERT(userDataObj);

	Callback	clientCallback = userDataObj->GetClientCallback();
	ASSERT(clientCallback);

	void*			clientUserData = userDataObj->GetClientUserData();
	void*			cStr;
	ROS::ROSString	fileName;

	switch(sceneEvent)
	{
		case ROS::kSEAudioEntityUpdateDescription:
			CharString	audioFileName;
			fileName = descriptionStrings.GetString(0);

			strcpy(audioFileName, fileName.c_str());

			cStr = audioFileName;

			clientCallback(GetScene(&sceneModel), sceneEvent, sceneEntityName.c_str(), entityCategoryName.c_str(), object, &cStr, flag);
		
			const_cast<ROS::StringList&>(descriptionStrings).Replace(0, audioFileName);

			break;
	
		case ROS::kSECompoundSceneEntityConstruct:
			clientCallback(GetScene(&sceneModel), sceneEvent, sceneEntityName.c_str(), entityCategoryName.c_str(), object, entityUserData, flag);
			*object = reinterpret_cast<const void*>(reinterpret_cast<int>(*object) + 1);		// Adding 1 because ROSE works on INSTANCE_INDEX + 1
			
			break;

		case ROS::kSECompoundSceneEntityDelete:
			*object = reinterpret_cast<const void*>(reinterpret_cast<int>(*object) - 1);		// Subtracting 1 because ROSE works on INSTANCE_INDEX + 1
			try
			{
				clientCallback(GetScene(&sceneModel), sceneEvent, sceneEntityName.c_str(), entityCategoryName.c_str(), object, entityUserData, flag);
			}
			catch(...)
			{
				*object = reinterpret_cast<const void*>(reinterpret_cast<int>(*object) + 1);	// Adding 1 because ROSE works on INSTANCE_INDEX + 1
				throw;
			}

			*object = reinterpret_cast<const void*>(reinterpret_cast<int>(*object) + 1);	// Adding 1 because ROSE works on INSTANCE_INDEX + 1

			break;

		default:
			clientCallback(GetScene(&sceneModel), sceneEvent, sceneEntityName.c_str(), entityCategoryName.c_str(), object, entityUserData, flag);
	}
	
	//OutputDebugString("Finished callback to client\n");
}
// --------------------------------------------------------------------------
CPP_DEFN const Scene* __cdecl create(Callback callback, void* sceneUserData)
{
	ROS::SceneModel* sceneModel = NULL;
	
	UserData*	userData = new UserData(callback, sceneUserData);

    try
    {
		sceneModel = new ROS::SceneModel(SceneCallback, userData);
    }
    catch(...)
    {
    }

    return GetScene(sceneModel);
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl set_performance_style(const Scene* scene, PerformanceStyle style)
{
	ASSERT(scene);
	ASSERT(style == ROSSystem::kPTOnce || style == ROSSystem::kPTRepeat || style == ROSSystem::kPTLoop);

	ROS::SceneModel*	sceneModel = GetSceneModel(scene);

	sceneModel->SetPerformanceStyle(gSceneModelPerformanceStyle[style]);
}
// --------------------------------------------------------------------------
CPP_DEFN PerformanceStyle __cdecl get_performance_style(const Scene* scene)
{
	ASSERT(scene);

	ROS::SceneModel*	sceneModel = GetSceneModel(scene);

	const ROS::SceneModel::PerformanceStyle	sMStyle = sceneModel->GetPerformanceStyle();
	ASSERT(sMStyle == ROS::SceneModel::kOnce || sMStyle == ROS::SceneModel::kRepeat || sMStyle == ROS::SceneModel::kLoop);

	return gROSSystemPerformanceStyle[sMStyle];
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl set_use_initial_entity_state(const Scene* scene, bool use)
{
	ASSERT(scene);

	ROS::SceneModel*	sceneModel = GetSceneModel(scene);

	sceneModel->SetUseInitialEntityState(use);
}
// --------------------------------------------------------------------------
CPP_DEFN bool is_using_initial_entity_state(const Scene* scene)
{
	ASSERT(scene);

	ROS::SceneModel*	sceneModel = GetSceneModel(scene);

	return sceneModel->IsUsingInitialEntityState();
}
// --------------------------------------------------------------------------
CPP_DEFN bool __cdecl read(const Scene* scene, const CharString sceneFileName)
{
	ASSERT(scene);

	ROS::SceneModel*	sceneModel = GetSceneModel(scene);

    try
    {
		sceneModel->Read(sceneFileName);	

		return true;
	}
	catch(...)
	{
		return false;
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl destroy(const Scene* scene)
{
	ROS::SceneModel*	sceneModel = GetSceneModel(scene);
	UserData*			userData = reinterpret_cast<UserData*>(const_cast<void*>(sceneModel->GetUserData()));
	ASSERT(userData);

    delete sceneModel;

	delete userData;
}
// --------------------------------------------------------------------------
CPP_DEFN void* __cdecl get_user_data(const Scene* scene)
{
	ASSERT(scene);

	UserData*	userDataObj = reinterpret_cast<UserData*>(GetSceneModel(scene)->GetUserData());
	ASSERT(userDataObj);

	return userDataObj->GetClientUserData();
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl get_name(const Scene* scene, CharString name)
{
	ROS::ROSString   nameString = GetSceneModel(scene)->GetSceneName();

    strcpy(name, nameString.c_str());
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl get_filename(const Scene* scene, CharString fileName)
{
    ROS::ROSString   fileNameString = GetSceneModel(scene)->GetSceneFileName();

    strcpy(fileName, fileNameString.c_str());
}
// --------------------------------------------------------------------------
CPP_DEFN float __cdecl get_duration(const Scene* scene)
{
    ROS::Time    duration = GetSceneModel(scene)->GetSceneDuration();

    return duration.GetTime();
}
// --------------------------------------------------------------------------
CPP_DEFN float __cdecl get_current_time(const Scene* scene)
{
    ROS::Time    currTime = GetSceneModel(scene)->GetCurrentSceneTime();

    return currTime.GetTime();
}
// --------------------------------------------------------------------------
CPP_DEFN bool __cdecl is_playing(const Scene* scene)
{
    return GetSceneModel(scene)->IsScenePlaying();
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl set_user_data(const Scene* scene, void* userData)
{
	ASSERT(scene);

	UserData*	userDataObj = reinterpret_cast<UserData*>(GetSceneModel(scene)->GetUserData());
	ASSERT(userDataObj);

	userDataObj->SetClientUserData(userData);
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl set_duration(const Scene* scene, float duration)
{
    GetSceneModel(scene)->SetSceneDuration(ROS::Time(duration));
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl set_current_time(const Scene* scene, float time)
{
    GetSceneModel(scene)->SetCurrentSceneTime(ROS::Time(time));
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl play(const Scene* scene)
{
    GetSceneModel(scene)->PlayScene();
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl update(const Scene* scene, float timeDelta)
{
    GetSceneModel(scene)->UpdateScene(ROS::Time(timeDelta));
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl Pause(const Scene* scene)
{
    GetSceneModel(scene)->PauseScene();
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl save(const Scene* scene, const CharString fileName, bool saveExtraData)
{
    GetSceneModel(scene)->SaveScene(fileName, saveExtraData);
}
//---------------------------------------------------------------------------
CPP_DEFN void __cdecl get_location(const SceneEntity* sceneEntity, Vector& location)
{
	const ROS::ASceneEntity*		aSceneEntity = GetASceneEntity(sceneEntity);
	const ROS::AStaticSceneEntity*	aStaticSceneEntity = dynamic_cast<const ROS::AStaticSceneEntity*>(aSceneEntity);
	ASSERT(aStaticSceneEntity);
	const ROS::Location				loc = aStaticSceneEntity->GetConstStaticsStateAccessor()->GetLocation();

	location = loc.GetVector();
}
//---------------------------------------------------------------------------
CPP_DEFN void __cdecl get_orientation(const SceneEntity* sceneEntity, Matrix& orientation)
{
	const ROS::ASceneEntity*		aSceneEntity = GetASceneEntity(sceneEntity);
	const ROS::AStaticSceneEntity*	aStaticSceneEntity = dynamic_cast<const ROS::AStaticSceneEntity*>(aSceneEntity);
	ASSERT(aStaticSceneEntity);
	const ROS::Orientation			orient = aStaticSceneEntity->GetConstStaticsStateAccessor()->GetOrientation();

	orientation.set_i(orient.GetI());
	orientation.set_j(orient.GetJ());
	orientation.set_k(orient.GetK());
}
//---------------------------------------------------------------------------
#ifdef __cplusplus
}   // extern "C"
#endif
// --------------------------------------------------------------------------
}
//---------------------------------------------------------------------------

