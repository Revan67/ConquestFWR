// StopWatch.h
//
//
//

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "typedefs.h"

#include <mmsystem.h>

typedef unsigned int SWID;

const SWID_INVALID = 0xFFFFFFFF;

const SW_MAX_WATCHES = 10;

class CStopWatch 
{
public:
	SWID	AddWatch( void *data = NULL );
	void	RemoveWatch( SWID id = 0 );

	void	CStopWatch::Start( SWID id = 0 );
	float	GetElapsedSeconds( SWID id = 0 );
	float	GetElapsedMSeconds( SWID id = 0 );
	float	GetElapsedUSeconds( SWID id = 0 );
	void	Stop( SWID id = 0 );

	CStopWatch();
protected:
	bool m_UseQPC;
	
	LONGLONG sample();

	LONGLONG m_Frequency;

	struct SWTIMERDESC {
		U32		 in_use;
		LONGLONG ll_start;
		LONGLONG ll_stop;
		void	*data;
	};

	SWTIMERDESC m_Watches[SW_MAX_WATCHES];
};

#include "StopWatch_inl.cpp"

#endif

