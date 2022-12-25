// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>

#include "SceneEntityList.h"
#include "ASceneEntity.h"
#include "Article.h"
#include "Actor.h"
#include "Light.h"
#include "Camera.h"
#include "PositionMarker.h"
#include "DACamera.h"
#include "LiveCamera.h"
#include "DAAmbientLight.h"
#include "DADynamicSpotLight.h"
#include "CompoundSceneEntity.h"
#include "DeformableSceneEntity.h"
#include "Utils.h"
#include "CodeMsg.h"
#include "ConstSceneEntityStateAccessor.h"
#include "SceneEntityStateAccessor.h"
// --------------------------------------------------------------------------
/**# implementation SceneEntityList:: id(C_0886798843)
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kNumberOfSceneEntities,
	kFirstSceneEntityArchetypeName,
	kFirstSceneEntity
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
SceneEntityList::SceneEntityList(Scene& scene)
:mScene(scene)
{
}
// --------------------------------------------------------------------------
CPP_DEFN SceneEntityList::~SceneEntityList()
{
	ConstIterator	begin = Begin();
	const ConstIterator	kEnd = End();

    //OutputDebugString("Starting deletion of all SceneEntities in SceneEntityList\n");

    while(begin != kEnd)
    {
		(*begin)->Delete();
    	++begin;
    }

    //OutputDebugString("Deleted all SceneEntities in SceneEntityList\n");
}
// --------------------------------------------------------------------------
CPP_DEFN ASceneEntity* SceneEntityList::GetSceneEntity(const ROSString& kEntityName) const
{
	ConstIterator	begin = Begin();
	const ConstIterator	kEnd = End();

    while(begin != kEnd)
    {
		if((*begin)->GetConstSceneEntityStateAccessor()->GetName() == kEntityName)
        {
			return *begin;
        }
        else
        {
			++begin;
        }
    }

    return NULL;
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneEntityList::AddSceneEntity(ASceneEntity& sceneEntity)
{
	if(GetSceneEntity(sceneEntity.GetConstSceneEntityStateAccessor()->GetName()))
    {
		throw ExDuplicateSceneEntityName();
    }
    else
    {
		mSceneEntityPL.insert(mSceneEntityPL.end(), &sceneEntity);
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneEntityList::RemoveSceneEntity(const ASceneEntity& sceneEntity)
{
	Iterator		begin = Begin();
	const Iterator	kEnd = End();

    while(begin != kEnd)
    {
		if((*begin) == &sceneEntity)
        {
			mSceneEntityPL.erase(begin);
			return;
        }
        else
        {
			++begin;
        }
    }

	ASSERT(0);	// Specified an entity that is not in the list
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneEntityList::RenameSceneEntity(const ROSString& kEntityToRename, const ROSString& kNewName)
{
    ASceneEntity*    sceneEntityToRename = GetSceneEntity(kEntityToRename);

    if(IsNull(sceneEntityToRename))
    {
		throw ExSceneEntityNotFound();
    }

    if(IsNotNull(GetSceneEntity(kNewName)))
    {
		throw ExSceneEntityNameAlreadyInUse();
    }

    sceneEntityToRename->GetSceneEntityStateAccessor()->SetName(kNewName);
}
// --------------------------------------------------------------------------
CPP_DEFN SceneEntityList::ConstIterator SceneEntityList::Begin() const
{
	return mSceneEntityPL.begin();
}
// --------------------------------------------------------------------------
CPP_DEFN SceneEntityList::Iterator SceneEntityList::Begin()
{
	return mSceneEntityPL.begin();
}
// --------------------------------------------------------------------------
CPP_DEFN SceneEntityList::ConstIterator SceneEntityList::End() const
{
	return mSceneEntityPL.end();
}
// --------------------------------------------------------------------------
CPP_DEFN SceneEntityList::Iterator SceneEntityList::End()
{
	return mSceneEntityPL.end();
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int SceneEntityList::Size() const
{
	return mSceneEntityPL.size();
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneEntityList::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneEntityList::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	// First write the number of entities to be written
	SceneEntityList::ConstIterator			begin = Begin();
	const SceneEntityList::ConstIterator	end = End();
	unsigned int							numEntitiesToWrite = 0;

    while(begin != end)
    {
		const ASceneEntity*	entity = *begin;

		if(entity->IsPersistent())
		{
			++numEntitiesToWrite;
		}

   		++begin;
    }

	oWiz.Put(kNumberOfSceneEntities, numEntitiesToWrite);

	// Now write the entities themselves
	unsigned int	writtenEntities = 0;

	begin = Begin();

    while(begin != end)
    {
		const ASceneEntity*	entity = *begin;

		if(entity->IsPersistent())
		{
			oWiz.Put(static_cast<FieldID>(kFirstSceneEntityArchetypeName + (2 * writtenEntities)), entity->GetArchetypeName());
    		
			oWiz.Put(static_cast<FieldID>(kFirstSceneEntity + (2 * writtenEntities)), *entity);

			++writtenEntities;
		}

   		++begin;
    }

	ASSERT(numEntitiesToWrite == writtenEntities);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneEntityList::Read(std::istream& iStream)
{
    //OutputDebugString("In SceneEntityList::Read\n");
    
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void SceneEntityList::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    unsigned int	numEntities;

    iWiz.Get(kNumberOfSceneEntities, numEntities);

    for(unsigned int entityIdx = 0; entityIdx < numEntities; ++entityIdx)
    {
		ASceneEntity*    sceneEntity;
    	ROSString		archetypeName;

    	iWiz.Get(static_cast<FieldID>(kFirstSceneEntityArchetypeName + (2 * entityIdx)), archetypeName);

		if(archetypeName == Actor::GetActorArchetypeName())
		{
			sceneEntity = new Actor(mScene);
		}
		else if(archetypeName == Article::GetArticleArchetypeName())
		{
			sceneEntity = new Article(mScene);
		}
		else if(archetypeName == Light::GetLightArchetypeName())
		{
			sceneEntity = new Light(mScene);
		}
		else if(archetypeName == Camera::GetCameraArchetypeName())
		{
			sceneEntity = new Camera(mScene);
		}
		else if(archetypeName == PositionMarker::GetPositionMarkerArchetypeName())
		{
			sceneEntity = new PositionMarker(mScene);
		}
		else if(archetypeName == DACamera::GetDACameraArchetypeName())
		{
			sceneEntity = new DACamera(mScene);
		}
		else if(archetypeName == LiveCamera::GetLiveCameraArchetypeName())
		{
			sceneEntity = new LiveCamera(mScene);
		}
		else if(archetypeName == DAAmbientLight::GetDAAmbientLightArchetypeName())
		{
			sceneEntity = new DAAmbientLight(mScene);
		}
		else if(archetypeName == DADynamicSpotLight::GetDADynamicSpotLightArchetypeName())
		{
			sceneEntity = new DADynamicSpotLight(mScene);
		}
		else if(archetypeName == CompoundSceneEntity::GetCompoundSceneEntityArchetypeName())
		{
			sceneEntity = new CompoundSceneEntity(mScene);
		}
		else if(archetypeName == DeformableSceneEntity::GetDeformableSceneEntityArchetypeName())
		{
			sceneEntity = new DeformableSceneEntity(mScene);
		}
		else
		{	
			throw ExInvalidSceneEntityInStream(archetypeName);
		}

		iWiz.Get(static_cast<FieldID>(kFirstSceneEntity + (2 * entityIdx)), *sceneEntity);

        AddSceneEntity(*sceneEntity);

        //OutputDebugString("SceneEntity added\n");
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

