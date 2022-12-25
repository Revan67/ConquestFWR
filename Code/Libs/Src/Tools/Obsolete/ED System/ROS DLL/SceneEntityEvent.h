// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneEntityEvent_h
#define SceneEntityEvent_h
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ASceneEntity;
// --------------------------------------------------------------------------
class SceneEntityEvent
{
	public:
		enum ID
		{
			kSourceEntityDeleted
		};

		SceneEntityEvent(ID id, ASceneEntity& sourceEntity)
		:mID(id), mSourceEntity(sourceEntity)
		{
		}

		ID GetID() const
		{
			return mID;
		}

		ASceneEntity& GetSourceEntity() const
		{
			return mSourceEntity;
		}

	private:
		ID				mID;
		ASceneEntity&	mSourceEntity;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif