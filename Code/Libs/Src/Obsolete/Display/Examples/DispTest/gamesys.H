//****************************************************************************
//*                                                                          *
//*  GAMESYS.H: DA COM game shell services component                         *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 10-Apr-97: Initial                                             *
//*                                                                          *
//*  32-bit protected-mode source compatible with Watcom 10.5/MSC 9.0        *
//*                                                                          *
//*  Author: John Miles                                                      *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  The system component acts as a DA COM-compatible wrapper for the        *
//*  SAL (System Abstraction Layer) library and all system-specific events   *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#ifndef GAMESYS_H
#define GAMESYS_H

#ifndef SYSTEM_H
#include "system.h"
#endif

#ifndef _DISPLAY_H
#include "Display.h"
#endif

#ifndef WINDOWMANAGER_H
#include "WindowManager.h"
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef TSMARTPOINTER_H
#include "TSmartPointer.h"
#endif

#ifndef RENDPIPELINE_H
#include "RendPipeline.h"
#endif

//----------------------------------------------------------------------------
//
inline ISystemContainer * CreateGameSystem (HINSTANCE hInstance, 
											const C8 *app_name, 
											WMEXITCB  _exit_fn,
											U32 flags=0,
											const WNDCLASSEX *wc = 0)
{
	ISystemContainer * result;
	COMPTR<IWindowManager> WM;
	COMPTR<IRenderPipeline> PIPE;
	ICOManager *DACOM = DACOM_Acquire();
	AGGDESC adesc = "ISystemContainer";
	
	if (DACOM->CreateInstance(&adesc, (void **) &result) == GR_OK)
	{
		result->LoadSystemComponents();
		
		if (result->QueryInterface("IWindowManager", WM) == GR_OK)
		{
			WM->Startup(hInstance, app_name, _exit_fn, flags, wc);
		}

		if (result->QueryInterface("IRenderPipeline", PIPE) == GR_OK)
		{
			PIPE->startup();
		}
	}
	
	return result;
}
//----------------------------------------------------------------------------
//
inline IEngine * CreateGameEngine (struct IDAComponent *system)
{
	IEngine *engine;
	ICOManager *DACOM = DACOM_Acquire();
	DACOMDESC desc = "IEngine";
	
	if (DACOM->CreateInstance(&desc, (void **) &engine) == GR_OK)
		engine->load_engine_components(system);
	
	return engine;
}

struct GameSystem
{
	COMPTR<ISystemContainer> GS;
	COMPTR<IEngine>          ENG;
	
	BOOL32 startup (HINSTANCE hInstance, 
					const C8 *app_name, 
					WMEXITCB  _exit_fn,
					U32 flags=0,
					const WNDCLASSEX *wc = 0)
	{
		GS.ptr  = CreateGameSystem(hInstance, app_name, _exit_fn, flags, wc);
		
		if (GS == 0 || (ENG.ptr = CreateGameEngine(GS)) == 0)
		{
			GS.free();
			return 0;
		}
		
		return 1;
	}
	
	void shut_down (void)
	{
		ENG.free();
		if (GS)
		{
			GS->Shutdown();
			GS.free();
		}
	}
	
	operator IEngine * (void)
	{
		return ENG;
	}
	
	operator ISystemContainer * (void)
	{
		return GS;
	}
};

#endif
