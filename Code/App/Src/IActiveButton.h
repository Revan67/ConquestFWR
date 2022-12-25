#ifndef IACTIVEBUTTON_H
#define IACTIVEBUTTON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IActiveButton.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IActiveButton.h 13    7/27/00 11:25p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef DHOTBUTTONTEXT_H
#include "DHotButtonText.h"
#endif

struct TECHNODE;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IActiveButton : IDAComponent
{
	enum StallTypes
	{
		NO_STALL,
		NO_MONEY,
		MAX_UNITS
	};

	virtual void SetTechNode (TECHNODE node,TECHNODE allowed,TECHNODE workingNode) = 0;

	virtual void SetUpgradeLevel(S32 upgradeLevel) = 0;

	virtual void SetQueueNumber (U32 number) = 0;

	virtual U32 GetBuildArchetype (void) = 0;

	virtual void UpdateBuild(SINGLE percent, U32 stallType) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual void ResetActiveButton (void) = 0;

	virtual void SetBuildModeOn (bool value) = 0;

	virtual void EnableButton (bool bEnable) = 0;

	virtual void DrawAt(U32 xpos,U32 ypos) =0;

	virtual void GetShape(IDrawAgent ** shape) = 0;

	virtual HOTKEYS::HOTKEY GetHotkey() = 0;

	virtual bool IsActive() = 0;
};


#endif