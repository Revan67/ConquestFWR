//--------------------------------------------------------------------------//
//                                                                          //
//                               GroupObj.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/GroupObj.cpp 19    10/17/00 6:24p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "ObjList.h"
#include "Startup.h"
#include <DGroup.h>
#include "MGlobals.h"
#include "Mission.h"
#include "IGroup.h"
#include "MPart.h"
#include "OpAgent.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>

#define IDLE_TIMEOUT (60 * RENDER_FRAMERATE * 3)		// 3 minutes, in real time
#define DEAD_TIMEOUT (60 * RENDER_FRAMERATE * 5)		// 5 minutes, in real time

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE GroupObj : public IBaseObject, IGroup, ISaveLoad, GROUPOBJ_SAVELOAD
{
	BEGIN_MAP_INBOUND(GroupObj)
	_INTERFACE_ENTRY(IGroup)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IBaseObject)
	END_MAP()

	U32 *objectID;

	mutable U32  idleTime;
	bool bDeathScheduled;		// already scheduled for death
	bool bDeathEnabled;			// enabled by host

	static U32 currentTick;
	static U32 lastGameTick;

	GroupObj (void) 
	{
	}

	virtual ~GroupObj (void);	// See ObjList.cpp
	
	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual U32 GetPartID (void) const;
	
	virtual U32 GetPlayerID (void) const;

	/* IGroup methods */

	virtual void InitGroup (const U32 *pObjIDs, U32 numObjects, U32 dwMissionID);

	virtual bool TestGroup (const U32 *pObjIDs, U32 numObjects) const;

	virtual U32 GetObjects (U32 pObjIDs[MAX_SELECTED_UNITS]) const;

	virtual void EnableDeath (void);

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* GroupObj methods */

	void init (void)
	{
	}

	static U32 getTickCount (void)
	{
		return MGlobals::GetUpdateCount();
	}
};
U32 GroupObj::currentTick;
U32 GroupObj::lastGameTick;
//---------------------------------------------------------------------------
//
GroupObj::~GroupObj (void)
{
	delete [] objectID;

	if (dwMissionID)
		OBJLIST->RemoveGroupPartID(this, dwMissionID);
}
//---------------------------------------------------------------------------
//
BOOL32 GroupObj::Update (void)
{
	BOOL32 result=1;
	
	if (dwMissionID)		// is this a networked group?
	{
		if (idleTime == 0)
		{
			idleTime = getTickCount();
		}
		else
		{
			const U32 newTickCount = getTickCount();
			const S32 timeDiff = newTickCount - idleTime;

			if (bDeathScheduled==false && timeDiff >= IDLE_TIMEOUT)
			{
				if (bDeathEnabled||THEMATRIX->IsMaster())
				{
					if (bDeathEnabled==false)		// we are on the master machine
						THEMATRIX->SignalStaleGroupObject(dwMissionID);
					bDeathScheduled = true;				
				}
			}
			else
			if (bDeathScheduled && timeDiff >= DEAD_TIMEOUT)
			{
				result = 0;		// delete the group
			}
			else
			if (bDeathScheduled)
				result = 2;		// signal OBJLIST to continue enumeration
		}
	}		

	return result;
}
//---------------------------------------------------------------------------
//
U32 GroupObj::GetPartID (void) const
{
	return dwMissionID;
}
//---------------------------------------------------------------------------
//
U32 GroupObj::GetPlayerID (void) const
{
	return MGlobals::GetPlayerFromPartID(dwMissionID);
}
//---------------------------------------------------------------------------
//
void GroupObj::InitGroup (const U32 *pObjIDs, U32 _numObjects, U32 _dwMissionID)
{
	CQASSERT(objectID == 0 || dwMissionID==0);		// only allow non-networked groups to be redefined

	numObjects = _numObjects;
	delete [] objectID;
	objectID = new U32[numObjects];

	memcpy(objectID, pObjIDs, numObjects*sizeof(U32));

	dwMissionID = _dwMissionID;	// MGlobals::CreateNewPartID(MGlobals::GetGroupID());
	OBJLIST->AddGroupPartID(this, dwMissionID);
	idleTime = getTickCount();		// reset the idle timer if we are still in use
}
//---------------------------------------------------------------------------
//
bool GroupObj::TestGroup (const U32 *pObjIDs, U32 _numObjects) const
{
	bool result=false;

	if (dwMissionID && bDeathScheduled==false && bDeathEnabled==false && numObjects == _numObjects)
		if ((result = (memcmp(objectID, pObjIDs, numObjects*sizeof(U32)) == 0)) != 0)
			idleTime = getTickCount();		// reset the idle timer if we are still in use

	return result;
}
//---------------------------------------------------------------------------
//
U32 GroupObj::GetObjects (U32 pObjIDs[MAX_SELECTED_UNITS]) const
{
	memcpy(pObjIDs, objectID, sizeof(U32)*numObjects);
	return numObjects;
}
//---------------------------------------------------------------------------
//
void GroupObj::EnableDeath (void)
{
	CQASSERT(THEMATRIX->IsMaster()==false);
	bDeathEnabled = true;
}
//---------------------------------------------------------------------------
//
BOOL32 GroupObj::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "GROUPOBJ_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	GROUPOBJ_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	save = *static_cast<GROUPOBJ_SAVELOAD *>(this);
	
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	file->WriteFile(0, objectID, numObjects * sizeof(U32), &dwWritten, 0);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 GroupObj::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "GROUPOBJ_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	GROUPOBJ_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("GROUPOBJ_SAVELOAD", buffer, &load);

	*static_cast<GROUPOBJ_SAVELOAD *>(this) = load;

	dwRead -= sizeof(U32)*load.numObjects;

	objectID = new U32[load.numObjects];

	memcpy(objectID, buffer+dwRead, sizeof(U32)*load.numObjects);

	OBJLIST->AddGroupPartID(this, load.dwMissionID);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void GroupObj::ResolveAssociations (void)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createGroupObj (PARCHETYPE pArchetype)
{
	GroupObj * obj = new ObjectImpl<GroupObj>;
	obj->init();

	obj->objClass = OC_GROUP;
	obj->pArchetype = pArchetype;

	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------GroupObj Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE GroupObjFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		void   operator delete (void *ptr)
		{
			::free(ptr);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(GroupObjFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	GroupObjFactory (void) { }

	~GroupObjFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IObjectFactory methods */

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	/* GroupObjFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
GroupObjFactory::~GroupObjFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void GroupObjFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE GroupObjFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_GROUP)
	{
		// BT_GROUP * data = (BT_GROUP *) _data;

		result = new OBJTYPE;
		result->pArchetype = ARCHLIST->GetArchetype(szArchname);
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 GroupObjFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	delete objtype;
	GroupObj::currentTick = 0;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * GroupObjFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createGroupObj(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void GroupObjFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _groupobj : GlobalComponent
{
	GroupObjFactory * gfactory;

	virtual void Startup (void)
	{
		gfactory = new DAComponent<GroupObjFactory>;
		AddToGlobalCleanupList((IDAComponent **) &gfactory);
	}

	virtual void Initialize (void)
	{
		gfactory->init();
	}
};

static _groupobj __gobj;

//---------------------------------------------------------------------------
//---------------------------End GroupObj.cpp--------------------------------
//---------------------------------------------------------------------------
