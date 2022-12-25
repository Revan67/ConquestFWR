// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "CodeMsg.h"
#include "SceneEntityState.h"
#include "SceneEntityRemapper.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kName,
	kIsVisible,
	kHasParent,
	kParentName,
	kRoleCount,
	kFirstRole
};
// --------------------------------------------------------------------------
namespace ROS
{
class SceneEntityAddSourceRemap: public TARemapClass<ASceneEntity*>
{
	public:
		SceneEntityAddSourceRemap(SceneEntityState& state)
		: mState(state)
		{
		}

	protected:
		typedef ASceneEntity* CPtr;

		virtual void Remap(const CPtr& value) const
		{
			ASSERT(value);

			mState.AddSource(*value);
		}

	private:
		SceneEntityState&	mState;
};

class SceneEntityAddListenerRemap: public TARemapClass<ASceneEntity*>
{
	public:
		SceneEntityAddListenerRemap(SceneEntityState& state)
		: mState(state)
		{
		}

	protected:
		typedef ASceneEntity* CPtr;

		virtual void Remap(const CPtr& value) const
		{
			ASSERT(value);

			mState.AddListener(*value);
		}

	private:
		SceneEntityState&	mState;
};
// --------------------------------------------------------------------------
SequenceGenerator<unsigned int> SceneEntityState::mNameSeqGen(0, 1);
// --------------------------------------------------------------------------
SceneEntityState::SceneEntityState(ASceneEntity& ownerEntity, const ROSString& name, bool makeNameUnique)
: mName(name), mOwnerEntity(&ownerEntity), mParentEntity(NULL), mIsVisible(true), mUserData(NULL), mTrackId(0), mScene(NULL), mIsStaticsPathVisible(false)
{
	if(makeNameUnique)
	{   
		unsigned int uniqueInt = mNameSeqGen.GetNextValue();

		try
		{   
			char	uniqueStr[10];

			mName += ROSString("_") + ROSString(itoa(uniqueInt, uniqueStr, 10));
		}
		catch(...)
		{   
			mNameSeqGen.SetNextValue(uniqueInt);
			throw;
		}
	}
}
// --------------------------------------------------------------------------
SceneEntityState::SceneEntityState(ASceneEntity& ownerEntity, std::istream& iStream)
: mOwnerEntity(&ownerEntity), mParentEntity(NULL), mIsVisible(true), mUserData(NULL), mScene(NULL), mIsStaticsPathVisible(false)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
SceneEntityState::~SceneEntityState()
{
	// Remove listeners
	mEventListenerTracker.Clear();

	// Remove this from all the sources
	mEventSourceTracker.RemoveFromAllSources(*mOwnerEntity);

	RoleList::iterator				begin = mRoles.begin();
	const RoleList::const_iterator	end = mRoles.end();

	while(begin != end)
	{
		delete *begin;

		++begin;
	}
}
// --------------------------------------------------------------------------
ROSString SceneEntityState::GetName() const
{
	return mName;
}
// --------------------------------------------------------------------------
void SceneEntityState::SetName(const ROSString& name)
{
	mName = name;
}
// --------------------------------------------------------------------------
bool SceneEntityState::IsVisible() const
{
	return mIsVisible;
}
// --------------------------------------------------------------------------
void SceneEntityState::SetVisible(bool isVisible)
{
	mIsVisible = isVisible;
}
// --------------------------------------------------------------------------
unsigned int SceneEntityState::GetRoleCount() const
{
	return mRoles.size();
}
// --------------------------------------------------------------------------
unsigned int SceneEntityState::AddRole(ARole& role)
{
	mRoles.push_back(&role);

	return mRoles.size() - 1;
}
// --------------------------------------------------------------------------
const ARole& SceneEntityState::GetRole(unsigned int roleIndex) const
{
	ASSERT(roleIndex < mRoles.size());

	return *mRoles[roleIndex];
}
// --------------------------------------------------------------------------
ARole& SceneEntityState::GetRole(unsigned int roleIndex)
{
	ASSERT(roleIndex < mRoles.size());

	return *mRoles[roleIndex];
}
// --------------------------------------------------------------------------
void SceneEntityState::SetUserData(void* userData) const
{
	mUserData = userData;
}
// --------------------------------------------------------------------------
void* SceneEntityState::GetUserData() const
{
	return mUserData;
}
// --------------------------------------------------------------------------
void SceneEntityState::SetTrackId(long trackId)
{
	mTrackId = trackId;
}
// --------------------------------------------------------------------------
long SceneEntityState::GetTrackId() const
{
	return mTrackId;
}
// --------------------------------------------------------------------------
ASceneEntity* SceneEntityState::GetParentEntity()
{
	return mParentEntity;
}
// --------------------------------------------------------------------------
const ASceneEntity* SceneEntityState::GetParentEntity() const
{
	return mParentEntity;
}
// --------------------------------------------------------------------------
void SceneEntityState::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void SceneEntityState::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void SceneEntityState::SetScene(Scene* scene)
{
	mScene.reset(scene);
}
// --------------------------------------------------------------------------
Scene& SceneEntityState::GetScene()
{
	ASSERT(IsNotNull(mScene));

	return *mScene.get();
}
// --------------------------------------------------------------------------
const Scene& SceneEntityState::GetScene() const
{
	ASSERT(IsNotNull(mScene));

	return *mScene.get();
}
// --------------------------------------------------------------------------
void SceneEntityState::SetStaticsPathVisible(bool visible)
{
	mIsStaticsPathVisible = visible;
}
// --------------------------------------------------------------------------
bool SceneEntityState::IsStaticsPathVisible() const
{
	return mIsStaticsPathVisible;
}
// --------------------------------------------------------------------------
void SceneEntityState::AddListener(ASceneEntityEventListener& listener)
{
	mEventListenerTracker.Add(listener);
}
// --------------------------------------------------------------------------
void SceneEntityState::RemoveListener(ASceneEntityEventListener& listener)
{
	mEventListenerTracker.Remove(listener);
}
// --------------------------------------------------------------------------
void SceneEntityState::RemoveAllListeners()
{
	mEventListenerTracker.RemoveAll();
}
// --------------------------------------------------------------------------
unsigned int SceneEntityState::GetListenerCount() const
{
	return mEventListenerTracker.GetCount();
}
// --------------------------------------------------------------------------
ASceneEntityEventListener& SceneEntityState::GetListener(unsigned int listenerIndex) const
{
	return mEventListenerTracker.Get(listenerIndex);
}
// --------------------------------------------------------------------------
void SceneEntityState::FireToListeners(const SceneEntityEvent& event)
{
	mEventListenerTracker.Fire(event);
}
// --------------------------------------------------------------------------
void SceneEntityState::AddSource(ASceneEntityEventSource& source)
{
	mEventSourceTracker.Add(source);
}
// --------------------------------------------------------------------------
void SceneEntityState::RemoveSource(ASceneEntityEventSource& source)
{
	mEventSourceTracker.Remove(source);
}
// --------------------------------------------------------------------------
unsigned int SceneEntityState::GetSourceCount() const
{
	return mEventSourceTracker.GetCount();
}
// --------------------------------------------------------------------------
ASceneEntityEventSource& SceneEntityState::GetSource(unsigned int sourceIndex) const
{
	return mEventSourceTracker.Get(sourceIndex);
}
// --------------------------------------------------------------------------
void SceneEntityState::RemoveFromAllSources(ASceneEntityEventListener& listener)
{
	mEventSourceTracker.RemoveFromAllSources(listener);
}
// --------------------------------------------------------------------------
void SceneEntityState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kName, mName);
	oWiz.Put(kIsVisible, mIsVisible);

	const bool	hasParent = IsNotNull(mParentEntity);
	
	oWiz.Put(kHasParent, hasParent);

	if(hasParent)
	{
		oWiz.Put(kParentName, mParentEntity->GetConstSceneEntityStateAccessor()->GetName());
	}

	// Write roles
	const unsigned int	roleCount = mRoles.size();

	oWiz.Put(kRoleCount, roleCount);

	RoleList::const_iterator		begin = mRoles.begin();
	const RoleList::const_iterator	end = mRoles.end();
	unsigned int					roleIdx = 0;

	while(begin != end)
	{
		oWiz.Put(static_cast<FieldID>(kFirstRole + roleIdx), **begin);

		++begin;
		++roleIdx;
	}

	// Write sources
	const FieldID		kSourceCount = static_cast<FieldID>(kFirstRole + roleIdx);
	const unsigned int	sourceCount = mEventSourceTracker.GetCount();
	unsigned int		count = 0;
	const FieldID		kFirstSource = static_cast<FieldID>(kSourceCount + 1);

	for(unsigned int sourceIdx = 0; sourceIdx < sourceCount; ++sourceIdx)
	{
		const ASceneEntityEventSource*	eventSource = &mEventSourceTracker.Get(sourceIdx);
		const ASceneEntity*				entity = dynamic_cast<const ASceneEntity*>(eventSource);
		ASSERT(entity);

		if(entity->IsPersistent())
		{
			oWiz.Put(static_cast<FieldID>(kFirstSource + sourceIdx), entity->GetConstSceneEntityStateAccessor()->GetName());

			++count;
		}
	}

	oWiz.Put(kSourceCount, count);

	// Write listeners
	const FieldID		kListenerCount = static_cast<FieldID>(kFirstSource + count);
	const unsigned int	listenerCount = mEventListenerTracker.GetCount();
	count = 0;

	const FieldID	kFirstListener = static_cast<FieldID>(kListenerCount + 1);

	for(unsigned int listenerIdx = 0; listenerIdx < listenerCount; ++listenerIdx)
	{
		const ASceneEntityEventListener*	eventListener = &mEventListenerTracker.Get(listenerIdx);
		const ASceneEntity*					entity = dynamic_cast<const ASceneEntity*>(eventListener);
		ASSERT(entity);

		if(entity->IsPersistent())
		{
			oWiz.Put(static_cast<FieldID>(kFirstListener + listenerIdx), entity->GetConstSceneEntityStateAccessor()->GetName());

			++count;
		}
	}

	oWiz.Put(kListenerCount, count);

	const FieldID	kNameSequenceGenerator = static_cast<FieldID>(kFirstListener + count);

	oWiz.Put(kNameSequenceGenerator, mNameSeqGen);
}
// --------------------------------------------------------------------------
void SceneEntityState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kName, mName);
	iWiz.Get(kIsVisible, mIsVisible);

	bool	hasParent;
	
	iWiz.Get(kHasParent, hasParent);
	
	if(hasParent)
	{
		ROSString	name;

		iWiz.Get(kParentName, name);

		SceneEntityRemapper::Add(name, new SceneEntityRemap<ASceneEntity*>(&mParentEntity));
	}
	else
	{
		mParentEntity = NULL;
	}

	// Read roles
	unsigned int	roleCount;

	iWiz.Get(kRoleCount, roleCount);

	RoleList::iterator				begin = mRoles.begin();
	const RoleList::const_iterator	end = mRoles.end();
	unsigned int					roleIdx = 0;

	while(begin != end && roleIdx < roleCount)
	{
		const FieldID	fieldID = static_cast<FieldID>(kFirstRole + roleIdx);

		if(iWiz.Has(fieldID))
		{
			iWiz.Get(fieldID, **begin);
		}
		
		++begin;
		++roleIdx;
	}

	// Read sources
	const FieldID	kSourceCount = static_cast<FieldID>(kFirstRole + roleIdx);
	unsigned int	sourceCount;

	iWiz.Get(kSourceCount, sourceCount);

	const FieldID	kFirstSource = static_cast<FieldID>(kSourceCount + 1);

	mEventSourceTracker.Clear();

	for(unsigned int sourceIdx = 0; sourceIdx < sourceCount; ++sourceIdx)
	{
		ROSString	entityName;

		iWiz.Get(static_cast<FieldID>(kFirstSource + sourceIdx), entityName);

		SceneEntityRemapper::Add(entityName, new SceneEntityAddSourceRemap(*this));
	}

	// Read listeners
	const FieldID	kListenerCount = static_cast<FieldID>(kFirstSource + sourceIdx);
	unsigned int	listenerCount;

	iWiz.Get(kListenerCount, listenerCount);

	const FieldID	kFirstListener = static_cast<FieldID>(kListenerCount + 1);

	mEventListenerTracker.Clear();

	for(unsigned int listenerIdx = 0; listenerIdx < listenerCount; ++listenerIdx)
	{
		ROSString	entityName;

		iWiz.Get(static_cast<FieldID>(kFirstListener + listenerIdx), entityName);

		SceneEntityRemapper::Add(entityName, new SceneEntityAddListenerRemap(*this));
	}

	const FieldID	kNameSequenceGenerator = static_cast<FieldID>(kFirstListener + listenerIdx);

	iWiz.Get(kNameSequenceGenerator, mNameSeqGen);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
