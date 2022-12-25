// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DAAudioObject_h
#define DAAudioObject_h

#define WIN32_LEAN_AND_MEAN
#include <afx.h>

namespace ROS
{
class DAAudioObject;
}

struct ISystemContainer;
struct IEngine;
class ASoundListener;

// Audio system management
bool AudioObjectSystemStartup(HWND hWnd, ISystemContainer* system, IEngine* engine);

void AudioObjectSystemShutdown();

void AudioObjectSystemUpdate();

// Audio object management
const ROS::DAAudioObject* AudioObjectCreate(const char* filename, float attenuation, float minDistance, float maxDistance, const ROS::AStaticSceneEntity* staticSE);

void AudioObjectDestroy(const ROS::DAAudioObject* audioObj);

void AudioObjectPlay(const ROS::DAAudioObject* audioObj, float startTimePoint);

void AudioObjectStop(const ROS::DAAudioObject* audioObj);

// Listener management
void AudioObjectListenerSet(const ASoundListener* targetListener);

const ASoundListener* AudioObjectListenerGet();

#endif