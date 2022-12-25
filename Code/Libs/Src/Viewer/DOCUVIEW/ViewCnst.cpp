//--------------------------------------------------------------------------//
//                                                                          //
//                               ViewCnst.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Header: /Libs/Src/Viewer/DataViewer/ViewCnst.cpp 2     11/15/02 10:42a Tmauer $

					View Contructor, parses C/C++ header, 
*/			    
//--------------------------------------------------------------------------//

// 4514: unused inline function
// 4201: nonstandard no-name structure use
// 4100: formal parameter was not used
// 4512: assignment operator could not be generated
// 4245: conversion from signed to unsigned
// 4127: constant condition expression
// 4355: 'this' used in member initializer
// 4244: conversion from int to unsigned char, possible loss of data
// 4200: zero sized struct member
// 4710: inline function not expanded
// 4702: unreachable code
// 4786: truncating function name (255 chars) in browser info
#pragma warning (disable : 4514 4201 4100 4512 4245 4127 4355 4244 4710 4702 4786)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning (disable : 4355 4201)

#include "ViewCnst.h"
#include "SymTable.h"
#include "DataView.h"
#include "Document.h"
#include "TComponent.h"
#include "IConnection.h"
#include "TSmartPointer.h"
#include "HeapObj.h"
#include "DataParser.h"

extern HINSTANCE hInstance;
extern ICOManager * DACOM;

static char interface_name[] = "IViewConstructor";

extern SymbolManager SymbolTable;

static BOOL32 CorrelateSymbol (SYMBOL pOldSymbol, char *pOldData, SYMBOL pNewSymbol, char *pNewData);
void __stdcall CreateDataParser (SYMBOL symbol, IDataParser ** pParser);

//--------------------------------------------------------------------------
//  
struct ViewConstructor : public IViewConstructor, IViewConstructor2, IAggregateComponent
{
	DWORD dwRefs;
	SYMBOL table;
	IDAComponent * outer;

	BEGIN_DACOM_MAP_INBOUND(ViewConstructor)
	DACOM_INTERFACE_ENTRY(IViewConstructor)
	DACOM_INTERFACE_ENTRY(IViewConstructor2)
	DACOM_INTERFACE_ENTRY2(IID_IViewConstructor,IViewConstructor)
	DACOM_INTERFACE_ENTRY2(IID_IViewConstructor2,IViewConstructor2)
	END_DACOM_MAP()

	
	ViewConstructor (void)
	{
		dwRefs=1;
	}

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "ViewConstructor");
	}

	/* IDAComponent members */

	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)           (void);
	DEFMETHOD_(U32,Release)          (void);

	/* IComponentFactory members */

	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);

	/* IAggregateComponent members */

	DEFMETHOD(Initialize) (void);

	/* IViewConstructor members */

	DEFMETHOD(ParseFile) (const C8 *filename);

	DEFMETHOD(ParseMemory) (const C8 *memory);

	DEFMETHOD_(BOOL32,EnumerateTypes) (IVIEWCNST_ENUM_PROC proc, void *context=0);

	DEFMETHOD_(U32,GetTypeSize) (const C8 *typeName);

	/* IViewConstructor2 members */

	DEFMETHOD_(BOOL32,EnumerateTypes) (IVIEWCNST_ENUM_PROC2 proc, void *context=0);

	DEFMETHOD_(U32,GetTypeSize) (SYMBOL symbol);
	
	DEFMETHOD_(HANDLE,ParseNewMemory) (const C8 *memory);	// create a secondary symbol table

	DEFMETHOD_(SYMBOL,GetSymbol) (HANDLE hSymbolList, const C8 *pszSymbolName); // get handle to specific type
	
	DEFMETHOD(DestroySymbols) (HANDLE hSymbolList);

	DEFMETHOD_(void,CorrelateSymbol) (SYMBOL oldSymbol, void *pOldData, SYMBOL newSymbol, void *pNewData);

	DEFMETHOD_(BOOL32,IsEqual) (SYMBOL oldSymbol, SYMBOL newSymbol);

	DEFMETHOD_(BOOL32,HasVariableSize) (SYMBOL symbol);

	/* ViewConstructor members */

	GENRESULT CreateViewer (VIEWDESC *lpDesc, void **instance);

	IDAComponent * getBase (void)
	{
		return (IViewConstructor *)this;
	}
};
//--------------------------------------------------------------------------//
//
GENRESULT ViewConstructor::Initialize (void)
{
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT ViewConstructor::CreateInstance (DACOMDESC *lpDesc, void **instance)
{
	GENRESULT		result        = GR_OK;
	ViewConstructor *pNewInstance = NULL;
	AGGDESC * aggDesc = (AGGDESC *) lpDesc;

	if (lpDesc==NULL || (lpDesc->interface_name==NULL))
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}

   //
   // If unsupported interface requested, fail call
   //

	if (strcmp(lpDesc->interface_name, "IDataParser") == 0)
	{
		if (lpDesc->size != sizeof(DPARSERDESC))
		{
			result = GR_INVALID_PARMS;
			goto Done;
		}

		CreateDataParser(((DPARSERDESC *)lpDesc)->symbol, (IDataParser **) instance);
		if (*instance==0)
			return GR_GENERIC;
		return GR_OK;
	}
	else
	if (strcmp(lpDesc->interface_name, interface_name))
	{
		return CreateViewer((VIEWDESC *) lpDesc, instance);
	}

	if (lpDesc->size == sizeof(AGGDESC))
	{
		outer = aggDesc->outer;
		*aggDesc->inner = getBase();
	}
	else
	if (lpDesc->size != sizeof(DACOMDESC))
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}

	if (table == 0 && (table = SymbolTable.set_reserved()) != 0)
		table->AddRef();

	AddRef();
	pNewInstance = this;

Done:
	*instance = pNewInstance;
	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT ViewConstructor::QueryInterface (const C8 *interface_name, void **instance)
{
	int i;
	const _DACOM_INTMAP_ENTRY * interfaces = _GetEntriesIn();

	for (i = 0; interfaces[i].interface_name; i++)
	{
		if (strcmp(interfaces[i].interface_name, interface_name) == 0)
		{
			IDAComponent *result = (IDAComponent *) (((char *) this) + interfaces[i].offset);
			result->AddRef();
			*instance = result;
			return GR_OK;
		}
	}

	*instance = 0;
	return GR_INTERFACE_UNSUPPORTED;
}
//--------------------------------------------------------------------------//
//
U32 ViewConstructor::AddRef (void)
{
	dwRefs++;
	return dwRefs;
}
//--------------------------------------------------------------------------//
//
U32 ViewConstructor::Release (void)
{
	if (dwRefs > 0)
		dwRefs--;
	if (dwRefs <= 1)
	{
		SYMBOL tmp;

		outer = 0;

		while (table)
		{
			tmp = table->link;
			table->Release();
			table = tmp;
		}

		SymbolTable.free();
			
		if (dwRefs == 0)
		{
 			dwRefs++;	// prevent infinite looping
			delete this;
			return 0;
		}
	}

	return dwRefs;
}

SYMBOL parse_file   (const char *name);
SYMBOL parse_memory (const char *memory);

//--------------------------------------------------------------------------//
//
GENRESULT ViewConstructor::ParseFile (const C8 *value)
{
	SYMBOL sym = parse_file(value), tmp;

	if (sym)
	{
		if ((tmp = table) != 0)
		{
			while (tmp->link)
				tmp = tmp->link;

			tmp->link = sym;	
		}
		else
			table = sym;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT ViewConstructor::ParseMemory (const C8 *memory)
{
	SYMBOL sym = parse_memory(memory), tmp;

	if (sym)
	{
		if ((tmp = table) != 0)
		{
			while (tmp->link)
				tmp = tmp->link;

			tmp->link = sym;	
		}
		else
			table = sym;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
BOOL32 ViewConstructor::EnumerateTypes (IVIEWCNST_ENUM_PROC proc, void *context)
{
	BOOL32 result = 1;
	SYMBOL list = table;

	while (list && result)
	{
		result = proc(this, list->name,	context);
		list = list->link;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
U32 ViewConstructor::GetTypeSize (const C8 *typeName)
{
	SYMBOL list = table;

	while (list)
	{
		if (strcmp(typeName, list->name) == 0)
			return list->size;
		list = list->link;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 ViewConstructor::EnumerateTypes (IVIEWCNST_ENUM_PROC2 proc, void *context)
{
	BOOL32 result = 1;
	SYMBOL list = table;

	while (list && result)
	{
		result = proc(this, list->name,	list, context);
		list = list->link;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
U32 ViewConstructor::GetTypeSize (SYMBOL symbol)
{
	return symbol->size;
}
//--------------------------------------------------------------------------//
//
HANDLE ViewConstructor::ParseNewMemory (const C8 *memory)
{
	SYMBOL oldTable = table;

	SymbolTable.free();
	if ((table = SymbolTable.set_reserved()) != 0)
		table->AddRef();
	
	SYMBOL sym = parse_memory(memory), tmp;

	if (sym)
	{
		if ((tmp = table) != 0)
		{
			while (tmp->link)
				tmp = tmp->link;

			tmp->link = sym;	
		}
		else
			table = sym;
	}

	tmp = table;
	table = oldTable;	// restore old state
	SymbolTable.free();

	return tmp;			// return new parsed list
}
//--------------------------------------------------------------------------//
//
SYMBOL ViewConstructor::GetSymbol (HANDLE hSymbolList, const C8 *pszSymbolName)
{
	SYMBOL list = (SYMBOL) hSymbolList;

	if (list == 0)
		list = table;

	while (list)
	{
		if (strcmp(pszSymbolName, list->name) == 0)
			break;
		list = list->link;
	}

	return list;
}
//--------------------------------------------------------------------------//
//
GENRESULT ViewConstructor::DestroySymbols (HANDLE hSymbolList)
{
	SYMBOL tmp, list = (SYMBOL) hSymbolList;

	while (list)
	{
		tmp = list->link;
		list->Release();
		list = tmp;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void ViewConstructor::CorrelateSymbol (SYMBOL oldSymbol, void *pOldData, SYMBOL newSymbol, void *pNewData)
{
	memset(pNewData, 0, DataViewer::GetType(newSymbol)->size);		// initialize output to 0

	::CorrelateSymbol(oldSymbol, (char *)pOldData, newSymbol, (char *)pNewData);
}
//--------------------------------------------------------------------------//
//
BOOL32 ViewConstructor::IsEqual (SYMBOL oldSymbol, SYMBOL newSymbol)
{
	return DataViewer::IsEqual(oldSymbol, newSymbol);
}
//--------------------------------------------------------------------------//
//
BOOL32 ViewConstructor::HasVariableSize (SYMBOL symbol)
{
	return DataViewer::HasVariableSize(symbol);
}
//--------------------------------------------------------------------------//
//
GENRESULT ViewConstructor::CreateViewer (VIEWDESC *lpDesc, void **instance)
{
	GENRESULT result = GR_GENERIC;
	DAComponent<DataViewer> *pNewInstance = NULL;
	SYMBOL list = 0;
	COMPTR<IDAConnectionPoint> connection;
	HWND hOwnerWindow=0;

	//
	// If unsupported interface requested, fail call
	//

	if (strcmp(lpDesc->interface_name, "IViewer"))
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}

	if (lpDesc->size != sizeof(*lpDesc))
	{
		// create viewer from SYMBOL directly
		if (lpDesc->size == sizeof(*lpDesc) + 4 && strcmp(lpDesc->className, "SYMBOL")==0)
		{
			list = ((SYMBOL *)(lpDesc+1))[0];
			hOwnerWindow = (HWND) lpDesc->hOwnerWindow;
		}
		else
		if (lpDesc->size != sizeof(*lpDesc) - 8)		// old style
		{
			result = GR_INTERFACE_UNSUPPORTED;
			goto Done;
		}
	}
	else
	{
		hOwnerWindow = (HWND) lpDesc->hOwnerWindow;
	}

	if (lpDesc->doc->QueryOutgoingInterface("IDocumentClient", connection) != GR_OK)
	{
		result = GR_GENERIC;
		goto Done;
	}

	if (list==0)
	{
		list = table;
		while (list)
		{
			if (strcmp(list->name, lpDesc->className) == 0)
				break;
			list = list->link;
		}
	}
	
	if (list)
	{
		if (lpDesc->doc->GetFileSize() == (DWORD) list->size || DataViewer::HasVariableSize(list))
		{
			if ((pNewInstance = new DAComponent<DataViewer>) == 0)
			{
				result = GR_OUT_OF_MEMORY;
				goto Done;
			}

			pNewInstance->doc = lpDesc->doc;
			pNewInstance->symbol = list;
			pNewInstance->hParentWindow = hOwnerWindow;
			pNewInstance->set_spelling_list(lpDesc->spellingSet);
			list->AddRef();

			strncpy(pNewInstance->szClassName, list->name, sizeof(pNewInstance->szClassName)-1);
			strcpy_s(pNewInstance->szInstanceName, "\1(Instance name goes here)\1");
			if (connection->Advise((IDocumentClient *)pNewInstance, &pNewInstance->connHandle) != GR_OK ||
				pNewInstance->init() == 0)
			{
				connection->Unadvise(pNewInstance->connHandle);
				list->Release();
				delete pNewInstance;
				pNewInstance = 0;
			}
			else
			{
				pNewInstance->OnUpdate(lpDesc->doc);
				pNewInstance->CreateView();
				result = GR_OK;
			}
		}
	}
	
Done:
	*instance = pNewInstance;
	return result;
}
//--------------------------------------------------------------------------//
//
static SYMBOL GetTypeByName (SYMBOL list, SYMBOL match)
{
	const char * matchName;
	const char * oldName;

	if ((matchName = match->name) == 0)
	{
		if ((matchName = DataViewer::GetType(match)->name) == 0)		// ambiguous case
			return 0;
	}

	while (list)
	{
		if ((oldName = list->name) == 0)
			oldName = DataViewer::GetType(list)->name;
		
		//if (oldName && strcmp(oldName, matchName) == 0)
		if (oldName == matchName)	// all names come from the same table
			break;
		list = list->link;
	}

	return list;
}
//--------------------------------------------------------------------------//
// return TRUE if symbols are equal
//
BOOL32 CorrelateSymbol (SYMBOL pOldSymbol, char *pOldData, SYMBOL pNewSymbol, char *pNewData)
{
	SYMBOL type = DataViewer::GetType(pNewSymbol);
	BOOL32 result = 1;

	switch (type->kind)
	{
	case BASICTYPE:
	case ENUMSYM:
		result &= DataViewer::CopySymValue(pOldSymbol, 0, (const char *) pOldData, pNewSymbol, 0, (char *)pNewData);
		break;

	case RECORDSYM:
	{
		SYMBOL pOldSubType;

		pOldSymbol = DataViewer::GetType(pOldSymbol);
		pOldSubType = pOldSymbol->datatype;	// this should be a VARSYM, can be NULL if empty struct!
		
		//
		// do not correlate unions. that causes too much chaos
		//
		if (pOldSubType==0 || (pOldSubType->link && pOldSubType->link->offset == 0 && pOldSubType->bit_count==0))	// union if next var is also at offset 0
		{	
			memcpy(pNewData, pOldData, __min(pOldSymbol->size, type->size));
		}
		else	// we are a struct
		{
			pOldSymbol = pOldSubType;
			type = type->datatype;									// this should be a VARSYM

			while (type)
			{
				if ((pOldSubType = GetTypeByName(pOldSymbol, type)) != 0)
					result &= CorrelateSymbol(pOldSubType, pOldData+pOldSubType->offset, type, pNewData+type->offset);
				else
					result = 0;

				type = type->link;
			}
		}
	}
		break;

	case ARRAYSYM:
	{
		int i = type->arrayval;		// i = num entries in the array

		pOldSymbol = DataViewer::GetType(pOldSymbol);
		if (i != pOldSymbol->arrayval)
			result = 0;
		i = __min(i, pOldSymbol->arrayval);	// i min of old and new

		pOldSymbol = pOldSymbol->datatype;		// this should be some kind of type
		type = type->datatype;					// this should be some kind of type
		if (type->kind != pOldSymbol->kind)
			result = 0;
		else
		{
			if (type->kind == BASICTYPE && type->size == pOldSymbol->size)
			{
				memcpy(pNewData, pOldData, type->size * i);
				pOldData+=type->size*i;
				pNewData+=type->size*i;
			}
			else
			while (i-- > 0)
			{
				result &= CorrelateSymbol(pOldSymbol, pOldData, type, pNewData);
				pOldData+=pOldSymbol->size;
				pNewData+=type->size;
			}
		}
	}
	break;
	
	case POINTERSYM:
		break;
	}


	return result;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void RegisterViewConstructor (ICOManager * DACOM)
{
	ViewConstructor *view;

	if ((view = new ViewConstructor) != 0)
	{
		DACOM->RegisterComponent((IViewConstructor *)view, interface_name);
		view->Release();
	}
}

//--------------------------------------------------------------------------//
//----------------------------End ViewCnst.cpp------------------------------//
//--------------------------------------------------------------------------//
