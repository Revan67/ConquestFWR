#include <windows.h>
#include "DACOM.h"
#include "TComponent.h"
#include "simpleSource.h"

SoundSource::SoundSource()
{
	m_startTime = 0;
	m_on = true;
	m_3D = true;
	m_loop = false;
	m_archetype = 0;
	m_attenuation = 0.0;
	m_minDistance = 5.0;	// at twice this distance, the sound's volume will be halved
	m_maxDistance = 100.0;	// beyond this distance, the sound will be inauble or remain at a constant volume
	m_position.set(0,0,0);
	m_velocity.set(0,0,0);
	m_refCount = 1;
}

SoundSource::SoundSource(U32 startTime, SOUND_ARCH_INDEX archetype = SM_INVALID_ARCHETYPE)
{
	m_startTime = startTime;
	m_on = true;
	m_3D = true;
	m_loop = false;
	m_archetype = archetype;
	m_attenuation = 0.0;
	m_minDistance = 5.0;	// at twice this distance, the sound's volume will be halved
	m_maxDistance = 100.0;	// beyond this distance, the sound will be inauble or remain at a constant volume
	m_position.set(0,0,0);
	m_velocity.set(0,0,0);
	m_refCount = 1;
}

GENRESULT COMAPI SoundSource::get_position(Vector *v) 
{ 
	*v = m_position; 
	return GR_OK; 
}

/*
GENRESULT SoundSource::QueryInterface(const char *n, void ** i) 
{
	return GR_INTERFACE_UNSUPPORTED;
}

U32 SoundSource::AddRef() 
{
	return ++m_refCount;
}

U32 SoundSource::Release() 
{
	m_refCount--;

	if (m_refCount == 0) 
	{
		delete this;
	}

	return m_refCount;
}
*/

Listener::Listener(Matrix &orientation, Vector &position)
{
	m_orientation = orientation;
	m_position = position;
	m_velocity.set(0,0,0);
	m_distanceFactor = 1.0;
	m_dopplerFactor = 1.0;
	m_rolloffFactor = 1.0;
	m_refCount = 1;
}

/*
GENRESULT Listener::QueryInterface(const char *n, void ** i) 
{
	return GR_INTERFACE_UNSUPPORTED;
}

U32 Listener::AddRef() 
{
	return ++m_refCount;
}

U32 Listener::Release() 
{
	m_refCount--;

	if (m_refCount == 0) 
	{
		delete this;
	}

	return m_refCount;
}
*/