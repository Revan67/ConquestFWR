//--------------------------------------------------------------------------//
//                                                                          //
//                               MacroHelp.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Macrohelp.cpp 16    6/22/01 9:56a Tmauer $
*/			    
//--------------------------------------------------------------------------//
#include "pch.h"

//
// This file helps the IDE generate helpful tips for the macros we use.
//
#ifdef MACROHELP_WAS_DEFINED   // should never be defined
#error Should never get here!

COLORREF RGB (BYTE r, BYTE g, BYTE b)		// convert 8 bit RGB values into a packed DWORD -- COLORREF
{
}

S32 REAL2IDEALX (S32 x)						// convert actual coordinate to IDEAL (640x480) (on x axis)
{
}

S32 REAL2IDEALY (S32 y)						// convert actual coordinate to IDEAL (640x480) (on y axis)
{
}

S32 IDEAL2REALX (S32 x)						// convert IDEAL (640x480) coordinate to actual (on x axis)
{
}

S32 IDEAL2REALY (S32 y)						// convert IDEAL (640x480) coordinate to actual (on y axis)
{
}

void SHIPCOMM (Speech message, U32 subTitle)				// play a spaceship comm message
{
}

void SHIPCOMM2 (IBaseObject *obj, U32 dwMissionID, Speech message, U32 subTitle, U32 displayName)
{
}

void GUNBOATCOMM (Speech message)				// play a gunboat comm message
{
}

void FABRICATORCOMM (Speech message,U32 subtitle)				// play a fabricator comm message
{
}

void MINELAYERCOMM (Speech message, U32 subtitle)				// play a minelayer comm message
{
}

void SUPPLYSHIPCOMM (Speech message, U32 subtitle)				// play a supplyship comm message
{
}

void GUNSATCOMM (Speech message)				// play a gunsat comm message
{
}

void PLATFORMCOMM (Speech message, U32 subtitle)
{
}

void PLATFORMCOMM2 (IBaseObject *obj, U32 dwMissionID, Speech message, U32 subtitle, U32 displayName)
{
}

void HARVESTERCOMM (IBaseObject *obj, U32 dwMissionID_ofPlanet, Speech message, U32 subtitle, U32 displayName)
{
}

void GUNBOATCOMM2 (IBaseObject *obj, U32 dwMissionID, Speech message, subTitle,displayName)
{
}

void FLAGSHIPCOMM (Speech message, U32 subtitle)
{
}

void FLAGSHIPCOMM2 (IBaseObject *obj, U32 dwMissionID, Speech message, U32 subtitle, U32 displayName)
{
}

bool TESTADMIRAL (U32 dwMissionID)	// returns true if ID refers to an admiral
{
}

void FLEETSHIPCOMM (Speech message, U32 subtitle)
{
}

void FLAGSHIPALERT (Speech message, U32 subtitle)		// bring up a GOTO alert with this message
{
}

void FLAGSHIPSCUTTLE (IBaseObject *obj, U32 dwMissionID, Speech message, U32 subtitle, U32 displayName)
{
}

void SUPPLYSHIPCOMM2 (IBaseObject *obj, U32 dwMissionID, Speech message, U32 subtitle, U32 displayName)
{
}

void TROOPSHIPCOMM (Speech message, U32 subtitle)
{
}

void TROOPSHIPCOMM2 (IBaseObject *obj, U32 dwMissionID, Speech message, U32 subtitle, U32 displayName)
{
}

OBJPTR VOLPTR (InterfaceName)
{
}

#endif
//--------------------------------------------------------------------------//
//------------------------------END MacroHelp.cpp---------------------------//
//--------------------------------------------------------------------------//
