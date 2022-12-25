//--------------------------------------------------------------------------//
//                                                                          //
//                             ObjMapIterator.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjMapIterator.cpp 8     10/06/00 7:50p Jasony $
*/			    
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include "ObjMapIterator.h"

#ifndef DSECTOR_H
#include "DSector.h"
#endif

#ifndef IEXPLOSION_H
#include "IExplosion.h"
#endif

#ifndef ILAUNCHER_H
#include "ILauncher.h"
#endif

#ifndef OBJWATCH_H
#include "ObjWatch.h"
#endif

#include "ObjList.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
ObjMapIterator::ObjMapIterator (U32 _systemID,const Vector & _center, U32 _radius,U32 _playerID)
{
#ifndef FINAL_RELEASE
	CQASSERT(DEBUG_ITERATOR==0);
	DEBUG_ITERATOR = this;
#endif
	systemID = _systemID;
	center = _center;
	radius = _radius;
	playerID = _playerID;
	UpdateArea();
}
//---------------------------------------------------------------------------
//
void ObjMapIterator::SetArea (U32 _systemID,const Vector & _center, U32 _radius,U32 _playerID)
{
	systemID = _systemID;
	center = _center;
	radius = _radius;
	playerID = _playerID;
	UpdateArea();
}
//---------------------------------------------------------------------------
//
void ObjMapIterator::UpdateArea (void)
{
	numRefArray = OBJMAP->GetSquaresNearPoint(systemID,center,radius,ref_array);
	if(numRefArray)
	{
		refLoc = 0;
		current = OBJMAP->GetNodeList(systemID,ref_array[refLoc]);
		while(!current && (++refLoc)<numRefArray)
		{
			current = OBJMAP->GetNodeList(systemID,ref_array[refLoc]);
		}
		if(playerID)
		{
			while(current && (current->flags & OM_SHADOW) && (!(current->flags & (0x01000000 << (playerID-1)))))
			{
				current = current->next;
				while(!current && (++refLoc)<numRefArray)
				{
					current = OBJMAP->GetNodeList(systemID,ref_array[refLoc]);
				}
			}
		}
	}
	else
	{
		current = NULL;
	}
}
//---------------------------------------------------------------------------
//
void ObjMapIterator::SetFirst (void)
{
	if(numRefArray)
	{
		refLoc = 0;
		current = OBJMAP->GetNodeList(systemID,ref_array[refLoc]);
		while(!current && (++refLoc)<numRefArray)
		{
			current = OBJMAP->GetNodeList(systemID,ref_array[refLoc]);
		}
		if(playerID)
		{
			while(current && (current->flags & OM_SHADOW) && (!(current->flags & (0x01000000 << (playerID-1)))))
			{
				current = current->next;
				while(!current && (++refLoc)<numRefArray)
				{
					current = OBJMAP->GetNodeList(systemID,ref_array[refLoc]);
				}
			}
		}
	}
	else
	{
		current = NULL;
	}
}
//---------------------------------------------------------------------------
//
ObjMapNode * ObjMapIterator::Next (void)
{
	if(current)
	{
		current = current->next;
		while(!current && (++refLoc)<numRefArray)
		{
			current = OBJMAP->GetNodeList(systemID,ref_array[refLoc]);
		}
		if(playerID)
		{
			while(current && (current->flags & OM_SHADOW) && (!(current->flags & (0x01000000 << (playerID-1)))))
			{
				current = current->next;
				while(!current && (++refLoc)<numRefArray)
				{
					current = OBJMAP->GetNodeList(systemID,ref_array[refLoc]);
				}
			}
		}
	}
	return current;
}
//---------------------------------------------------------------------------
//
U32 ObjMapIterator::GetApparentPlayerID (U32 allyMask)
{
	if ((current->flags & OM_MIMIC) == 0)
		return (current->dwMissionID & PLAYERID_MASK);
	
	VOLPTR(ILaunchOwner) launchOwner=current->obj;
	CQASSERT(launchOwner);
	OBJPTR<ILauncher> launcher;
	launchOwner->GetLauncher(2,launcher);
	if (launcher)
	{
		VOLPTR(IMimic) mimic=launcher.Ptr();
		// if we are enemies
		if (mimic && mimic->IsDiscoveredTo(allyMask)==0)//hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
		{
			VOLPTR(IExtent) extentObj = current->obj;
			CQASSERT(extentObj);
			U32 dummy;
			U8 apparentPlayerID;
			extentObj->GetAliasData(dummy,apparentPlayerID);
			return apparentPlayerID;
		}
	}

	return current->dwMissionID & PLAYERID_MASK;
}
#ifndef FINAL_RELEASE
//------------------------------------------------------------------------------------
//
ObjMapIterator::operator bool (void) const
{
	if (current != 0)
	{
		// exclude child nuggets, and explosion
		U32 child = (current->dwMissionID>>24) & 0x7F;
		CQASSERT1( (child>0 && child < 5) || ((current->dwMissionID & 0xF)==0 ) || (current->flags&(OM_EXPLOSION|OM_AIR))!=0 ||
			OBJLIST->FindObject(current->dwMissionID, true) != 0, "Object 0x%X not removed from ObjMap", current->dwMissionID);
	}

	return (current != 0);
}
#endif
//------------------------------------------------------------------------------------
//---------------------------END ObjMapIterator.h-------------------------------------
//------------------------------------------------------------------------------------