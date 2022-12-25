#ifndef CQLIGHT_H
#define CQLIGHT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                CQLight.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/CQLight.h 19    10/23/00 5:10p Jasony $
*/			    
//---------------------------------------------------------------------------
// Base light for Conquest
//---------------------------------------------------------------------------

#ifndef BASELIGHT_H
#include <BaseLight.h>
#endif

#ifndef EVENTSYS_H
#include <EventSys.h>
#endif

#ifndef TSMARTPOINTER_H
#include <TSmartPointer.h>
#endif

#ifndef ICONNECTION_H
#include <IConnection.h>
#endif

#ifndef SECTOR_H
#include "Sector.h"
#endif

#ifndef GRIDVECTOR_H
#include "GridVector.h"
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct DACOM_NO_VTABLE ILights : IDAComponent
{	
	virtual void DeleteLight (struct GameLight *light) = 0;
	
	virtual void EnableSpecular (BOOL32 bEnable) = 0;

	virtual	BOOL32 __stdcall New (void) = 0;

	virtual	BOOL32 __stdcall Load (struct IFileSystem * inFile) = 0;

	virtual void ActivateBestLights (const Vector &spot, int maxLights, float maxRadius) = 0;
	
	virtual void ActivateAmbientLight(const Vector &_spot) = 0;

	virtual void DeactivateAllLights () = 0;

	static int GetNextEngineID ();
	
	static void ReleaseEngineID (int engineID);

	virtual void RestoreAllLights () = 0;

	virtual void SetSysAmbientLight (U8 r,U8 g,U8 b) = 0;

	virtual void GetSysAmbientLight (U8 &r,U8 &g,U8 &b) = 0;

	virtual void SetAmbientLight (U8 r,U8 g,U8 b) = 0;
};

struct DACOM_NO_VTABLE ICQLight : IDAComponent
{
	virtual U32 GetEngineID() = 0;

	virtual void UpdateLight(bool bUseLight = false) = 0;
};


struct DummyLight : BaseLight, IEventCallback, ICQLight
{
	BEGIN_DACOM_MAP_INBOUND(DummyLight)
	DACOM_INTERFACE_ENTRY(ILight)
	DACOM_INTERFACE_ENTRY(ICQLight)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	DummyLight (void)  : BaseLight(ENGINE,(ISystemContainer *)GS)
	{}
};

#pragma warning (disable : 4505)

struct CQLight : DAComponent<DummyLight>
{
private:
	bool bLogicalOn;
	U32 callbackHandle;
protected:
	U32 systemID;
	bool bAmbient;
public:
	U32 engineID;
	GRIDVECTOR gridPos;

	CQLight * pPrevReg, *pNextReg;
	static CQLight * pRegList;		// list of registered lights

	CQLight (void);

	~CQLight (void);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);

	/* IDAComponent methods */


	/* CQLight methods */

	DEFMETHOD(set_On) (BOOL32 _on);

	virtual void UpdateLight(bool bUseLight = false);

	void setSystem (U32 _systemID)
	{
		systemID = _systemID;
		if (bLogicalOn != (on != 0))
			set_On(bLogicalOn);
	}

	void setColor (int _r,int _g,int _b)
	{
		if (CQRENDERFLAGS.bSoftwareRenderer)
		{
			int total = _r+_g+_b;
			color.r = color.g = color.b = total/3;
		}
		else
		{
			color.r = _r;
			color.g = _g;
			color.b = _b;
		}
	}

	void makeAmbient (bool _bAmbient)
	{
		if (systemID == SECTOR->GetCurrentSystem())
		{
			if (_bAmbient)
			{
				LIGHTS->SetSysAmbientLight(color.r,color.g,color.b);
				LIGHTS->SetAmbientLight(color.r,color.g,color.b);
				BaseLight::set_On(0);
				removeFromRegList();
			}
			else if (bLogicalOn)
			{
				BaseLight::set_On(1);
				addToRegList();
			}
		}
		else
		{
			BaseLight::set_On(0);
			removeFromRegList();
		}

		bAmbient = _bAmbient;
	}

	bool isAmbient()
	{
		return bAmbient;
	}

	virtual U32 GetEngineID()
	{
		return engineID;
	}

	void addToRegList (void)
	{
		if (pRegList!=this && pPrevReg==0 && pNextReg==0)
		{
			if ((pNextReg = pRegList) != 0)
				pRegList->pPrevReg = this;
			pRegList = this;
		}
	}

	void removeFromRegList (void)
	{
		if (pRegList==this || pPrevReg || pNextReg)
		{
			if (pPrevReg)
				pPrevReg->pNextReg = pNextReg;
			if (pNextReg)
				pNextReg->pPrevReg = pPrevReg;
			if (pRegList==this)
				pRegList = pNextReg;
			pPrevReg = pNextReg = 0;
		}
	}

	GRIDVECTOR & getGridPosition (void)
	{
		if (gridPos.isZero())
			gridPos = transform.translation;
		return gridPos;
	}

};

//-------------------------------------------------------------------------------
//------------------------------END CQLight.h------------------------------------
//-------------------------------------------------------------------------------
#endif
