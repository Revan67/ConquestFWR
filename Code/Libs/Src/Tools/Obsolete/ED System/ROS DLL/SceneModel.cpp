// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <iostream>
#include <fstream>

#ifdef _MSC_VER
#include <stdlib.h>
#else
#include <dir.h>
#endif

#include "SceneModel.h"
#include "Scene.h"
#include "ASceneEntity.h"
#include "StringType.h"
#include "CompoundSceneEntity.h"
#include "DeformableSceneEntity.h"
#include "PositionMarker.h"
#include "DACamera.h"
#include "LiveCamera.h"
#include "DAAmbientLight.h"
#include "DADynamicSpotLight.h"
#include "SpotLightStateAccessor.h"
#include "CodeMsg.h"
#include "EventIterator.h"
#include "StringUtils.h"

// --------------------------------------------------------------------------
std::ostream * gKludgyThornConversionStep1Stream;
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/**# implementation SceneModel:: id(C_0888350758)
*/

// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
static const Time	kDefaultSceneDuration(10);

static SceneModel::PerformanceStyle gSceneModelPerformanceStyle[] =
	{
		SceneModel::kOnce,
		SceneModel::kRepeat,
		SceneModel::kLoop
	};

static Scene::PerformanceStyle gScenePerformanceStyle[] =
	{
		Scene::kOnce,
		Scene::kRepeat,
		Scene::kLoop
	};
// --------------------------------------------------------------------------
class UserData
{	
	public:
		UserData(SceneModel& sceneModel, SceneModel::Callback clientCallback, void* clientUserData)
		: mSceneModel(sceneModel), mClientCallback(clientCallback), mClientUserData(clientUserData)
		{
		}

		SceneModel&				GetSceneModel() const { return mSceneModel; }
		SceneModel::Callback	GetClientCallback() const { return mClientCallback; }
		void*					GetClientUserData() const { return mClientUserData; }

	private:
		SceneModel&				mSceneModel;
		SceneModel::Callback	mClientCallback;
		void*					mClientUserData;
};
// --------------------------------------------------------------------------
CPP_DEFN SceneModel::SceneModel(Callback callback, void* userData)
:mSelectedSceneEntityP(NULL), mSecondarySceneEntityP(NULL), mSceneEntitySelectionLocked(false)
, mNewSceneModel(NULL), mUndoOperationStack(100), mRedoOperationStack(100)
{
    //OutputDebugString("In SceneModel::SceneModel\n");

	std::auto_ptr<UserData>	myUserData(new UserData(*this, callback, userData));
	
	mSceneSP = SceneSP(new Scene(SceneCallback, myUserData.get()));
	mSceneSP->SetDuration(kDefaultSceneDuration);

	myUserData.release();
}
// --------------------------------------------------------------------------
CPP_DEFN SceneModel::~SceneModel()
{
//    Notify(ModelNS::kSceneModelDeleted);
	UserData* userData = reinterpret_cast<UserData*>(mSceneSP->GetUserData());
	ASSERT(userData);

	delete mSceneSP.get();

	mSceneSP.release();

	delete userData;
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SetPerformanceStyle(PerformanceStyle style)
{
	mSceneSP->SetPerformanceStyle(gScenePerformanceStyle[style]);
}
// --------------------------------------------------------------------------
CPP_DEFN SceneModel::PerformanceStyle SceneModel::GetPerformanceStyle() const
{
	const Scene::PerformanceStyle	sStyle = mSceneSP->GetPerformanceStyle();
	ASSERT(sStyle == Scene::kOnce || sStyle == Scene::kRepeat || sStyle == Scene::kLoop);

	return gSceneModelPerformanceStyle[sStyle];
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SetUseInitialEntityState(bool use)
{
	mSceneSP->SetUseInitialEntityState(use);
}
// --------------------------------------------------------------------------
CPP_DEFN bool SceneModel::IsUsingInitialEntityState() const
{
	return mSceneSP->IsUsingInitialEntityState();
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::Read(const ROSString& sceneFileName)
{
    if(!sceneFileName.empty())
	{
		Scene::StreamType		streamType;
		std::ios_base::openmode	openMode = std::ios::in;

		const ROSString	extension = GetFileExtension(sceneFileName);

		if(extension == ROSString(".sce"))
		{
			streamType = Scene::kSceneData;
			openMode |= std::ios::binary;
		}
		else if(extension == ROSString(".scl"))
		{
			streamType = Scene::kSceneList;
		}
		else
		{
			throw ExInvalidSceneFile();
		}

		std::ifstream	iFStream(sceneFileName.c_str(), openMode);

		if(!iFStream.is_open())
		{
			throw ExInvalidSceneFile();
		}

		mSceneSP->Read(iFStream, streamType);

		mSceneFileName = sceneFileName;
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString SceneModel::GetSceneName() const
{
    return mSceneSP->GetName();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString SceneModel::GetSceneFileName() const
{
	return mSceneFileName;
}
// --------------------------------------------------------------------------
CPP_DEFN void* SceneModel::GetUserData() const
{
	UserData* userData = reinterpret_cast<UserData*>(mSceneSP->GetUserData());
	ASSERT(userData);

	return userData->GetClientUserData();
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::GetSceneEntities(SceneEntityCollection& kSceneEntityCollectionR) const
{
    mSceneSP->GetSceneEntities(kSceneEntityCollectionR);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SetSelectedSceneEntity(ASceneEntity* sceneEntityP)
{
	// If the selection is locked, due to lazy evaluation, the selection is not reset
    if(!mSceneEntitySelectionLocked && sceneEntityP != mSelectedSceneEntityP.reset(sceneEntityP))
    {
		Notify(ModelNS::kEntitySelectionChanged);
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SelectedSceneEntityUpdated()
{
	// Notify about selected entity
    Notify(ModelNS::kSelectedEntityUpdated);

	// If selected entity has any dependent entity, update for that as well
	ASSERT(IsNotNull(mSelectedSceneEntityP));

	ASceneEntity*	dependentEntity = mSelectedSceneEntityP->GetSceneEntityStateAccessor()->GetDependentEntity();

	if(dependentEntity)
	{
		SetSecondarySceneEntity(dependentEntity);
		SecondaryDependentSceneEntityUpdated();
		SetSecondarySceneEntity(NULL);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ASceneEntity* SceneModel::GetSelectedSceneEntity() const
{
    return mSelectedSceneEntityP.get();
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SetSecondarySceneEntity(ASceneEntity* sceneEntityP)
{
    mSecondarySceneEntityP.reset(sceneEntityP);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SecondarySceneEntityUpdated()
{
    Notify(ModelNS::kSecondaryEntityUpdated);

	ASceneEntity*	secondarySceneEntity = GetSecondarySceneEntity();
	ASSERT(IsNotNull(secondarySceneEntity));
	
	ASceneEntity*	dependentEntity = secondarySceneEntity->GetSceneEntityStateAccessor()->GetDependentEntity();

	if(dependentEntity)
	{
		SetSecondarySceneEntity(dependentEntity);
		SecondaryDependentSceneEntityUpdated();
		SetSecondarySceneEntity(secondarySceneEntity);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SecondaryDependentSceneEntityUpdated()
{
    Notify(ModelNS::kSecondaryDependentEntityUpdated);
}
// --------------------------------------------------------------------------
CPP_DEFN ASceneEntity* SceneModel::GetSecondarySceneEntity() const
{
    return mSecondarySceneEntityP.get();
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SceneUpdated()
{
    Notify(ModelNS::kAll);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::LockSceneEntitySelection(bool lock)
{
	mSceneEntitySelectionLocked = lock;	/**** NOTE: CREATE A kSelectionLocked OR kSelectionUnlocked EVENT AND SEND THAT OUT. ****/
	Notify(ModelNS::kEntitySelectionLockUpdated);
}
// --------------------------------------------------------------------------
CPP_DEFN bool SceneModel::IsSceneEntitySelectionLocked() const
{
	return mSceneEntitySelectionLocked;
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddSceneEntity(ASceneEntity& sceneEntity)
{
    mSceneSP->AddSceneEntity(sceneEntity);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddCompoundSceneEntity(const ROSString& entityName, const ROS::ROSString& categoryName, const StringList& descriptionStrings)
{
    AddSceneEntity(*(new CompoundSceneEntity(entityName, categoryName, descriptionStrings, *mSceneSP)));
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddDeformableSceneEntity(const ROSString& entityName, const ROS::ROSString& categoryName, const StringList& descriptionStrings)
{
    AddSceneEntity(*(new DeformableSceneEntity(entityName, categoryName, descriptionStrings, *mSceneSP)));
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddNewPositionMarker()
{
    AddSceneEntity(*(new PositionMarker("New_Position_Marker", true, *mSceneSP)));
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddNewCamera()
{
    AddSceneEntity(*(new DACamera("New_Camera", true, *mSceneSP)));
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddNewLiveCamera()
{
    AddSceneEntity(*(new LiveCamera("New_Live_Camera", true, *mSceneSP)));
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddNewAmbientLight()
{
    AddSceneEntity(*(new DAAmbientLight("Ambient_Light", true, *mSceneSP)));
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddNewDirectionalLight()
{
	DADynamicSpotLight*	directionalLight = new DADynamicSpotLight("Directional_Light", true, *mSceneSP);

	directionalLight->GetSpotLightStateAccessor()->SetInfinite(true);
    
	AddSceneEntity(*directionalLight);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddNewPointLight()
{
	DADynamicSpotLight*	pointLight = new DADynamicSpotLight("Point_Light", true, *mSceneSP);

	pointLight->GetSpotLightStateAccessor()->SetCutOff(180);
    
	AddSceneEntity(*pointLight);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddNewSpotLight()
{
    AddSceneEntity(*(new DADynamicSpotLight("Spot Light", true, *mSceneSP)));
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::RemoveSceneEntity(const ASceneEntity& sceneEntityR)
{
	const bool	isSelectedEntity = (GetSelectedSceneEntity() == &sceneEntityR);
	
	if(isSelectedEntity)
	{
		if(IsSceneEntitySelectionLocked())
		{
			LockSceneEntitySelection(false);
		}
	}

	mSceneSP->RemoveSceneEntity(sceneEntityR);

	if(isSelectedEntity)
	{
		SetSelectedSceneEntity(NULL);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::RenameSceneEntity(const ROSString& kEntityToRenameR, const ROSString& kNewNameR)
{
    mSceneSP->RenameSceneEntity(kEntityToRenameR, kNewNameR);
    Notify(ModelNS::kSelectedEntityUpdated);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SetGameEngineUpdatingMode(bool updateGameEngine)
{
    mSceneSP->SetGameEngineUpdatingMode(updateGameEngine);
}
// --------------------------------------------------------------------------
CPP_DEFN Time SceneModel::GetSceneDuration() const
{
    return mSceneSP->GetDuration();
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SetSceneDuration(Time duration)
{
    mSceneSP->SetDuration(duration);
    Notify(ModelNS::kSceneDurationUpdated);
}
// --------------------------------------------------------------------------
CPP_DEFN Time SceneModel::GetCurrentSceneTime() const
{
    return mSceneSP->GetCurrentTimePoint();
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SetCurrentSceneTime(Time time)
{
	Time	currentTime = GetCurrentSceneTime();

    mSceneSP->SetCurrentTimePoint(time);

    if(time != currentTime)
    {
		Notify(ModelNS::kSceneCurrentTimePointUpdated);
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::PlayScene()
{
#if 1
    //OutputDebugString("In SceneModel::PlayScene()\n");
#endif
   mSceneSP->Perform(Time(0.0));
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::PauseScene()
{
    mSceneSP->Pause();

	Notify(ModelNS::kScenePaused);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::StopScene()
{
    mSceneSP->Stop();
}
// --------------------------------------------------------------------------
CPP_DEFN bool SceneModel::IsScenePlaying() const
{
	return mSceneSP->IsPerforming();
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::UpdateScene(Time timeDelta)
{
	mSceneSP->Update(timeDelta);
    if(timeDelta != Time(0))
    {
		Notify(ModelNS::kSceneCurrentTimePointUpdated);
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SaveScene(const ROSString& fileName, bool saveExtraData)
{
	gKludgyThornConversionStep1Stream = NULL;
	char cnvFileBuffer[256];
	char tempBuffer[512];

	// write to temp file so crashes don't nuke the original
	sprintf(tempBuffer, "%s.tmp", fileName.c_str());

	if (NULL != fileName.c_str())
	{
		std::ofstream	oFStream(tempBuffer, std::ios_base::out | std::ios_base::binary);
		std::ofstream	oExtraFStream;
		if (saveExtraData)
		{
			sprintf(cnvFileBuffer, "%s%s", fileName.c_str(), ".cnv");
			oExtraFStream.open(cnvFileBuffer, std::ios_base::out | std::ios_base::binary);
			gKludgyThornConversionStep1Stream = &oExtraFStream;
		}
		mSceneSP->Write(oFStream);
		mSceneFileName = fileName;
		gKludgyThornConversionStep1Stream = NULL;
	}
	else
	{
		saveExtraData = false;
	}

	DWORD saveAttribs = GetFileAttributes(fileName.c_str());
	// saving succeeded so rename/move the temp file
	SetFileAttributes(fileName.c_str(), FILE_ATTRIBUTE_NORMAL);
	if (0 != CopyFile(tempBuffer, fileName.c_str(), false))
	{
		DeleteFile(tempBuffer);
	}
	else
	{
		DWORD error = GetLastError();
	}
	SetFileAttributes(fileName.c_str(), saveAttribs);

	if (saveExtraData)
	{
		const int	size = 255;
		char		moduleFileName[size];
		GetModuleFileName(NULL, moduleFileName, size);
		// do the conversion in two steps because the command string is too long for system
		sprintf(tempBuffer, "perl \"%srosconv.pl\" < %s > x", GetFilePath(moduleFileName).c_str(), cnvFileBuffer);
		system(tempBuffer);
		// move the file because the command string is too long
		sprintf(tempBuffer, "%s", fileName.c_str());
		sprintf(tempBuffer + strlen(tempBuffer) - 3, "thn");
		saveAttribs = GetFileAttributes(tempBuffer);
		SetFileAttributes(tempBuffer, FILE_ATTRIBUTE_NORMAL);
		if (0 != CopyFile("x", tempBuffer, false))
		{
			DeleteFile("x");
		}
		else
		{
			DWORD error = GetLastError();
		}
		SetFileAttributes(tempBuffer, saveAttribs);
		SetFileAttributes(cnvFileBuffer, FILE_ATTRIBUTE_NORMAL);
		DeleteFile(cnvFileBuffer);
	}
}
// --------------------------------------------------------------------------
void SceneModel::SceneCallback(const Scene& scene, SceneEvent sceneEvent, const ROSString& sceneEntityName, const ROSString& sceneEntityCategory, const StringList& descriptionStrings, const void** entity, void** entityUserData, SceneEventFlag* flags)
{
	const UserData*	userDataObj = reinterpret_cast<const UserData*>(scene.GetUserData());
	ASSERT(userDataObj);

	SceneModel&	sceneModelObj = userDataObj->GetSceneModel();

	if(sceneEvent == kSESceneEvent)
	{
		ModelNS::UpdateID	updateId = *(static_cast<const ModelNS::UpdateID*>(*entityUserData));

		if(updateId == ModelNS::kEntityRemoved)
		{
			const ASceneEntity*	sceneEntity = reinterpret_cast<const ASceneEntity*>(*entity);
			ASSERT(sceneEntity);

			if(sceneEntity == sceneModelObj.GetSelectedSceneEntity())
			{
				updateId = ModelNS::kSelectedEntityRemoved;
			}
			else
			{
				updateId = ModelNS::kAll;
			}

			// Remove operations from undo and redo stacks for the entity
			sceneModelObj.RemoveFromUndoRedoStacks(*sceneEntity);
			
			// If necessary, clear the remembered hardpoint
			if(sceneModelObj.RecallHardPoint().GetACompoundSceneEntity() == sceneEntity)
			{
				sceneModelObj.RememberHardPoint(HardPoint());
			}
		}

		sceneModelObj.Notify(updateId);
	}
	else
	{
		const Callback	clientCallback = userDataObj->GetClientCallback();

		if(clientCallback)
		{
			clientCallback(sceneModelObj, sceneEvent, sceneEntityName, sceneEntityCategory, descriptionStrings, entity, entityUserData, flags);
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::SwitchToNewSceneModel(SceneModel* sceneModel)
{
    mNewSceneModel = sceneModel;

    Notify(ModelNS::kSceneModelReplaced);

    mNewSceneModel->AcquireObservers(*this);    // AcquireObservers() notifies all its newly acquired observers with a kAll
}
// --------------------------------------------------------------------------
CPP_DEFN SceneModel* SceneModel::GetNewSceneModel()
{
    return mNewSceneModel;
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::ShutdownSystem()
{
    Notify(ModelNS::kSceneSystemShutdown);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::RememberPosition(const Position& position)
{	
	mRememberedPosition = position;
}
// --------------------------------------------------------------------------
CPP_DEFN Position SceneModel::RecallPosition() const
{
	return mRememberedPosition;
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::RememberHardPoint(const HardPoint& hardPoint)
{
	mRememberedHardPoint = hardPoint;
}
// --------------------------------------------------------------------------
CPP_DEFN HardPoint SceneModel::RecallHardPoint() const
{
	return 	mRememberedHardPoint;
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::UndoLastOperation() 
{
	if(!mUndoOperationStack.IsEmpty())
	{
		AOperation*	inverse = mUndoOperationStack.PerformTopOperationAndRemove();

		mRedoOperationStack.PushOperation(inverse);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::RedoLastOperation() 
{
	if(!mRedoOperationStack.IsEmpty())
	{
		AOperation*	inverse = mRedoOperationStack.PerformTopOperationAndRemove();

		mUndoOperationStack.PushOperation(inverse);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::AddUndoableOperation(AOperation* operation)
{
	mUndoOperationStack.PushOperation(operation);
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString SceneModel::GetUndoOperationName() const
{
	if(!mUndoOperationStack.IsEmpty())
	{
		return mUndoOperationStack.GetTopOperationName();
	}
	else
	{
		return "";
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString SceneModel::GetRedoOperationName() const
{
	if(!mRedoOperationStack.IsEmpty())
	{
		return mRedoOperationStack.GetTopOperationName();
	}
	else
	{
		return "";
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneModel::RemoveFromUndoRedoStacks(const ASceneEntity& entity)
{
	mUndoOperationStack.RemoveOperations(entity);
	mRedoOperationStack.RemoveOperations(entity);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
