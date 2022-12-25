#ifndef DSPECIALDATA_H
#define DSPECIALDATA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSpecialData.h    							//
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSpecialData.h 12    10/06/00 4:28p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DHOTBUTTONTEXT_H
#include "DHotButtonText.h"
#endif

//---------------------------------------------------------------------------//
//
struct SPECIALABILITYINFO
{
	U32 baseSpecialWpnButton;		// index into shape table
	HBTNTXT::BUTTON_TEXT specialWpnTooltip;
	HBTNTXT::HOTBUTTONINFO specialWpnHelpBox;//actualy the status text
	HBTNTXT::MULTIBUTTONINFO specialWpnHintBox;//this is the hint box
};
//---------------------------------------------------------------------------//
//
struct GT_SPECIALABILITIES
{
	// note: these special abilities must be listed in order of the enum!!!
	SPECIALABILITYINFO none, assault, tempest, probe, cloak, aegis, vampire, stasis, furyram, repel, bomber, mimic, minelayer, s_minelayer,
	repulsor, synthesis, massdisruptor, destabilizer, wormhole,ping,m_ping,s_ping, multiCloak, tractor, jump, artilery, tractorWave, nova, shieldJam,
	weaponJam, sensorJam, transfer;
};
#ifndef _ADB
struct _GT_SPECIALABILITIES
{
	SPECIALABILITYINFO info[USA_LAST];
};
#endif // !_ADB


//---------------------------------------------------------------------------//
//
#endif
