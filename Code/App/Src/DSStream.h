#ifndef DSSTREAM_H
#define DSSTREAM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSStream.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/DSStream.h 9     12/14/00 9:42a Jasony $


	Wrapper around DirectShow Multi-media streaming for both audio and video.
*/
//--------------------------------------------------------------------------//
//
#ifndef TSMARTPOINTER_H
#include <TSmartPointer.h>
#endif

#ifndef DACOM_H
#include <DACOM.h>
#endif

#include "HeapObj.h"

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DSStream : IDAComponent
{
	enum STATE
	{
		STOPPED,
		OPENPENDING,
		PAUSED,
		RUNNING,
		ABORTED
	};

	DSStream (void);

	~DSStream (void);

	/* Streaming methods */

	bool Open (struct IFileSystem * parentDir, const char * fileName);

	void Stop (void);		// use when done playing this stream

	void Pause (void);		// use when you might want to restart later

	void Run (void);

	void SetVolume (S32 dsVolume);

	void SetPosition (S32 x, S32 y);

	void SetPosition (const RECT& rc);

	void GetEvent (void);

	void Blit (void);

	/* access methods */

	STATE GetState (void) const
	{
		return state;
	}

	S32 GetDSVolume (void) const
	{
		return volume;
	}

	bool IsValid (void) const
	{
		return bInitialized;
	}

	/* IDAComponent methods */

	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		*instance = 0;
		return GR_GENERIC;
	}
	
	DEFMETHOD_(U32,AddRef)           (void)
	{
		return ++refCount;
	}
	
	DEFMETHOD_(U32,Release)          (void)
	{
		if (refCount && --refCount == 0)
		{
			refCount+=2;
			delete this;
			return 0;
		}
		return refCount;
	}

	void ThreadedOpen (const char * pathName);		// called in background thread
	
	void ThreadedReset (void);

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "DSStream");
	}

private:
	COMPTR<struct IGraphBuilder> pGB;
	COMPTR<struct IMediaEventEx> pME;
    COMPTR<struct IAMMultiMediaStream> pAMStream;
	COMPTR<struct IDirectDraw4> pDD4;
    COMPTR<struct IDirectDrawStreamSample> pSample;

	DSStream * pNext;
	bool bInitialized;
	U32 refCount;
	S32 posX, posY;
	bool bUseRect;
	RECT rect;

	STATE state, pendingState;
	S32 volume;

	DSStream (const DSStream & new_ptr);
	DSStream & operator = (const DSStream & new_ptr);

	HRESULT createDDSample (void);
};


//----------------------------------------------------------------------------//
//------------------------------END DSStream.h--------------------------------//
//----------------------------------------------------------------------------//
#endif