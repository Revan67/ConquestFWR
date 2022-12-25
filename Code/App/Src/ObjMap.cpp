//--------------------------------------------------------------------------//
//                                                                          //
//                               ObjMap.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjMap.cpp 21    10/27/00 11:47a Jasony $
*/			    
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include "ObjMap.h"
#include "ObjMapIterator.h"
#include "Sector.h"
#include "IObject.h"
#include "Startup.h"
#include "GridVector.h"
#include "ObjWatch.h"
#include "ObjList.h"
#include "IExplosion.h"
#include "ILauncher.h"


#include <Heapobj.h>

//////////////////////////////

/////////////////////////////////////

struct ObjMapSquare
{
	ObjMapNode *nodeList;
	ObjMapNode *lastNode;
	
	ObjMapSquare()
	{
		nodeList = lastNode = 0;
	}

	~ObjMapSquare()
	{
		ObjMapNode *pos=nodeList;
		while (pos)
		{
			nodeList=pos->next;
			delete pos;
			pos = nodeList;
		}
	}
};

struct ObjMapSystem
{
	int width;
	int square_size;
	ObjMapSquare *squares;
	int square_cnt;
};

struct ObjMap : IObjMap
{
	ObjMapSystem maps[MAX_SYSTEMS];
	bool bInitialized;

	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(ObjMap)
	DACOM_INTERFACE_ENTRY(IObjMap)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	~ObjMap();

	virtual ObjMapNode * AddObjectToMap(IBaseObject *obj,U32 systemID,int square_id,U32 _flags);

	virtual void RemoveObjectFromMap(IBaseObject *obj,U32 systemID,int square_id);

	virtual int GetMapSquare(U32 systemID,const Vector &pos);

	virtual int GetSquaresNearPoint(U32 systemID,const Vector &pos,int radius,int *ref_array);

	virtual ObjMapNode *GetNodeList(U32 systemID,int square_id);

	virtual U32 GetApparentPlayerID(ObjMapNode *node,U32 allyMask);

	virtual bool IsObjectInMap(IBaseObject *obj);

	virtual void Init (void);

	virtual void Uninit (void);

	void freeArrays();
};

ObjMap::~ObjMap()
{
	freeArrays();
}

ObjMapNode * ObjMap::AddObjectToMap(IBaseObject *obj,U32 systemID,int square_id,U32 _flags)
{
	if (bInitialized==0)
		return NULL;
	CQASSERT(systemID != 0);
	CQASSERT(square_id < maps[systemID-1].square_cnt);
	CQASSERT((U32(obj->next) & 0xff000000) != 0xff000000);

	// exclude child nuggets, and explosion
/*#ifndef FINAL_RELEASE
	U32 child = (obj->GetPartID()>>24) & 0x7F;
	CQASSERT1( (child>0 && child < 5) || ((obj->GetPartID() & 0xF)==0 ) || (_flags&(OM_EXPLOSION|OM_AIR))!=0 ||
		OBJLIST->FindObject(obj->GetPartID()) != 0, "Object 0x%X not removed from ObjMap", obj->GetPartID());
#endif
*/
	ObjMapNode *newNode=0;

	if (systemID <= MAX_SYSTEMS)
	{
		newNode = new ObjMapNode;
		
		if (maps[systemID-1].squares[square_id].lastNode)
			maps[systemID-1].squares[square_id].lastNode->next = newNode;
		else
			maps[systemID-1].squares[square_id].nodeList = newNode;
		maps[systemID-1].squares[square_id].lastNode = newNode;
		newNode->next = NULL;
		newNode->obj = obj;
		newNode->flags = _flags;
		newNode->dwMissionID = obj->GetPartID();
	}

	return newNode;
}

void ObjMap::RemoveObjectFromMap(IBaseObject *obj,U32 systemID,int square_id)
{
	if (bInitialized && systemID && systemID <= MAX_SYSTEMS)
	{

		CQASSERT(square_id < maps[systemID-1].square_cnt);
		ObjMapNode *pos;
		
		pos = maps[systemID-1].squares[square_id].nodeList;
		if (pos)
		{
			if (pos->obj == obj)
			{
				maps[systemID-1].squares[square_id].nodeList = pos->next;
				if (pos == maps[systemID-1].squares[square_id].lastNode)
					maps[systemID-1].squares[square_id].lastNode = NULL;
				delete pos;
			}
			else
			{
				ObjMapNode *deadguy;
				while (pos->next && pos->next->obj != obj)
				{
					CQASSERT1((U32(pos->next->obj->next) & 0xff000000) != 0xff000000, "Dead unit 0x%X found in OBJMAP", pos->next->dwMissionID);
					pos = pos->next;
				}

				if ((deadguy = pos->next) != 0)
				{
					pos->next = pos->next->next;
					if (deadguy == maps[systemID-1].squares[square_id].lastNode)
						maps[systemID-1].squares[square_id].lastNode = pos;
					delete deadguy;
				}
			}
		}
	}
}

int ObjMap::GetMapSquare(U32 systemID,const Vector &pos)
{
	if (bInitialized)
	{
		CQASSERT(systemID != 0 && systemID <= MAX_SYSTEMS);
		int x = (pos.x/maps[systemID-1].square_size);
		int y = (pos.y/maps[systemID-1].square_size);
		if (x < 0)
			x=0;
		if (x > maps[systemID-1].width-1)
			x=maps[systemID-1].width-1;
		if (y<0)
			y=0;
		if (y > maps[systemID-1].width-1)
			y=maps[systemID-1].width-1;
		return (x*maps[systemID-1].width+y);
	}
	return 0;
}

int ObjMap::GetSquaresNearPoint(U32 systemID,const Vector &pos,int radius,int *ref_array)
{
	if (bInitialized==0)
		return 0;
	CQASSERT(radius >= 0);
	
	if ((systemID == 0) || systemID > MAX_SYSTEMS)
		return 0;
	int posX = F2LONG(pos.x);
	int posY = F2LONG(pos.y);
	int cnt=0;
	S32 minX,maxX,minY,maxY;
	minX=max((posX-radius)/SQUARE_SIZE,0);
	minY=max((posY-radius)/SQUARE_SIZE,0);
	maxX=min(((posX+radius-1)/SQUARE_SIZE)+1,maps[systemID-1].width);
	maxY=min(((posY+radius-1)/SQUARE_SIZE)+1,maps[systemID-1].width);
//	CQASSERT(minX < maps[systemID-1].width);
//	CQASSERT(minY < maps[systemID-1].width);
	for (int i=minX;i<maxX;i++)
	{
		for (int j=minY;j<maxY;j++)
		{
		/*	S32 closeX,closeY;
			closeX = i*SQUARE_SIZE;
			if (posX-closeX > SQUARE_SIZE/2)
				closeX += SQUARE_SIZE;
			
			closeY = j*SQUARE_SIZE;
			if (posY-closeY > SQUARE_SIZE/2)
				closeY += SQUARE_SIZE;
			
			if (abs(closeX-posX) < radius && abs(closeY-posY) < radius)
			{
				CQASSERT(i*maps[systemID-1].width+j < maps[systemID-1].square_cnt);*/
					ref_array[cnt++] = i*maps[systemID-1].width+j;
			/*}
			else
				CQASSERT("ER?");*/
		}
	}

	CQASSERT(cnt <= MAX_MAP_REF_ARRAY_SIZE);
	return cnt;
}

ObjMapNode * ObjMap::GetNodeList(U32 systemID,int square_id)
{
	if (bInitialized==0)
		return NULL;
	CQASSERT(square_id < maps[systemID-1].square_cnt);
	return maps[systemID-1].squares[square_id].nodeList;
}

U32 ObjMap::GetApparentPlayerID(ObjMapNode *node,U32 allyMask)
{
	if ((node->flags & OM_MIMIC) == 0)
		return (node->dwMissionID & PLAYERID_MASK);
	
	VOLPTR(ILaunchOwner) launchOwner=node->obj;
	CQASSERT(launchOwner);
	if (launchOwner)
	{
		OBJPTR<ILauncher> launcher;
		launchOwner->GetLauncher(2,launcher);
		if (launcher)
		{
			VOLPTR(IMimic) mimic=launcher.Ptr();
			// if we are enemies
			if (mimic && mimic->IsDiscoveredTo(allyMask)==0)//hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
			{
				VOLPTR(IExtent) extentObj = node->obj;
				CQASSERT(extentObj);
				if (extentObj)
				{
					U32 dummy;
					U8 apparentPlayerID;
					extentObj->GetAliasData(dummy,apparentPlayerID);
					return apparentPlayerID;
				}
			}
		}
	}

	return node->dwMissionID & PLAYERID_MASK;
}

bool ObjMap::IsObjectInMap(IBaseObject *obj)
{
	if (bInitialized==0)
		return false;
	for(U32 mCount = 0; mCount < MAX_SYSTEMS; ++mCount)
	{
		for(S32 sqCount = 0; sqCount < maps[mCount].square_cnt; ++ sqCount)
		{
			if(maps[mCount].squares)
			{
				ObjMapNode * list = maps[mCount].squares[sqCount].nodeList;
				while(list)
				{
					if(list->obj == obj)
						return true;
					list = list->next;
				}
			}
		}
	}
	return false;
}

void ObjMap::Init()
{
	CQASSERT(bInitialized==false);

	freeArrays();

	int numSystems = SECTOR->GetNumSystems();

	for (int s=0;s<numSystems;s++)
	{
		int width;
		RECT rect;

		SECTOR->GetSystemRect(s+1,&rect,false);
		width = rect.right-rect.left;
		maps[s].square_size = SQUARE_SIZE;
		maps[s].width = width/SQUARE_SIZE;
		maps[s].squares = new ObjMapSquare[maps[s].width*maps[s].width];
		maps[s].square_cnt = maps[s].width*maps[s].width;
	}

	bInitialized = true;
}

void ObjMap::Uninit (void)
{
	freeArrays();
	bInitialized = false;
}

void ObjMap::freeArrays()
{
	for (int s=0;s<MAX_SYSTEMS;s++)
	{
		delete [] maps[s].squares;
		maps[s].squares=0;
		maps[s].square_cnt = 0;
		maps[s].square_size = 0;
		maps[s].squares = 0;
		maps[s].width = 0;
	}
}
//----------------------------------------------------------------------------------------------
//
struct _objmap : GlobalComponent
{
	ObjMap * om;

	virtual void Startup (void)
	{
		OBJMAP = om = new DAComponent<ObjMap>;
		AddToGlobalCleanupList(&OBJMAP);
	}
	
	virtual void Initialize (void)
	{
		//om->Init();

	//	if (om->CreateViewer() == 0)
	//		CQBOMB0("Viewer could not be created.");
	}
};

static _objmap objmap;
//----------------------------------------------------------------------------
//-------------------------END ObjMap.cpp-------------------------------------
//----------------------------------------------------------------------------