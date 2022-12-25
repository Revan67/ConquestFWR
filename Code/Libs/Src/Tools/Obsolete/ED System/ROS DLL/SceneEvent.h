// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneEvent_h
#define SceneEvent_h
// --------------------------------------------------------------------------
#include "Typedefs.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// SceneEvent
// --------------------------------------------------------------------------
typedef U32 SceneEvent;

// A SceneEvent can have one of the following vaules
static const SceneEvent	kSECompoundSceneEntityConstruct				= 0;
static const SceneEvent	kSECompoundSceneEntityDelete				= 1;
static const SceneEvent	kSEDeformableSceneEntityConstruct			= 2;
static const SceneEvent	kSEDeformableSceneEntityDelete				= 3;
static const SceneEvent	kSEBaseCameraConstruct						= 4;
static const SceneEvent	kSEBaseCameraDelete							= 5;
static const SceneEvent	kSEInternalBaseCameraConstruct				= 6;
static const SceneEvent	kSEInternalBaseCameraDelete					= 7;
static const SceneEvent	kSEAudioObjectConstruct						= 8;
static const SceneEvent	kSEAudioObjectDelete						= 9;
static const SceneEvent	kSEAudioObjectPlay							= 10;
static const SceneEvent	kSEAudioObjectStop							= 11;
static const SceneEvent	kSECompoundSceneEntityUpdateDescription		= 12;	
static const SceneEvent	kSEDeformableSceneEntityUpdateDescription	= 13;			
static const SceneEvent	kSEAudioEntityUpdateDescription				= 14;
static const SceneEvent	kSEAudioGetEntityDBAudioFilename			= 15;
static const SceneEvent kSESceneEvent								= 16;

// --------------------------------------------------------------------------
// SceneEventFlag
// --------------------------------------------------------------------------
typedef U32 SceneEventFlag;

static const SceneEventFlag	kSEFNone = 0;	// Use this to initialize flags

// The flags are divided into two groups. The two groups should not be mixed.
// The first group is for the purpose of the client sending information to
// ROS, while the second, for ROS to send information to the client.

//
// Flags client can specify to ROS
//
// A SceneEventFlag can have any combination of the following flags by
// performing a bitwise OR
static const SceneEventFlag	kSEFUseStateInScriptAsInitialState		= 1;
static const SceneEventFlag	kSEFUseTransitionInScriptTheFirstTime	= 2;
static const SceneEventFlag kSEFUseFloorHeightInScript				= 4;

//
// Flags ROS can specify to client
//
// A SceneEventFlag can have any combination of the following flags by
// performing a bitwise OR
static const SceneEventFlag	kSEFScriptHasLiveCamera	= 1;
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

