#ifndef TRESCLIENT_H
#define TRESCLIENT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                TResClient.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TResClient.h 9     8/21/00 5:27p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef IRESOURCE_H
#include "IResource.h"
#endif

#ifndef CURSOR_H
#include "Cursor.h"
#endif

#ifndef STATUSBAR_H
#include "StatusBar.h"
#endif

#ifndef HINTBOX_H
#include "Hintbox.h"
#endif

#ifndef CQTRACE_H
#include "CQTrace.h"
#endif

#ifndef MISSION_H
#include "Mission.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define RF_CURSOR  0x00000001
#define RF_STATUS  0x00000002
#define RF_HINTBOX 0x00000004	
#define RF_ALL     0x00000007

template <class Base = IResourceClient> 
struct DACOM_NO_VTABLE ResourceClient : public Base
{
	U32 statusHandle, cursorHandle, hintboxHandle;

	U32 actualOwnedFlags;
	U32 desiredOwnedFlags;
	U32 resPriority;
	U32 cursorID, toolTextID, statusTextID, hintboxID;
#ifndef FINAL_RELEASE
private:
	U32 recurseCount;
public:	
#endif
	//------------------------
	// public methods
	//------------------------

	ResourceClient (void)
	{
		resPriority = RES_PRIORITY_MEDIUM;
	}

	virtual ~ResourceClient (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (STATUS && CURSOR && HINTBOX)
			releaseResources();

		if (statusHandle && STATUS->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
			connection->Unadvise(statusHandle);
		if (cursorHandle && CURSOR->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
			connection->Unadvise(cursorHandle);
		if (hintboxHandle && HINTBOX->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
			connection->Unadvise(hintboxHandle);
	}

	void initializeResources (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (STATUS->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
			connection->Advise(this, &statusHandle);
		if (CURSOR->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
			connection->Advise(this, &cursorHandle);
		if (HINTBOX->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
			connection->Advise(this, &hintboxHandle);
	}

	/* IResourceClient method */

	DEFMETHOD_(BOOL32,Notify) (struct IResource *res, U32 message, void *parm)
	{
#ifndef FINAL_RELEASE
		if (recurseCount++>2)
		{
			CQASSERT(HEAP->EnumerateBlocks());
			CQBOMB1("recurseCount>2 in resourceClient!\nStatusTextID=%d", statusTextID);
		}
#endif
		switch (message)
		{
		case RESMSG_RES_CLOSING:
			if (res == STATUS)
			{
				desiredOwnedFlags &= ~RF_STATUS;
				actualOwnedFlags &= ~RF_STATUS;
				statusHandle = 0;
			}
			else
			if (res == CURSOR)
			{
				desiredOwnedFlags &= ~RF_CURSOR;
				actualOwnedFlags &= ~RF_CURSOR;
				cursorHandle = 0;
			}
			else
			if (res == HINTBOX)
			{
				desiredOwnedFlags &= ~RF_HINTBOX;
				actualOwnedFlags &= ~RF_HINTBOX;
				hintboxHandle = 0;
			}
			break;

		case RESMSG_RES_LOST:
			if (res == STATUS)
				actualOwnedFlags &= ~RF_STATUS;
			else
			if (res == CURSOR)
				actualOwnedFlags &= ~RF_CURSOR;
			else
			if (res == HINTBOX)
				actualOwnedFlags &= ~RF_HINTBOX;

			if (ownsResources() == 0)
				releaseResources();
			break;

		case RESMSG_RES_UNOWNED:
			if (res == CURSOR)
			{
				if (desiredOwnedFlags & RF_CURSOR)
				{
					CURSOR->SetDefaultState();
					grabAllResources();	// grab all or nothing
				}
			}
			else
			if (res == STATUS)
			{
				if (desiredOwnedFlags & RF_STATUS)
				{
					STATUS->SetDefaultState();
					grabAllResources();	// grab all or nothing
				}
			}
			else
			if (res == HINTBOX)
			{
				if (desiredOwnedFlags & RF_HINTBOX)
				{
					HINTBOX->SetDefaultState();
					grabAllResources();	// grab all or nothing
				}
			}
			break;
		} // end switch(message)

#ifndef FINAL_RELEASE
		recurseCount--;
#endif
		return 1;
	}

	/* ResourceClient methods */

	virtual BOOL32 grabAllResources (void)	// grab all or nothing
	{
		BOOL32 result;

		if ((result = ownsResources()) == 0)
		{
			if ((desiredOwnedFlags & RF_CURSOR) && (actualOwnedFlags & RF_CURSOR)==0)
			{
				if (CURSOR->GetOwnership(this, resPriority))
					actualOwnedFlags |= RF_CURSOR;
				else
					goto Done;
			}
			if ((desiredOwnedFlags & RF_STATUS) && (actualOwnedFlags & RF_STATUS)==0)
			{
				if (STATUS->GetOwnership(this, resPriority))
					actualOwnedFlags |= RF_STATUS;
				else
					goto Done;
			}
			if ((desiredOwnedFlags & RF_HINTBOX) && (actualOwnedFlags & RF_HINTBOX)==0)
			{
				if (HINTBOX->GetOwnership(this, resPriority))
					actualOwnedFlags |= RF_HINTBOX;
				else
					goto Done;
			}
			setResources();
			result = 1;		// we now own everything!!
		}
Done:
		if (result == 0)
			releaseResources();
		return result;
	}

	void setResources (void)
	{
		if (actualOwnedFlags & RF_STATUS)
			setStatus();
		if (actualOwnedFlags & RF_CURSOR)
			setCursor();
		if (actualOwnedFlags & RF_HINTBOX)
			setHintbox();
	}

	BOOL32 ownsResources (void)
	{
		return (desiredOwnedFlags && (desiredOwnedFlags == (desiredOwnedFlags & actualOwnedFlags)));
	}

	void releaseResources (void)
	{
		if (actualOwnedFlags & RF_STATUS)
			STATUS->ReleaseOwnership(this);
		if (actualOwnedFlags & RF_CURSOR)
			CURSOR->ReleaseOwnership(this);
		if (actualOwnedFlags & RF_HINTBOX) 
			HINTBOX->ReleaseOwnership(this);
		actualOwnedFlags &= ~(RF_STATUS|RF_CURSOR|RF_HINTBOX);
	}


	virtual void setStatus (void)
	{
		if (statusTextID)
			STATUS->SetText(statusTextID);
		else
			STATUS->SetDefaultState();
	}

	virtual void setCursor (void)
	{
		if (cursorID)
			CURSOR->SetCursor(cursorID);
		else
			CURSOR->SetDefaultState();
	}

	virtual void setHintbox (void)
	{
		if (hintboxID)
			HINTBOX->SetText(hintboxID);
	}

};

//--------------------------------------------------------------------------//
//---------------------------End TResClient.h-------------------------------//
//--------------------------------------------------------------------------//
#endif
