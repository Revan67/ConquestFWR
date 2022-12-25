


inline CStopWatch::CStopWatch() 
{
	m_UseQPC = false;
	m_Frequency = 1.0;

#if 1
	if( QueryPerformanceFrequency( (LARGE_INTEGER*)&m_Frequency ) != 0 ) {
		m_UseQPC = true;
	}
#endif
	memset( &m_Watches[0], 0, sizeof(SWTIMERDESC)*SW_MAX_WATCHES );
}


inline SWID	CStopWatch::AddWatch( void *data ) 
{
	data; // bonk the 'data' : unreferenced formal parameter

	for( U32 w=0; w<SW_MAX_WATCHES; w++ ) {
		if( m_Watches[w].in_use == 0 ) {
			m_Watches[w].in_use = 1;
			return (SWID)w;
		}
	}

	return SWID_INVALID;
}


inline void	CStopWatch::RemoveWatch( SWID id  ) 
{
	m_Watches[(U32)id].in_use = 0;
}


inline LONGLONG CStopWatch::sample()
{
	LONGLONG sam;

	if( m_UseQPC ) {
		QueryPerformanceCounter( (LARGE_INTEGER*) &sam );	
	}
	else {
		sam = (LONGLONG)timeGetTime();
	}

	return sam;
}

inline void	CStopWatch::Start( SWID id  ) 
{
	m_Watches[(U32)id].ll_start = sample();
	m_Watches[(U32)id].ll_stop = 0;
}


inline void	CStopWatch::Stop( SWID id  ) 
{
	m_Watches[(U32)id].ll_stop = sample();
}


inline float CStopWatch::GetElapsedSeconds( SWID id  ) 
{
	LONGLONG ll_sample;
	if( m_Watches[(U32)id].ll_stop != 0 ) {
		ll_sample = m_Watches[(U32)id].ll_stop;
	}
	else {
		ll_sample = sample();
	}

	ll_sample = ll_sample - m_Watches[(U32)id].ll_start;
	if( m_UseQPC ) { 
		return (float)((double)ll_sample/(double)m_Frequency);
	}	
	return (float)((double)ll_sample/1000.00);
}


inline float CStopWatch::GetElapsedMSeconds( SWID id  ) 
{
	LONGLONG ll_sample;
	if( m_Watches[(U32)id].ll_stop != 0 ) {
		ll_sample = m_Watches[(U32)id].ll_stop;
	}
	else {
		ll_sample = sample();
	}

	ll_sample = ll_sample - m_Watches[(U32)id].ll_start;
	if( m_UseQPC ) { 
		return (float)(ll_sample/(m_Frequency/1000));
	}	
	return (float)ll_sample;
}


inline float CStopWatch::GetElapsedUSeconds( SWID id  ) 
{
	LONGLONG ll_sample;
	if( m_Watches[(U32)id].ll_stop != 0 ) {
		ll_sample = m_Watches[(U32)id].ll_stop;
	}
	else {
		ll_sample = sample();
	}

	ll_sample = ll_sample - m_Watches[(U32)id].ll_start;
	if( m_UseQPC ) { 
		return (float)(ll_sample/(m_Frequency/1000000));
	}	
	return (float)(ll_sample * 1000.00);
}



