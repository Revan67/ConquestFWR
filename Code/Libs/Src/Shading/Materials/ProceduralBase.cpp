/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ProceduralBase.cpp

	MAR.24 2000 Written by Yuichi Ito

=============================================================================*/
#define __PROCEDURALBASE_CPP

#include "ProceduralBase.h"

//-----------------------------------------------------------------------------
ProceduralBase::Container	ProceduralBase::m_createFuncs;

//-----------------------------------------------------------------------------
BOOL32 ProceduralBase::install()
{
	m_createFuncs.clear();
	return TRUE;
}

//-----------------------------------------------------------------------------
void ProceduralBase::unInstall()
{
	m_createFuncs.clear();
}

//-----------------------------------------------------------------------------
ProceduralBase *ProceduralBase::create( ProceduralBase::Type type )
{
	return m_createFuncs[ type ]();
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralBase::registory_create_func( ProceduralBase::Type type, CreateFunc func )
{
	return m_createFuncs.insert( Container::value_type( type, func ) ).second;
}
