#ifndef DSOUNDS_H
#define DSOUNDS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSounds.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSounds.h 15    8/11/99 2:35p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifdef _ADB
#ifndef SFXID_H
#include "SfxID.h"
#endif
#else // ! _ADB
namespace SFX
{
enum ID;
}
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

//----------------------------------------------------------------
//
struct GT_GLOBAL_SOUNDS
{
	SFX::ID defaultButton;
	SFX::ID systemSelect;
	SFX::ID buildConfirm;
	SFX::ID moveConfirm;
	
	SFX::ID startConstuction; //Begin construction
	SFX::ID endConstruction; //Ship finished construction
	SFX::ID zeroMoney; //Out of Money
	SFX::ID lightindustryButton; //Light Industry button
	SFX::ID heavyindustryButton; //Heavy Industry button
	SFX::ID hitechindustryButton; //Hitech Industry button
	SFX::ID hqButton; //HQ button
	SFX::ID researchButton; //Research Button
	SFX::ID planetDepleted; //Planet Depleted
	SFX::ID harvestRedeploy; //Harvest Button
	SFX::ID researchCompleted; //When Research is done
};
//----------------------------------------------------------------
//

//----------------------------------------------------------------
//

#endif
