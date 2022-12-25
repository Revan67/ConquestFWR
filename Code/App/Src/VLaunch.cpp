//--------------------------------------------------------------------------//
//                                                                          //
//                                VLaunch.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/VLaunch.cpp 31    10/04/00 8:35p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "VLaunch.h"
#include "SuperTrans.h"
#include "ObjList.h"
#include <DWeapon.h>

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "GridVector.h"


#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h>


#include <stdlib.h>

//---------------------------------------------------------------------------
//
VLaunch::VLaunch (void) 
{
	memset(ownerIndex, -1, sizeof(ownerIndex));
}
//---------------------------------------------------------------------------
//
VLaunch::~VLaunch (void)
{
}
//---------------------------------------------------------------------------
//
void VLaunch::init (PARCHETYPE _pArchetype, PARCHETYPE _pBoltType)
{
	const BT_VERTICAL_LAUNCH * data = (const BT_VERTICAL_LAUNCH *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->type == LC_VERTICLE_LAUNCH);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

	pBoltType = _pBoltType;

	REFIRE_PERIOD = data->refirePeriod;
	CQASSERT(REFIRE_PERIOD!=0);
	MINI_REFIRE = data->miniRefire * REALTIME_FRAMERATE;
	MINI_REFIRE = __max(MINI_REFIRE, 1);
	SUPPLY_COST = data->supplyCost;
}
//---------------------------------------------------------------------------
//
void VLaunch::InitLauncher (IBaseObject * _owner, S32 _ownerIndex, S32 animArcheIndex, SINGLE range)
{
	const BT_VERTICAL_LAUNCH * data = (const BT_VERTICAL_LAUNCH *) ARCHLIST->GetArchetypeData(pArchetype);
	int i;

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	salvo = MAX_SALVO = findSalvo(data);

	for (i = 0; i < MAX_VERTICAL_TUBES; i++)
	{
		if (data->hardpoint[i][0])
		{
			FindHardpoint(data->hardpoint[i], ownerIndex[i], hardpointinfo[i], _ownerIndex);
			CQASSERT(ownerIndex[i] != -1);
		}
	}
}


Vector VLaunch::calculateErrorPos(const Vector & pos)
{
	Vector result;
	SINGLE angle = (float(rand() & 2047) / 2048) * 2 * PI;
	
	result.x = cos(angle) * rangeError;
	angle = (float(rand() & 2047) / 2048) * 2 * PI;
	result.y = sin(angle) * rangeError;
	result.z = ((float(rand() & 2047) / 2048) - 0.5F) * rangeError;
	
	return result + pos;
}
//---------------------------------------------------------------------------
//
BOOL32 VLaunch::Update (void)
{
	if (attacking == 2 && target == 0)
	{
		target = 0;
		attacking = 0;
	}
	if (attacking == 2)
		targetPos = target->GetPosition();
	if (attacking)
	{
		if (refireDelay <= 0)
		{
			if((owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked()))
			{
				if (--miniDelay <= 0)
				{
					//if this is the first missile of the bunch
					if (salvo == (S32)MAX_SALVO)
						rangeError = 0.5*rangeFinder.calcRangeError(this, target, owner.Ptr());

					miniDelay += MINI_REFIRE;
					if (--salvo <= 0)
					{
						salvo = MAX_SALVO= findSalvo();
						refireDelay += REFIRE_PERIOD;
					}

					Vector goal = targetPos - owner.Ptr()->GetPosition();
					goal.z = 0;
					SINGLE distanceToTarget = goal.magnitude();

					if (U32(salvo) != MAX_SALVO-1 || 
						((target==0 || (target->GetSystemID()==GetSystemID() && target->IsVisibleToPlayer(owner.Ptr()->GetPlayerID()))) &&
						 (distanceToTarget < owner->GetWeaponRange() && checkSupplies()) ) )
					{
						if(owner.Ptr()->bVisible || target.Ptr()->bVisible)
						{
							if(OBJLIST->CreateProjectile())
							{
								IBaseObject * obj;
								VOLPTR(IWeapon) bolt;
								TRANSFORM trans;

								getSighting(getRandomTube(), trans);
								obj = ARCHLIST->CreateInstance(pBoltType);

								if (obj)
								{
									OBJLIST->AddObject(obj);
									if ((bolt = obj) != 0)
									{
										Vector temp = calculateErrorPos(targetPos);
										bolt->InitWeapon(owner.Ptr(), trans, target, 0, &temp);
									}
								}
								else
								{
									OBJLIST->ReleaseProjectile();
								}
							}
						}
					}
					else
					if (U32(salvo) == MAX_SALVO-1)	// out of supplies or out of range
					{
						salvo = MAX_SALVO= findSalvo();
						miniDelay = MINI_REFIRE;
					}
				}
			}
		}
		else
			refireDelay -= ELAPSED_TIME;
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void VLaunch::AttackPosition (const struct GRIDVECTOR * gridtarget, bool bSpecial)
{
	if (gridtarget == 0)
	{
		attacking = 0;
		target = 0;
	}
	else
	{
		targetPos = *gridtarget;
		attacking = 1;
		target = 0;
	}
}
//---------------------------------------------------------------------------
//
void VLaunch::AttackObject (IBaseObject * obj)
{
	if (obj == 0)
	{
		attacking = 0;
		target = 0;
		dwTargetID = 0;
	}
	else
	{
		dwTargetID = obj->GetPartID();
		obj->QueryInterface(IBaseObjectID, target, GetPlayerID());
		attacking = 2;
		if (refireDelay <= 0 && salvo != (S32)MAX_SALVO)
		{
			salvo = MAX_SALVO= findSalvo();
			refireDelay += REFIRE_PERIOD;
		}
	}
}
//---------------------------------------------------------------------------
//
S32 VLaunch::GetObjectIndex (void) const
{
	return -1;
}
//---------------------------------------------------------------------------
//
U32 VLaunch::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
// get the sighting vector of the barrel
//
void VLaunch::getSighting (int which, TRANSFORM & result)
{
	result.TRANSFORM::TRANSFORM(hardpointinfo[which].orientation, hardpointinfo[which].point);
	result = ENGINE->get_transform(ownerIndex[which]).multiply(result);
}
//---------------------------------------------------------------------------
//
/*S32 VLaunch::findChild (const char * pathname, INSTANCE_INDEX parent)
{
	S32 index = -1;
	char buffer[MAX_PATH];
	const char *ptr=pathname, *ptr2;
	INSTANCE_INDEX child=-1;

	if (ptr[0] == '\\')
		ptr++;

	if ((ptr2 = strchr(ptr, '\\')) == 0)
	{
		strcpy(buffer, ptr);
	}
	else
	{
		memcpy(buffer, ptr, ptr2-ptr);
		buffer[ptr2-ptr] = 0;		// null terminating
	}

	while ((child = MODEL->get_child(parent, child)) != -1)
	{
		if (MODEL->is_named(child, buffer))
		{
			if (ptr2)
			{
				// found the child, go deeper if needed
				parent = child;
				child = -1;
				ptr = ptr2+1;
				if ((ptr2 = strchr(ptr, '\\')) == 0)
				{
					strcpy(buffer, ptr);
				}
				else
				{
					memcpy(buffer, ptr, ptr2-ptr);
					buffer[ptr2-ptr] = 0;		// null terminating
				}
			}
			else
			{
				index = child;
				break;
			}
		}
	}

	return index;
}
//----------------------------------------------------------------------------------------
//
BOOL32 VLaunch::findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
{
	BOOL32 result=0;
	char buffer[MAX_PATH];
	char *ptr=buffer;

	strcpy(buffer, pathname);
	if ((ptr = strrchr(ptr, '\\')) == 0)
		goto Done;

	*ptr++ = 0;
	if (buffer[0])
		index = findChild(buffer, parent);
	else
		index = parent;

Done:	
	if (index != -1)
	{
		HARCH arch = index;

		if ((result = HARDPOINT->retrieve_hardpoint_info(arch, ptr, hardpointinfo)) == false)
			index = -1;		// invalidate result
	}

	return result;
}*/
//---------------------------------------------------------------------------------------------
//
int VLaunch::getRandomTube (void)
{
	int i, result;

	if (numRandomEntries==0)
	{
		// reset the list
		for (i = 0; i < MAX_VERTICAL_TUBES; i++)
		{
			if (ownerIndex[i] >= 0)		// isValid
				randomIndex[numRandomEntries++] = i;
		}

		CQASSERT(numRandomEntries);
	}

	i = rand() % numRandomEntries;
	result = randomIndex[i];
	randomIndex[i] = randomIndex[--numRandomEntries];

	return result;
}
//---------------------------------------------------------------------------
// return true if ok to fire
//
bool VLaunch::checkSupplies (void)
{
	bool result = owner->UseSupplies(SUPPLY_COST);

	return result;
}
//---------------------------------------------------------------------------
//
U32 VLaunch::findSalvo(const BT_VERTICAL_LAUNCH * data)
{
	if(!data)
		data = (BT_VERTICAL_LAUNCH *)(ARCHLIST->GetArchetypeData(pArchetype));
	CQASSERT(data);
	if(owner.Ptr())
	{
		U32 ownerPlayerID = owner.Ptr()->GetPlayerID();
		if(ownerPlayerID)
		{
			TECHNODE tech = MGlobals::GetCurrentTechLevel(ownerPlayerID);
			for(S32 i = 3; i >= 0; --i)
			{
				if(tech.HasTech(data->upgrade[i].techNeed))
				{
					return data->upgrade[i].salvo;
				}
			}
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createVLaunch (PARCHETYPE pArchetype, PARCHETYPE pBoltType)
{
	VLaunch * launch = new ObjectImpl<VLaunch>;

	launch->init(pArchetype, pBoltType);

	return launch;
}
//------------------------------------------------------------------------------------------
//---------------------------VLaunch Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE VLaunchFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		PARCHETYPE pBoltType;

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
			if (pBoltType)
				ARCHLIST->Release(pBoltType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(VLaunchFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	VLaunchFactory (void) { }

	~VLaunchFactory (void);

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

	/* VLaunchFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
VLaunchFactory::~VLaunchFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void VLaunchFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE VLaunchFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_VERTICAL_LAUNCH * data = (BT_VERTICAL_LAUNCH *) _data;

		if (data->type == LC_VERTICLE_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
 
			result->pBoltType = ARCHLIST->LoadArchetype(data->weaponType);
			CQASSERT(result->pBoltType);
			ARCHLIST->AddRef(result->pBoltType, OBJREFNAME);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 VLaunchFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * VLaunchFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createVLaunch(objtype->pArchetype, objtype->pBoltType);
}
//-------------------------------------------------------------------
//
void VLaunchFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _vlaunch : GlobalComponent
{
	VLaunchFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<VLaunchFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _vlaunch __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End VLaunch.cpp-----------------------------------------------
//---------------------------------------------------------------------------------------------
