
struct SoundSource : public ISoundSource
{
protected:
	U32 m_startTime;
	bool m_on;
	bool m_3D;
	bool m_loop;
	SOUND_ARCH_INDEX m_archetype;
	SINGLE m_attenuation;
	SINGLE m_minDistance;
	SINGLE m_maxDistance;
	Vector m_position;
	Vector m_velocity;
	U32 m_refCount;

public:
	SoundSource(U32 startTime, SOUND_ARCH_INDEX archetype);
	virtual U32 get_start_time() { return m_startTime; };
	virtual bool COMAPI is_on() { return m_on; };
	virtual bool COMAPI is_3D() { return m_3D; };
	virtual bool COMAPI is_looping() { return m_loop; };
	virtual SOUND_ARCH_INDEX get_archetype() { return m_archetype; };
	virtual GENRESULT COMAPI get_attenuation(SINGLE *s) { s = &m_attenuation; return GR_OK; }; 
	virtual GENRESULT COMAPI get_frequency(SINGLE *s) { return GR_NOT_IMPLEMENTED; };
	virtual GENRESULT COMAPI get_pan(S32 *) { return GR_NOT_IMPLEMENTED; };
	virtual GENRESULT COMAPI get_position(Vector *v) { v = &m_position; return GR_OK; };
	virtual GENRESULT COMAPI get_min_distance(SINGLE *s) { s = &m_minDistance; return GR_OK; };
	virtual GENRESULT COMAPI get_max_distance(SINGLE *s) { s = &m_maxDistance; return GR_OK; };
	virtual GENRESULT COMAPI get_cone_angles(DWORD *insideAngle, DWORD *outsideAngle) { return GR_NOT_IMPLEMENTED; }; 
	virtual GENRESULT COMAPI get_cone_orientation(Vector *) { return GR_NOT_IMPLEMENTED; }; 
	virtual GENRESULT COMAPI get_cone_outside_attenuation(SINGLE *) { return GR_NOT_IMPLEMENTED; };
	virtual GENRESULT COMAPI get_velocity(Vector *v) { v = &m_velocity; return GR_OK; };
	virtual GENRESULT COMAPI get_reverb_mix(SINGLE *) { return GR_NOT_IMPLEMENTED; };
	virtual GENRESULT COMAPI get_sound_mode(S32 *) { return GR_NOT_IMPLEMENTED; };
	virtual GENRESULT COMAPI get_apply_mode(S32 *) { return GR_NOT_IMPLEMENTED; };

	virtual GENRESULT COMAPI QueryInterface(const char *n, void **i);
	virtual U32 COMAPI AddRef();
	virtual U32 COMAPI Release();

	void set_start_time(U32 time) { m_startTime = time; };
	void set_on(bool state) { m_3D = state; };
	void set_3D(bool state) { m_on = state; };
	void set_looping(bool state) { m_loop = state; };
	void set_attenuation(SINGLE attenuation) { m_attenuation = attenuation; };
	void set_min_distance(SINGLE dist) { m_minDistance = dist; };
	void set_max_distance(SINGLE dist) { m_maxDistance = dist; };
	void set_position(Vector v) { m_position = v; };
	void set_velocity(Vector v) { m_velocity = v; };
};

SoundSource::SoundSource(U32 startTime, SOUND_ARCH_INDEX archetype = SM_INVALID_ARCHETYPE)
{
	m_startTime = startTime;
	m_on = true;
	m_3D = true;
	m_loop = false;
	m_archetype = archetype;
	m_attenuation = 1.0;
	m_minDistance = 5.0;	// at twice this distance, the sound's volume will be halved
	m_maxDistance = 100.0;	// beyond this distance, the sound will be inauble or remain at a constant volume
	m_position.set(0,0,0);
	m_velocity.set(0,0,0);
	m_refCount = 1;
}

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

struct Listener: public ISoundListener
{
protected:
	Matrix m_orientation;
	Vector m_position;
	Vector m_velocity;
	SINGLE m_distanceFactor;
	SINGLE m_dopplerFactor;
	SINGLE m_rolloffFactor;
	U32 m_refCount;

public:
	Listener(Matrix &orientation, Vector &position);
	virtual GENRESULT COMAPI get_ear_orientation(Matrix * orientation) { orientation = &m_orientation; return GR_OK; };
	virtual GENRESULT COMAPI get_ear_position(Vector *position) { position = &m_position; return GR_OK; };
	virtual GENRESULT COMAPI get_ear_velocity(Vector *velocity) { velocity = &m_velocity; return GR_OK; };
	virtual GENRESULT COMAPI get_ear_distance_factor(SINGLE *distance) { distance = &m_distanceFactor; return GR_OK; };
	virtual GENRESULT COMAPI get_ear_doppler_factor(SINGLE *doppler) { doppler = &m_dopplerFactor; return GR_OK; };
	virtual GENRESULT COMAPI get_ear_rolloff_factor(SINGLE *rolloff) { rolloff = &m_rolloffFactor; return GR_OK; };

	virtual GENRESULT COMAPI QueryInterface(const char *n, void **i);
	virtual U32 COMAPI AddRef();
	virtual U32 COMAPI Release();
};

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
