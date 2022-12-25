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

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef TSMARTPOINTER_H
#include "TSmartPointer.h"
#endif

//----------------------------------------------------------------------------
//
inline ISystemContainer * CreateGameSystem ()
{
	ISystemContainer * result;
	ICOManager *DACOM = DACOM_Acquire();
	AGGDESC adesc = "ISystemContainer";
	
	if (DACOM->CreateInstance(&adesc, (void **) &result) == GR_OK)
	{
		result->LoadSystemComponents();
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

struct SystemManager
{
	COMPTR<ISystemContainer> container;
	COMPTR<IEngine>          engine;
	
	BOOL32 startup ()
	{
		container.ptr  = CreateGameSystem();
		
		if (container == 0 || (engine.ptr = CreateGameEngine(container)) == 0)
		{
			container.free();
			return 0;
		}
		
		return 1;
	}
	
	void shut_down (void)
	{
		engine.free();
		if (container)
		{
			container->Shutdown();
			container.free();
		}
	}
	
	operator IEngine * (void)
	{
		return engine;
	}
	
	operator ISystemContainer * (void)
	{
		return container;
	}
};

#endif
