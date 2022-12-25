#ifndef ModelNS_h
#define ModelNS_h
// --------------------------------------------------------------------------
namespace ModelNS
{  	enum UpdateID
    {	kAll,
        kEntitySelectionChanged,
		kEntitySelectionLockUpdated,
        kSelectedEntityUpdated,
		kSelectedEntityRemoved,
		kSecondaryEntityUpdated,
        kSecondaryDependentEntityUpdated,
		kEntityAdded,
		kEntityRemoved,			// Used by Scene to send a message to SceneModel
		kScenePaused,
		kSceneDurationUpdated,
        kSceneCurrentTimePointUpdated,
        kSceneModelReplaced,
        kSceneModelDeleted,
        kSceneSystemShutdown
    };
}
// --------------------------------------------------------------------------
#endif

