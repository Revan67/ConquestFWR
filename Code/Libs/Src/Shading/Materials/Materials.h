// Materials.h
//
// Declarations that are common for all material related code shared
// in the Materials.h
// 

#ifndef __Materials_h__
#define __Materials_h__

//

#pragma warning( disable: 4018 4100 4201 4511 4512 4530 4663 4688 4710 4786 )

//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <map>
#pragma warning( disable: 4018 4201 4245 4663 )	// for some reason this is required *right here* again
#include <vector>
#include <string>
#include <list>
#include <mmsystem.h>

//

#include "dacom.h"
#include "FDump.h"
#include "TempStr.h"
#include "TSmartPointer.h"
#include "tcomponent.h"
#include "da_heap_utility.h"
#include "AllocLite.h"
#include "FileSys_Utility.h"
#include "IProfileParser_Utility.h"
#include "ITextureLibrary.h"
#include "IVertexBufferManager.h"
#include "IMaterialLibrary.h"
#include "IMaterial.h"
#include "IStateMaterial.h"
#include "IMaterialProperties.h"

//

// Type macros used by DECLARE_MATERIAL
//
#define IS_AGGREGATE(cls) DAComponentFactory2< DAComponentAggregate<cls>, AGGDESC >
#define IS_SIMPLE(cls) DAComponentFactory< DAComponent<cls>, DACOMDESC >


// DECLARE_MATERIAL
//
// Use this macro to declare the code to register this material.
// See the standard materials for examples of how to use this macro.
//
#define DECLARE_MATERIAL(comp,type) \
bool Register_ ## comp( void ) \
{\
	IComponentFactory *s ## comp = new type(comp)( CLSID_ ## comp );\
	if( s ## comp == NULL ) {\
		return false;\
	}\
	DACOM_Acquire()->RegisterComponent( s ## comp, CLSID_ ## comp, DACOM_NORMAL_PRIORITY );	\
	s ## comp ->Release();	\
	return true; \
}

// DECLARE_MATERIAL_WITH_IMPL
//
// Use this macro to declare the code to register this material.
// See the standard materials for examples of how to use this macro.
//
#define DECLARE_MATERIAL_WITH_IMPL(mtl,impl,type) \
bool Register_ ## mtl( void ) \
{\
	IComponentFactory *s ## mtl = new type(impl)( CLSID_ ## mtl );\
	if( s ## mtl == NULL ) {\
		return false;\
	}\
	DACOM_Acquire()->RegisterComponent( s ## mtl, CLSID_ ## mtl, DACOM_NORMAL_PRIORITY );	\
	s ## mtl ->Release();	\
	return true; \
}

// REGISTER_MATERIAL
//
// Place a call to this macro in the DllMain in Materials.cpp
// to register a material component.  See DllMain() for more information.
//
#define REGISTER_MATERIAL(comp) \
extern bool Register_ ## comp( void );	\
	Register_ ## comp();	


#endif // EOF
