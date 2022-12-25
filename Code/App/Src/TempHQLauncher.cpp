//--------------------------------------------------------------------------//
//                                                                          //
//                                TempHQLauncher.cpp                        //
//                                                                          //
//                  COPYRIGHT (C) 2003 By Warthog TX, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "SuperTrans.h"
#include "ObjList.h"
#include "sfx.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "TerrainMap.h"
#include "MPart.h"
#include "TObject.h"
#include <DTempHQLauncher.h>
#include "ILauncher.h"
#include "IWeapon.h"
#include "DSpaceShip.h"
#include "IGotoPos.h"
#include "OpAgent.h"
#include "IShipMove.h"
#include "CommPacket.h"
#include "sector.h"
#include "ObjSet.h"
#include "IFabricator.h"
#include "IPlanet.h"
#include "ObjMapIterator.h"
#include "UserDefaults.h"
#include "Camera.h"
#include "DBHotkeys.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h> 
#include <MGlobals.h>
#include <DMTechNode.h>
#include <hkevent.h>

#include <stdlib.h>

// so that we can use the techtree stuff
using namespace TECHTREE;

struct TempHQLauncherArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE TempHQLauncher : IBaseObject, ILauncher, ISaveLoad, ISystemSupplier, TEMPHQ_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(TempHQLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	_INTERFACE_ENTRY(ISystemSupplier)
	END_MAP()

	BT_TEMPHQ_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	U32 ownerID;

	SINGLE flash;

	//----------------------------------
	//----------------------------------
	
	TempHQLauncher (void);

	~TempHQLauncher (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const
	{
		return owner.Ptr()->GetSystemID();
	}

	virtual U32 GetPlayerID (void) const
	{
		return owner.Ptr()->GetPlayerID();
	}

	virtual void DrawHighlighted (void);

	// ISystemSupplier

	virtual bool IsTempSupply ();

	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);
	

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
	{
	}

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID);

	virtual const bool TestFightersRetracted (void) const 
	{ 
		return true;
	}

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		ability = USA_NONE;
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel();

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherOpCompleted(U32 agentID);

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle();

	virtual bool CanToggle();

	virtual bool IsOn();

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const;

	virtual U32 GetSyncData (void * buffer);

	virtual void PutSyncData (void * buffer, U32 bufferSize);

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file)
	{
		return TRUE;
	}

	virtual BOOL32 Load (struct IFileSystem * file)
	{
		return FALSE;
	}
	
	virtual void ResolveAssociations()
	{
	}

	/* TempHQLauncher methods */
	
	void init (PARCHETYPE pArchetype);
};

//---------------------------------------------------------------------------
//
TempHQLauncher::TempHQLauncher (void) 
{
	chargeTimer = 0;
	charge = 0;
}
//---------------------------------------------------------------------------
//
TempHQLauncher::~TempHQLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_TEMPHQ_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_TEMPHQ_LAUNCHER);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

}
// the following methods are for network synchronization of realtime objects
U32 TempHQLauncher::GetSyncDataSize (void) const
{
	return 0;
}

U32 TempHQLauncher::GetSyncData (void * buffer)			// buffer points to use supplied memory
{
	return 0;
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::PutSyncData (void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::DoSpecialAbility (U32 specialID)
{
}
//---------------------------------------------------------------------------
//
S32 TempHQLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 TempHQLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::DrawHighlighted (void)
{
	const USER_DEFAULTS * const pDefaults = DEFAULTS->GetDefaults();

	flash += (OBJLIST->GetRealRenderTime()*255);
	if(flash > 255)
		flash = 0;

	MPart part = owner.Ptr();
	if(part.isValid())
	{
		SINGLE supplyRatio = ((SINGLE)charge)/((SINGLE)pData->maxCharge);
		U32 hullMax = part->hullPointsMax;

		Vector point;
		S32 x, y;

		int TBARLENGTH = 100;
		if (hullMax < 1000)
		{
			if (hullMax < 100)
			{
				if (hullMax > 0)
					TBARLENGTH = 20;
				else
				{
					// no hull points, length should be decided by supplies
					// use same length as max supplies
				}
			}
			else // hullMax >= 100
			{
				TBARLENGTH = 20 + (((hullMax - 100)*80) / (1000-100));
			}
		}
		TBARLENGTH = IDEAL2REALX(TBARLENGTH);

		// want the bar length to match up with a little rectangle square
		if (TBARLENGTH%5)
		{
			TBARLENGTH -= TBARLENGTH%5;
		}


		OBJBOX box;
		owner.Ptr()->GetObjectBox(box);

		point.x = 0;
		point.y = box[2]+250.0;
		point.z = 0;

		CAMERA->PointToScreen(point, &x, &y, &(owner.Ptr()->GetTransform()));
		PANE * pane = CAMERA->GetPane();

		//
		// draw the blue (supplies) bar RGB(0,128,225)
		//
		if ((pDefaults->bCheatsEnabled && DBHOTKEY->GetHotkeyState(IDH_HIGHLIGHT_ALL)) || pDefaults->bEditorMode || MGlobals::AreAllies(owner.Ptr()->GetPlayerID(), MGlobals::GetThisPlayer()))
		{
			DA::RectangleHash(pane, x-(TBARLENGTH/2), y+5, x+(TBARLENGTH/2), y+5+2, RGB(128,128,128));
			if (charge > 0.0f)
			{
				COLORREF color;
				if(SECTOR->SystemInRootSupply(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID()))
					color = RGB(0,255,255);
				else
					color = RGB(flash,255,255);

				int xpos = x-(TBARLENGTH/2);
				int max = S32(TBARLENGTH*supplyRatio);
				int xrc;

				// make sure at least one bar gets displayed for one supply point
				if (max == 0 && supplyRatio > 0.0f)
				{
					max = 1;
				}
				
				for (int i = 0; i < max; i+=5)
				{
					xrc = xpos + i;
					DA::RectangleFill(pane, xrc, y+5, xrc+3, y+7, color);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 TempHQLauncher::Update (void)
{
	if(SECTOR->SystemInRootSupply(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID()))
	{
		if(chargeTimer < 0)
			chargeTimer = 0;
		chargeTimer += ELAPSED_TIME;
		U32 chargeInc = chargeTimer*pData->chargeRegenRate;
		if(chargeInc > 0)
		{
			charge += chargeInc;
			charge = __min(pData->maxCharge,charge);
			chargeTimer -= chargeInc/pData->chargeRegenRate;
		}
	}
	else
	{
		if(charge)
		{
			if(chargeTimer > 0)
				chargeTimer = 0;
			chargeTimer -= ELAPSED_TIME;
			U32 chargeInc = (-chargeTimer)*pData->chargeUseRate;
			if(chargeInc > 0)
			{
				if(chargeInc > charge)
					charge = 0;
				else
					charge -= chargeInc;
				chargeTimer += chargeInc/pData->chargeUseRate;
			}

			if(charge == 0)
			{
				SECTOR->ComputeSupplyForAllPlayers();
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
bool TempHQLauncher::IsTempSupply ()
{
	return charge > 0;
}

//---------------------------------------------------------------------------
//
void TempHQLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	ownerID = _owner->GetPartID();
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
bool TempHQLauncher::IsToggle()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool TempHQLauncher::CanToggle()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool TempHQLauncher::IsOn()
{
	return false;
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void TempHQLauncher::DoCreateWormhole(U32 systemID)
{
}

//---------------------------------------------------------------------------
//
const TRANSFORM & TempHQLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createTempHQLauncher (PARCHETYPE pArchetype)
{
	TempHQLauncher * tempHQLauncher = new ObjectImpl<TempHQLauncher>;

	tempHQLauncher->init(pArchetype);

	return tempHQLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------TempHQLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TempHQLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : TempHQLauncherArchetype
	{
		PARCHETYPE pArchetype;

		void * operator new (size_t size)
		{
			return calloc(size,1);
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

	BEGIN_DACOM_MAP_INBOUND(TempHQLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TempHQLauncherFactory (void) { }

	~TempHQLauncherFactory (void);

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

	/* TempHQLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TempHQLauncherFactory::~TempHQLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TempHQLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TempHQLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_TEMPHQ_LAUNCHER * data = (BT_TEMPHQ_LAUNCHER *) _data;

		if (data->type == LC_TEMPHQ_LAUNCHER)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 TempHQLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TempHQLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createTempHQLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void TempHQLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _tempHQLauncher : GlobalComponent
{
	TempHQLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TempHQLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _tempHQLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End TempHQLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------