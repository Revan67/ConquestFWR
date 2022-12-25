// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DAAudioObject.h"
#include "StaticSceneEntitySoundSource.h"
#include "StaticSceneEntitySoundListener.h"
#include "Misc.h"
#include "Engine.h"
#include "ISoundManager.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
ISystemContainer*				SYS = NULL;
IEngine*						ENG = NULL;
ISoundManager*					SOUND = NULL;
StaticSceneEntitySoundListener*	gListener = NULL;
// --------------------------------------------------------------------------
namespace ROS
{
class AStaticSceneEntity;
}
// --------------------------------------------------------------------------
inline const ROS::DAAudioObject* GetDAAudioObject(StaticSceneEntitySoundSource* soundSource)
{
	return reinterpret_cast<const ROS::DAAudioObject*>(soundSource);
}
// --------------------------------------------------------------------------
inline StaticSceneEntitySoundSource* GetStaticSceneEntitySoundSource(const ROS::DAAudioObject* audioObj)
{
	return const_cast<StaticSceneEntitySoundSource*>(reinterpret_cast<const StaticSceneEntitySoundSource*>(audioObj));
}
// --------------------------------------------------------------------------
bool AudioObjectSystemStartup(HWND hWnd, ISystemContainer* system, IEngine* engine)
{
	ASSERT(hWnd && system && engine);

	SYS = system;
	ENG = engine;

	SYS->QueryInterface(IID_ISoundManager, (void**)&SOUND);

	if(SOUND)
	{
		if(SOUND->startup(hWnd, SM_DEFAULT_SETTINGS) == GR_OK)
		{
			gListener = new StaticSceneEntitySoundListener(NULL);
			
			return true;
		}
	}

	return false;
}
// --------------------------------------------------------------------------
void AudioObjectSystemShutdown()
{
	ASSERT(SOUND);

	gListener->Release();
	gListener = NULL;

	SOUND->shutdown();
	SOUND->Release();
	SOUND = NULL;
}
// --------------------------------------------------------------------------
void AudioObjectSystemUpdate()
{
	ASSERT(SOUND && gListener);

	SOUND->update(gListener);
}
// --------------------------------------------------------------------------
const ROS::DAAudioObject* AudioObjectCreate(const char* filename, float attenuation, float minDistance, float maxDistance, const ROS::AStaticSceneEntity* staticSE)
{
	const ROS::DAAudioObject*	audioObject = NULL;
	
	COMPTR<IFileSystem> fileSys;

	if(GR_OK == ENG->create_file_system (filename, fileSys))
	{
		const SOUND_ARCH_INDEX	arch = SOUND->create_archetype(fileSys, SM_ARCHETYPE_DEFAULT | SM_ENABLE_STICKY_FOCUS);

		if(arch != SM_INVALID_ARCHETYPE)
		{
			try
			{
				const U32	startTime = SOUND->get_current_time_ms();

				StaticSceneEntitySoundSource*	soundSource = new StaticSceneEntitySoundSource(staticSE, arch, startTime);

				soundSource->set_attenuation(attenuation);
				soundSource->set_min_distance(minDistance);
				soundSource->set_max_distance(maxDistance);

				return GetDAAudioObject(soundSource);
			}
			catch(...)
			{
				SOUND->destroy_archetype(arch);
			}
		}
	}

	return NULL;
}
// --------------------------------------------------------------------------
void AudioObjectDestroy(const ROS::DAAudioObject* audioObj)
{
	StaticSceneEntitySoundSource*	soundSource = GetStaticSceneEntitySoundSource(audioObj);

	SOUND->remove_active_sound(soundSource);

	const SOUND_ARCH_INDEX	arch = soundSource->get_archetype();

	SOUND->destroy_archetype(arch);

	soundSource->Release();
}
// --------------------------------------------------------------------------
void AudioObjectPlay(const ROS::DAAudioObject* audioObj, float startTimePoint)
{
	StaticSceneEntitySoundSource*	soundSource = GetStaticSceneEntitySoundSource(audioObj);
	ASSERT(soundSource);

	const U32	startTime = SOUND->get_current_time_ms() - (startTimePoint * 1000);	// subtracting because sound manager expects start time relative to its current time

	soundSource->set_start_time(startTime);

	const GENRESULT	result = SOUND->add_active_sound(soundSource, SOUND->get_active_sound_count());
	ASSERT(result == GR_OK);
}
// --------------------------------------------------------------------------
void AudioObjectStop(const ROS::DAAudioObject* audioObj)
{
	StaticSceneEntitySoundSource*	soundSource = GetStaticSceneEntitySoundSource(audioObj);
	ASSERT(soundSource);

	const GENRESULT	result = SOUND->remove_active_sound(soundSource);
	ASSERT(result == GR_OK);
}
// --------------------------------------------------------------------------
void AudioObjectListenerSet(const ASoundListener* targetListener)
{
	if(gListener)
	{
		gListener->set_target(targetListener);
	}
}
// --------------------------------------------------------------------------
const ASoundListener* AudioObjectListenerGet()
{
	if(gListener)
	{
		return gListener->get_target();
	}
	else
	{
		return NULL;
	}
}
// --------------------------------------------------------------------------
