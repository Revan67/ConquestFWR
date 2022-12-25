#ifndef GENDATA_H
#define GENDATA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               GenData.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/GenData.h 7     6/17/99 12:28p Jasony $
*/			    
//---------------------------------------------------------------------------

#ifndef DACOM_H
#include <Dacom.h>
#endif

#ifndef DBASEDATA_H
#include <DBaseData.h>
#endif

//----------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ICQFactory : public IDAComponent
{
	virtual HANDLE CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *data) = 0;

	virtual BOOL32 DestroyArchetype (HANDLE hArchetype) = 0;

	virtual GENRESULT CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance) = 0;
};
//----------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE IGeneralData : public IDAComponent
{
	virtual PGENTYPE GetArchetype (const C8 *name) = 0;

	virtual PGENTYPE LoadArchetype (const C8 *name) = 0;

	virtual BOOL32 UnloadArchetype (const C8 *name) = 0;

	virtual GENRESULT CreateInstance (PGENTYPE pArchetype, IDAComponent **pInstance) = 0;

	virtual GENRESULT CreateInstance (const char *name, IDAComponent **pInstance) = 0;

	virtual const char * GetArchName (PGENTYPE pArchetype) = 0;

	virtual void * GetArchetypeData (PGENTYPE pArchetype) = 0;

	virtual void * GetArchetypeData (const C8 * name) = 0;

	virtual void * GetArchetypeData (const C8 * name, U32 & dataSize) = 0;		// also returns data size

	virtual void AddRef (PGENTYPE pArchetype) = 0;		// add to the usage count

	virtual void Release (PGENTYPE pArchetype) = 0;		// decrement the usage count

	/*
	virtual GENRESULT GetDataFile (struct IDocument ** ppDocument) = 0;
		ppDocument: Address of pointer that gets set to the document instance.
	*/
	virtual GENRESULT GetDataFile (struct IDocument ** ppDocument) = 0;

	virtual void FlushUnusedArchetypes (void) = 0;
};

//-------------------------------------------------------------------
//-------------------------END GenData.h---------------------------
//-------------------------------------------------------------------


#endif