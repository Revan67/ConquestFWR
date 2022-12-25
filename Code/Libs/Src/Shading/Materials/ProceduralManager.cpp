/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ProceduralManager.cpp

	MAR.24 2000 Written by Yuichi Ito

=============================================================================*/
#define __PROCEDURALMANAGER_CPP

#include "ProceduralNoise.h"
#include "ProceduralSwirl.h"

//-----------------------------------------------------------------------------
void InstallProceduralManager()
{
	ProceduralBase::install();

	// registory
	ProceduralNoise::registory();
	ProceduralSwirl::registory();
}

//-----------------------------------------------------------------------------
void UnInstallProceduralManager()
{
	ProceduralBase::unInstall();
}

