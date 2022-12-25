#ifndef ARCHLIST_H
#define ARCHLIST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               ArchList.H                                  //
//                                                                          //
//--------------------------------------------------------------------------//
//
//  $Author: Ajackson $
//  Management of the archetype list
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef OBJCLASS_H
#include "ObjClass.h"
#endif

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#pragma warning(disable : 4200)

struct BASIC_DATA;
struct IBaseObject;
////--------------------------------------------------------------------------//
////
struct ARCHDATATYPE
{
	C8 name[GT_PATH];
	BASIC_DATA * objData;
	U32 dataSize;		// size of data chunk in bytes
};

//--------------------------------------------------------------------------//
//
struct ARCHDATA
{
	U32 numArchetypes;
	ARCHDATATYPE type[];
};

//----------------------------------------------------------------------------
//

//struct DACOM_NO_VTABLE IArchetypeList : public IDAComponent
//{
//	virtual	PARCHETYPE GetArchetype (const C8 *name) = 0;
//
//	virtual	PARCHETYPE LoadArchetype (const C8 *name) = 0;
//
//	virtual	BOOL32 UnloadArchetype (const C8 *name) = 0;
//
//	virtual	IBaseObject * CreateInstance (PARCHETYPE pArchetype) = 0;
//
//	virtual	IBaseObject * CreateInstance (const char *name) = 0;
//
//	virtual	const char * GetArchName (PARCHETYPE pArchetype) = 0;
//
//	virtual const char * GetArchName (U32 dwArchetypeID) = 0;
//
//	virtual	void * GetArchetypeData (PARCHETYPE pArchetype) = 0;
//
//	virtual	void * GetArchetypeData (const C8 * name) = 0;
//
//	// the following 4 methods deal with converting local pointers to network safe offsets...
//	virtual U32 GetArchetypeDataID (const C8 * name) = 0;
//
//	virtual U32 GetArchetypeDataID (PARCHETYPE pArchetype) = 0;
//
//	virtual void * GetArchetypeData (U32 dwArchetypeID) = 0;
//
//	virtual PARCHETYPE LoadArchetype (U32 dwArchetypeID) = 0;
//
//	virtual void * GetArchetypeHandle (PARCHETYPE) =0;
//
//	virtual	BOOL32 EnumerateArchetypeData (struct IArchetypeEnum * enumerator) = 0;
//
//	virtual	BOOL32 EnumerateArchetypeDataBySubArch (struct IArchetypeEnum * enumerator, const char* subarch) = 0;
//
//	virtual	void AddRef (PARCHETYPE pArchetype, const char * szUser) = 0;		// add to the usage count
//
//	virtual	void Release (PARCHETYPE pArchetype, const char * szUser) = 0;		// decrement the usage count
//
//	virtual void Close (void) = 0;
//
//	virtual	BOOL32 IsSubArch( PARCHETYPE pArch, const char* subarch) = 0;
//
//	virtual	PARCHETYPE GetArchetypeByCRC32 ( U32 crc32 ) = 0;
//
//	virtual	U32 GetCRC32ForArchetype (PARCHETYPE) = 0;
//};

//IDataList - a simpler version of IArchetypeList that handles raw binary data

struct DACOM_NO_VTABLE IDataList : public IDAComponent
{
	virtual const char * GetArchName (U32 dwArchetypeID) = 0;

	virtual U32 GetArchetypeDataID (const C8 * name) = 0;

	virtual	void * GetArchetypeData (const C8 * name) = 0;

	virtual void * GetArchetypeData (U32 dwArchetypeID) = 0;

	virtual U32 GetArchetypeDataSize (U32 dwArchetypeID) = 0;

	virtual void Close (void) = 0;

	virtual	BOOL32 EnumerateArchetypeData (struct IArchetypeEnum * enumerator, DWORD context = 0) = 0;

	virtual	BOOL32 EnumerateArchetypeDataBySubArch (struct IArchetypeEnum * enumerator, const char* subarch) = 0;
};

////----------------------------------------------------------------------------
////
//
//struct DACOM_NO_VTABLE IObjectFactory : public IDAComponent
//{
//	virtual	HANDLE CreateArchetype (const char *szArchname, OBJCLASS objClass, void *data) = 0;
//
//	virtual	BOOL32 DestroyArchetype (HANDLE hArchetype) = 0;
//
//	virtual	IBaseObject * CreateInstance (HANDLE hArchetype) = 0;
//};
//
//----------------------------------------------------------------------------

struct IArchetypeEnum
{
	virtual	BOOL32 ArchetypeEnum (const char * name, void *data, U32 size, DWORD context) = 0;
};

#endif
