//--------------------------------------------------------------------------//
//                                                                          //
//                               DataView.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Ajackson $
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

#if !defined(_XBOX)
	#include <commctrl.h>               // Common controls
	#include <commdlg.h>
	#include <windowsx.h> 
	#include "TreeView.h"
#endif

#include <direct.h>
#include <stdio.h>

#include "DataView.h"
#include "Document.h"
#include "SymTable.h"
#include "TokenDef.h"
#include "HeapObj.h"
#include "IConnection.h"
#include "fdump.h"

#include <ctype.h>
#include <stdlib.h>

#include "Resource.h"

#if !defined(_XBOX)
	HINSTANCE hInstance;
	ICOManager * DACOM;
#else
	extern HINSTANCE hInstance;
	extern ICOManager * DACOM;
#endif

#define IDM_ADD_ELEMENT 2000

static char szDefaultValue[] = "...";
static char szInvalid[] = "???";
static char szUnknown[] = "<Unknown>";
static char szDataPath[MAX_PATH] = "C:\\";


/*
	"display_state"				TRUE if data is being displayed. (BOOL32)
	"display_value"				Short text description of data. (ASCIIZ string)
	"class_name"				Data type identifier. [read-only] (ASCIIZ string)
	"instance_name"				Instance identifier. (ASCIIZ string)
	"main_window"				Main window of viewer. (HWND)
	"rect"						Recomended size/position of window. (RECT *)
	"read_only"					TRUE if data cannot be modified. (BOOL32)
	"string_length"				MAX size buffer needed to hold an ASCIIZ property. (LONG)
	"auto_close"				TRUE if display state automatically turns off when it loses focus.(BOOL32)
	"hex_numbers"				TRUE if numbers are displayed hexidecimal, FALSE if decimal (BOOL32)
*/

#define USR_CLOSE				(WM_USER+1)

//--------------------------------------------------------------------------
//---------------------------DataViewer class-------------------------------
//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------//
//
#if !defined(_XBOX)
int __stdcall _localAnsiToWide (const char * input, wchar_t * output, U32 bufferSize)
{
		// 932 is Japanese code page
	return MultiByteToWideChar(CP_ACP, 0, input, -1, output, (bufferSize/sizeof(output[0])) );
}
//--------------------------------------------------------------------------//
//
int __stdcall _localWideToAnsi (const wchar_t * input, char * output, U32 bufferSize)
{
		// 932 is Japanese code page
	return WideCharToMultiByte(CP_ACP, 0, input, -1, output, bufferSize, 0, 0);
}
#else
	int __stdcall _localAnsiToWide (const char * input, wchar_t * output, U32 bufferSize);
	int __stdcall _localWideToAnsi (const wchar_t * input, char * output, U32 bufferSize);
#endif
//--------------------------------------------------------------------------
//
const char* __stdcall DataViewer_GetFile( const char* _mask, int& _fnOffset );
BOOL CALLBACK DataViewer_TreeViewDlgProc(HWND hwnd, UINT message, UINT wParam, LONG lParam);

//--------------------------------------------------------------------------
//
DataViewer::~DataViewer (void)
{
	DataViewer *tmp;

	while (pViewerList)
	{
		tmp = pViewerList->pNext;
		pViewerList->symbol = 0;
		pViewerList->pLastData = 0;
		pViewerList->dwLastDataSize = 0;
		pViewerList->BaseComponent()->Release();
		pViewerList = tmp;
	}
	
	if (hMainWindow)
		DestroyWindow(hMainWindow);
	hMainWindow=0;

	if (pLastData)
		free(pLastData);
	pLastData=0;
	dwLastDataSize = 0;
	
	if (symbol)
		symbol->Release();
	symbol=0;

	if (doc)
		OnClose(doc);
}
//--------------------------------------------------------------------------
//
void * DataViewer::operator new (size_t size)
{
	return calloc(size, 1);
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_display_state (BOOL32 state)
{
#if !defined(_XBOX)
	if (hMainWindow==0)
		CreateView();
	if (hMainWindow)
	{
		bVisible = (state != 0);
		if (bVisible && bTopMost)
		{
			SetWindowPos(hMainWindow,HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
		}
		if (hParentWindow && bVisible==0)
			SetFocus(hParentWindow);
		ShowWindow(hMainWindow, (bVisible) ? SW_SHOW:SW_HIDE);
		if (bVisible)
		{	
			if (hListView)
				SetFocus(hListView);
			else
			if (hEdit)
				SetFocus(hEdit);
			else
			if (hCombo)
				SetFocus(hCombo);

			if (pParentViewer==0 && bExtraReference==false)
			{
				bExtraReference = true;
				BaseComponent()->AddRef();			// add extra reference for the "user"
			}
		}
		else
		{
			DataViewer *tmp = pViewerList;

			while (tmp)
			{
				if (tmp->hMainWindow)
					SendMessage(tmp->hMainWindow, WM_CLOSE, 0, 0);
				tmp = tmp->pNext;
			}

			if (hParentWindow)
				SetForegroundWindow(hParentWindow);

			if (bExtraReference)
			{
				bExtraReference = false;
				BaseComponent()->Release();			// release extra reference
			}
		}
	}
	else
		return GR_GENERIC;
#endif
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_viewer_path( const char *path )
{
	SetDataDirectory( path );
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_display_state (BOOL32 *state)
{
	if (state)
	{
		*state = (bVisible && hMainWindow);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_display_value (const C8 *name)
{
	return GR_GENERIC;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_display_value (C8 *name)
{
	if (name)
	{	 
		strcpy(name, szDisplayName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_class_name (C8 *name)
{
	if (name)
	{
		strcpy(name, szClassName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_instance_name (const C8 *name)
{
	if (name)
	{
		SubstituteInstanceName(name);
		UpdateView();
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_instance_name (C8 *name)
{
	if (name)
	{
		strcpy(name, szInstanceName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_main_window (void **hwnd)
{
	if (hwnd)
	{
		*hwnd = hMainWindow;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_rect (const struct tagRECT *pRect)
{
#if !defined(_XBOX)
	if (pRect)
	{
		rect = *pRect;
		if (hMainWindow)
			MoveWindow(hMainWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom-rect.top, TRUE);
		else
			return GR_GENERIC;
		return GR_OK;
	}
#endif
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_rect (struct tagRECT *pRect)
{
	if (pRect)
	{
		*pRect = rect;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_read_only (BOOL32 value)
{
	SetNewReadOnly(value != 0);
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_read_only (BOOL32 *value)
{
	if (value)
	{
		*value = (userModifiers & MODIFIER_READONLY) != 0;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_auto_close (BOOL32 value)
{
	bAutoClose = (value != 0);
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_auto_close (BOOL32 *value)
{
	if (value)
	{
		*value = bAutoClose;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_hex_numbers (BOOL32 value)
{
	SetNewHexValue(value != 0);
	UpdateData(pLastData, dwLastDataSize);
	UpdateView();
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::get_hex_numbers (BOOL32 *value)
{
	if (value)
	{
		*value = (userModifiers & MODIFIER_HEXVIEW) != 0;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::set_spelling_list (IDAComponent * set)
{
	if(stringSet)
		stringSet->Release();
	stringSet = NULL;
	if(set)
		set->QueryInterface("IStringSet",(void **)(&stringSet));
	if(stringSet)
		stringSet->AddRef();

	DataViewer * search = pViewerList;
	while(search)
	{
		search->set_spelling_list(set);
		search = search->pNext;
	}
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::update (IDocument * doc)
{
	DWORD dwRead, dwNewSize;

	dwNewSize = doc->GetFileSize();

	if (pLastData==0 || dwNewSize != dwLastDataSize)
	{
		free(pLastData);
		pLastData = 0;
		if ((dwLastDataSize = dwNewSize) != 0)		// guard againat zero size array
		{
			if ((pLastData = (char *) malloc(dwNewSize)) == 0)
			{
				dwLastDataSize = 0;
				return GR_OUT_OF_MEMORY;
			}
		}
	}

	doc->SetFilePointer(0,0);
	if (doc->ReadFile(0, pLastData, dwNewSize, &dwRead, 0) == 0)
	{
		return GR_FILE_ERROR;
	}

	UpdateData(pLastData, dwLastDataSize);
	UpdateView();

	return GR_OK;
}
//--------------------------------------------------------------------------
//
void DataViewer::addToFile (U32 bytesToAdd)
{
	if (pParentViewer)
		pParentViewer->addToFile(bytesToAdd);
	else
	{
		if (doc!=0)
		{
			doc->SetFilePointer(0, dwLastDataSize+bytesToAdd);
			if (doc->SetEndOfFile())
				doc->UpdateAllClients();
		}	
	}
}
//--------------------------------------------------------------------------
//
void DataViewer::onAddRequested (void)
{
	// assumes that we are a zero length array
	ASSERT(GetType(symbol)->kind==ARRAYSYM && GetType(symbol)->arrayval==0);
	addToFile(GetType(symbol)->datatype->size);		// add this many bytes to the file
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::OnUpdate (struct IDocument *_doc, const C8 *message, void *parm)
{
	if (doc != _doc)
		return GR_GENERIC;

	return update(_doc);
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::OnClose (struct IDocument *document)
{
	if (document == doc && doc)	//is this the right instance?
	{
		COMPTR<IDAConnectionPoint> connection;

		if (doc->QueryOutgoingInterface(IID_IDocumentClient, connection) != GR_OK)
			return GR_GENERIC;

		connection->Unadvise(connHandle);
		doc = 0;
		if (bExtraReference)
		{
			bExtraReference = false;
			BaseComponent()->Release();			// release extra reference
		}
	}

	return GR_OK;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::WriteNewData (void)
{
	if (pParentViewer)
		return pParentViewer->WriteNewData();
	else
	{
		BOOL32 result = 0;
		DWORD dwWrite;

		if (doc!=0 && pLastData)
		{
			doc->SetFilePointer(0,0);
			if (doc->WriteFile(0, pLastData, doc->GetFileSize(), &dwWrite, 0))
				result = 1;
			doc->UpdateAllClients();
		}	
		return result;
	}
}
//--------------------------------------------------------------------------
//
SYMBOL DataViewer::GetType (SYMBOL sym)
{
	switch (sym->kind)
	{
	case TYPESYM:
	case VARSYM:
		if (sym->datatype)
			return GetType(sym->datatype);
	}

	return sym;
}
//--------------------------------------------------------------------------
// is char or wchar_t
//
BOOL32 DataViewer::IsStringBaseType (SYMBOL sym, U32 modifiers)
{
	modifiers |= sym->modifiers;
	switch (sym->kind)
	{
	case BASICTYPE:
		if (sym->basicdt == INTEGER && (modifiers & MODIFIER_UNSIGNED)==0 && sym->size==1)
			return 1;
		break;

	case VARSYM:
	case TYPESYM:
		if (sym->datatype == 0)
			return 0;
		if (sym->name && strcmp(sym->name, "wchar_t")==0)
			return 1;
		return IsStringBaseType(sym->datatype, modifiers);
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::GetSymValue (SYMBOL sym, U32 modifiers, const char *pData, char *value, U32 symbolSize, IStringSet * set)
{
	int bit_offset, bit_count;

	bit_offset = sym->bit_offset;
	bit_count  = sym->bit_count;
Begin:
	modifiers |= sym->modifiers;
	if (IsBadReadPtr(pData, symbolSize))
		strcpy(value, szInvalid);
	else
	switch (sym->kind)
	{
	case BASICTYPE:
		switch (sym->basicdt)
		{
		case REAL:		// double or float
			if (symbolSize == sizeof(double))
				_gcvt(*((double *)pData), 8, value);
			else
				_gcvt(*((float *)pData), 8, value);
			break;

		case STRINGTYPE:
			strcpy(value, pData);
			break;

		default:	// integer case
			if (bit_count)
			{
				if ((modifiers & MODIFIER_HEXVIEW) == 0)
				switch (symbolSize)
				{
				case 1:
				case 2:
				case 4:
					if ((modifiers & MODIFIER_UNSIGNED) == 0)
						_itoa(GetBitField32(pData, bit_offset, bit_count), value, 10);
					else
						_ultoa(GetBitFieldU32(pData, bit_offset, bit_count), value, 10);
					break;
				case 8:
					if ((modifiers & MODIFIER_UNSIGNED) == 0)
						_i64toa(GetBitField64(pData, bit_offset, bit_count), value, 10);
					else
						_ui64toa(GetBitFieldU64(pData, bit_offset, bit_count), value, 10);
					break;
				}
				else
				{
					value[0] = '0';
					value[1] = 'x';
					switch (symbolSize)
					{
					case 1:
					case 2:
					case 4:
						if ((modifiers & MODIFIER_UNSIGNED) == 0)
							_itoa(GetBitField32(pData, bit_offset, bit_count), value+2, 16);
						else
							_itoa(GetBitFieldU32(pData, bit_offset, bit_count), value+2, 16);
						break;
					case 8:
						if ((modifiers & MODIFIER_UNSIGNED) == 0)
							_i64toa(GetBitField64(pData, bit_offset, bit_count), value+2, 16);
						else
							_i64toa(GetBitFieldU64(pData, bit_offset, bit_count), value+2, 16);
						break;
					}
				}
			}
			else
			if ((modifiers & MODIFIER_HEXVIEW) == 0)
			{
				switch (symbolSize)
				{
				case 1:
					if ((modifiers & MODIFIER_UNSIGNED) == 0)
						_itoa(*((__int8 *)pData), value, 10);
					else
						_ultoa(*((unsigned __int8 *)pData), value, 10);
					if (*((__int8 *)pData) && (modifiers & MODIFIER_UNSIGNED) == 0)
					{
						int len = strlen(value);
						value[len] = ' ';
						value[len+1] = '\'';
						value[len+2] = *((__int8 *)pData);
						value[len+3] = '\'';
						value[len+4] = 0;
					}
					break;
				case 2:
					if ((modifiers & MODIFIER_UNSIGNED) == 0)
						_itoa(*((__int16 *)pData), value, 10);
					else
						_ultoa(*((unsigned __int16 *)pData), value, 10);
					break;
				case 4:
					if ((modifiers & MODIFIER_UNSIGNED) == 0)
						_itoa(*((__int32 *)pData), value, 10);
					else
						_ultoa(*((unsigned __int32 *)pData), value, 10);
					break;
				case 8:
					if ((modifiers & MODIFIER_UNSIGNED) == 0)
						_i64toa(*((__int64 *)pData), value, 10);
					else
						_ui64toa(*((unsigned __int64 *)pData), value, 10);
					break;
				}
			}
			else
			{
				value[0] = '0';
				value[1] = 'x';
				switch (symbolSize)
				{
				case 1:
					ConvertToString8(*((unsigned __int8 *)pData), value+2);
					if (*((__int8 *)pData) && (modifiers & MODIFIER_UNSIGNED) == 0)
					{
						int len = strlen(value);
						value[len] = ' ';
						value[len+1] = '\'';
						value[len+2] = *((__int8 *)pData);
						value[len+3] = '\'';
						value[len+4] = 0;
					}
					break;
				case 2:
					ConvertToString16(*((unsigned __int16 *)pData), value+2);
					break;
				case 4:
					ConvertToString32(*((unsigned __int32 *)pData), value+2);
					break;
				case 8:
					ConvertToString64(*((unsigned __int64 *)pData), value+2);
					break;
				}
			}
			break;
		} // end switch
		break;  // end case BASICTYPE

	case VARSYM:
	case TYPESYM:
		if (sym->datatype == 0)
			return 0;
		sym = sym->datatype;
		goto Begin;
	
	case POINTERSYM:
		if (sym->datatype == 0)
			return 0;
		if (IsStringBaseType(sym->datatype, 0) == 0 || IsBadReadPtr(*((void **)pData), MAX_NAME_LENGTH))
			wsprintf(value, "0x%08X", *((U32 **)pData));
		else
		{
			value[0] = '\"';
			if (sym->datatype->size==1)
				strncpy(value+1, *((char **)pData), MAX_NAME_LENGTH-3);
			else
				_localWideToAnsi(*((wchar_t **)pData), value+1, MAX_NAME_LENGTH-3);
			strcat(value, "\"");
		}
		break;
		
	case RECORDSYM:
		return 0;
	case ARRAYSYM:
		if (sym->datatype == 0 || IsStringBaseType(sym->datatype, 0) == 0)
			return 0;
		value[0] = 0;
		if (sym->datatype->size==1)
		{
			U32 len = 0;
			if(set && (modifiers & MODIFIER_SPELLCHECK) && (!(set->HasString(pData))))
			{
				strcpy(value,"<SPELLING> ");
				len += strlen("<SPELLING> ");
			}
			strcat(value,"\"");
			len +=  __min(MAX_NAME_LENGTH-3-len, symbolSize);
			if (pData)	// can be null for variable length string
				strcat(value, pData);
			value[len+1] = 0;
		}
		else
		{
			value[0] = '\"';
			int len =  __min(MAX_NAME_LENGTH-3, symbolSize);
			if (pData)	// can be null for variable length string
				_localWideToAnsi((wchar_t *)pData, value+1, len);
			value[len+1] = 0;
		}
		strcat(value, "\"");
		break;

	case ENUMSYM:
		{
			long eval;

			if (sym->datatype == 0)
				return 0;

			if (bit_count==1 && sym->name && strcmp(sym->name, "bool")==0)
				eval = GetBitFieldU32(pData, bit_offset, bit_count);	// one bit enum
			else
			if (bit_count!=0)
				eval = GetBitField32(pData, bit_offset, bit_count);		// signed number
			else
			switch (symbolSize)
			{
			case 1:
				eval = *((unsigned __int8 *)pData);
				break;
			case 2:
				eval = *((unsigned __int16 *)pData);
				break;
			case 4:
				eval = *((unsigned __int32 *)pData);
				break;
			default:
				eval = 0;
				break;
			}

			// search through the enumeration for the right value
			SYMBOL esym = sym->datatype;
			while (esym)
			{
				if (esym->constval.intnum == eval)
				{
					strcpy(value, esym->name);
					return 1;
				}
				esym = esym->link;
			}

			// else we did not find a match
			strcpy(value, "Bad enum value");
			return 0;
		} // end ENUMSYM case

	} // end switch (sym->kind)

	return 1;
}
//--------------------------------------------------------------------------
//
static __int64 localAsciiToInt64 (const char * string)
{
	if (string[0] != '0' || toupper(string[1]) != 'X')
		return _atoi64(string);
	__int64 result=0;

	string+=2;
	while (isxdigit(string[0]))
	{
		result <<= 4;
		if (isdigit(string[0]))
			result += string[0] - '0';
		else
			result += (string[0] & ~0x20) - 'A';
		string++;
	}

	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::SetSymValue (SYMBOL sym, char *pData, const char *value, U32 symbolSize)
{
	int bit_offset, bit_count;

	bit_offset = sym->bit_offset;
	bit_count  = sym->bit_count;
Begin:
	if (IsBadWritePtr(pData, symbolSize))
		return 0;
	else
	switch (sym->kind)
	{
	case BASICTYPE:
		switch (sym->basicdt)
		{
		case REAL:		// double or float
			if (symbolSize == sizeof(double))
				*((double *)pData) = strtod(value, 0);
			else
				*((float *)pData) = (float) strtod(value, 0);
			break;

		case STRINGTYPE:
			strcpy(pData, value);
			break;

		default:	// integer case
			if (bit_count)
			{
				switch (symbolSize)
				{
				case 1:
					*((__int8 *)pData) = SetBitField32(pData, bit_offset, bit_count, strtoul(value, 0, 0));
					break;
				case 2:
					*((__int16 *)pData) = SetBitField32(pData, bit_offset, bit_count, strtoul(value, 0, 0));
					break;
				case 4:
					*((__int32 *)pData) = SetBitField32(pData, bit_offset, bit_count, strtoul(value, 0, 0));
					break;
				case 8:
					// warning!!! can only understand decimal number here!!
					*((__int64 *)pData) = SetBitField64(pData, bit_offset, bit_count, localAsciiToInt64(value));
					break;
				}
			}
			else
			{
				switch (symbolSize)
				{
				case 1:
					*((__int8 *)pData) = (__int8) strtoul(value, 0, 0);
					break;
				case 2:
					*((__int16 *)pData) = (__int16) strtoul(value, 0, 0);
					break;
				case 4:
					*((__int32 *)pData) = (__int32) strtoul(value, 0, 0);
					break;
				case 8:
					// warning!!! can only understand decimal number here!!
					*((__int64 *)pData) = localAsciiToInt64(value);
					break;
				}
			}
			break;
		} // end switch
		break;  // end case BASICTYPE

	case VARSYM:
	case TYPESYM:
		if (sym->datatype == 0)
			return 0;
		sym = sym->datatype;
		goto Begin;
	
	case POINTERSYM:
		return 0;
	case RECORDSYM:
		return 0;

	case ARRAYSYM:
		if (sym->datatype == 0 || IsStringBaseType(sym->datatype, 0) == 0)
			return 0;
		if (sym->datatype->size==1)
			strncpy(pData, value, symbolSize);
		else
			_localAnsiToWide(value, (wchar_t *)pData, symbolSize);
		break;

	case ENUMSYM:
		{

			if (sym->datatype == 0)
				return 0;

			// search through the enumeration for the right value
			SYMBOL esym = sym->datatype;
			while (esym)
			{
				if (strcmp(esym->name, value) == 0)
				{
					long eval;

					eval = esym->constval.intnum;

					if (bit_count)
						*((unsigned __int32 *)pData) = SetBitField32(pData, bit_offset, bit_count, eval);
					else
					switch (symbolSize)
					{
					case 1:
						*((unsigned __int8 *)pData) = (__int8) eval;
						break;
					case 2:
						*((unsigned __int16 *)pData) = (__int16) eval;
						break;
					case 4:
						*((unsigned __int32 *)pData) = eval;
						break;
					}
					
					return 1;
				}
				esym = esym->link;
			}

			return 0;
		} // end ENUMSYM case

	} // end switch (sym->kind)

	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::_GetSymTypeName (SYMBOL sym, U32 modifiers, char *value)
{
	BOOL32 result = GetSymTypeName(sym, modifiers, value);

	if (result)
	{
		int len = strlen(value);

		if (len && value[len-1] == ' ')
			value[len-1] = 0;
	}

	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::GetSymTypeName (SYMBOL sym, U32 modifiers, char *value)
{
	modifiers |= sym->modifiers;

	switch (sym->kind)
	{
	case ARRAYSYM:
		if (sym->datatype == 0)
			return 0;
		if (GetSymTypeName(sym->datatype, modifiers, value) == 0)
			return 0;
		strcat(value, "[]");
		break;

	case POINTERSYM:
		if (sym->datatype == 0)
			return 0;
		if (GetSymTypeName(sym->datatype, modifiers, value) == 0)
			return 0;
		strcat(value, "*");
		break;

	case TYPESYM:
	case VARSYM:
		if (sym->datatype == 0)
			return 0;
		return GetSymTypeName(sym->datatype, modifiers, value);

	default:
		if ((modifiers & MODIFIER_UNSIGNED) == 0)
			value[0] = 0;
		else
			strcpy(value, "unsigned ");
		if (sym->name)
			strcat(value, sym->name);
		strcat(value," ");
		break;
	}

	return 1;
}
//--------------------------------------------------------------------------
// initArray() can be called more than once if arrayval==0, 
// in this case we are dynamically resizing
//
BOOL32 DataViewer::initArray (void)
{
	int i;
	DataViewer *last=pViewerList;
	int len, goalSize;

	if (symbol->datatype == 0)
		goto Done;

	if (symbol->arrayval == 0)
		symbol->arrayval = 0;		// for testing!!
	goalSize = (symbol->arrayval==0 && dwLastDataSize!=0) ? dwLastDataSize : (symbol->arrayval * symbol->datatype->size);

	len = strlen(szInstanceName);
	i=0;

	if (last)
	{
		while (last->pNext && goalSize >= symbol->datatype->size)
		{
			goalSize -= symbol->datatype->size;
			last = last->pNext;
			i++;
		}
		goalSize -= symbol->datatype->size;
	}

	while (goalSize >= symbol->datatype->size)
	{
		if (last)
		{
			if ((last->pNext = new DAComponent<DataViewer>) == 0)
				break;
			last = last->pNext;
			i++;
		}
		else
		{
			if ((pViewerList = last = new DAComponent<DataViewer>) == 0)
				break;
		}
		goalSize -= symbol->datatype->size;

		strcpy(last->szInstanceName, szInstanceName);
		last->szInstanceName[len] = '[';
		_itoa(i, last->szInstanceName+len+1 , 10);
		strcat(last->szInstanceName, "]");
		_GetSymTypeName(symbol->datatype, symbol->modifiers, last->szClassName);
		last->symbol = symbol->datatype;
		last->offset = last->symbol->size * i;
		last->pParentViewer = this;
		last->set_spelling_list(stringSet);
		last->symModifiers = symbol->modifiers | symModifiers;
		last->userModifiers = userModifiers;
		if (GetType(symbol->datatype)->kind != POINTERSYM)
			last->init();
	}

Done:
	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::initRecord (void)
{
	DataViewer *last;
	SYMBOL sym = symbol->datatype;

	if (sym)
	{
		pViewerList = last = new DAComponent<DataViewer>;
		if (last)
		while (1)
		{
			_GetSymTypeName(sym->datatype, sym->modifiers, last->szClassName);
			if (sym->name)
				strcpy(last->szInstanceName, sym->name);
			else
				strcpy(last->szInstanceName, last->szClassName);
			last->symbol = sym;
			last->offset = last->symbol->offset;
			last->pParentViewer = this;
			last->set_spelling_list(stringSet);
			last->symModifiers = symbol->modifiers | symModifiers;
			last->userModifiers = userModifiers;
			if (GetType(sym)->kind != POINTERSYM)
				last->init();

			if ((sym = sym->link) == 0)
				break;
			if ((last->pNext = new DAComponent<DataViewer>) == 0)
				break;
			last = last->pNext;
		}
	}

	return 1;
}
//--------------------------------------------------------------------------
// skip the type definition to get to the real thing
//
BOOL32 DataViewer::initType (void)
{
	SYMBOL saved = symbol;

	if (symbol->datatype)
	{
		symbol = symbol->datatype;
		init();
		symbol = saved;
	}

	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::initPointer (void)
{
	//
	// don't follow this chain until user wants this data
	// otherwise you might fall into an infinite recursion
	// Luke! It's a trap!
	//

	if (symbol->datatype)
	{
		if ((pViewerList = new DAComponent<DataViewer>) != 0)
		{
			pViewerList->szInstanceName[0] = '*';
			strcpy(pViewerList->szInstanceName+1, szInstanceName);
			_GetSymTypeName(symbol->datatype, 0, pViewerList->szClassName);
			pViewerList->symbol = symbol->datatype;
			pViewerList->pParentViewer = this;
			pViewerList->set_spelling_list(stringSet);
			pViewerList->userModifiers = userModifiers;
			pViewerList->symModifiers = symbol->modifiers;

			pViewerList->init();
		}
	}
	
	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::init (void)

{
	// set up viewer structure matching the symbol structure

	switch (symbol->kind)
	{
	case ARRAYSYM:
		initArray();
		break;

	case RECORDSYM:
		initRecord();
		break;

	case VARSYM:
	case TYPESYM:
		initType();
		break;

	case POINTERSYM:
		initPointer();
		break;
	}

	return 1;
}
//----------------------------------------------------------------------------
//
BOOL DataViewer::UpdateListView (void)
{
#if !defined(_XBOX)
//	int iItemToEdit;
//	iItemToEdit = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
//	ListView_DeleteAllItems(hListView);
	LV_ITEM lvi;
	char buffer[256];
	char *ptr;
	int numItems = ListView_GetItemCount(hListView); 

	if (bMenuItemAdded==0 && hMainWindow!=0 && GetType(symbol)->kind==ARRAYSYM && GetType(symbol)->arrayval==0 && GetType(symbol)->datatype!=0)
	{
		HMENU hMenu;
		if ((hMenu = GetSystemMenu(hMainWindow, 0)) != 0)
		{
			MENUITEMINFO minfo;

			memset(&minfo, 0, sizeof(minfo));
			minfo.cbSize = sizeof(minfo);
			minfo.fMask = MIIM_ID | MIIM_TYPE;
			minfo.fType = MFT_STRING;
			minfo.wID = IDM_ADD_ELEMENT;
			minfo.dwTypeData = const_cast<char *>("Add Element");
			minfo.cch = 11;		// strlen(ptr);
					
			InsertMenuItem(hMenu, 0x7FFE, 1, &minfo);
			bMenuItemAdded = 1;
		}
	}

	memset(&lvi, 0, sizeof(lvi));
	lvi.mask = LVIF_TEXT | LVIF_STATE;

	// set title
	if (szClassName[0])
		wsprintf(buffer, "%s  (%s)", szInstanceName, szClassName);
	else
		strcpy(buffer, szInstanceName);
	while ((ptr = strchr(buffer, 1)) != 0)
		strcpy(ptr, ptr+1);
	SetWindowText(hMainWindow, buffer);

	if (pViewerList == 0)
	{
		if (numItems <= 0)
		{
			lvi.pszText     = szInstanceName;

			ListView_InsertItem(hListView, &lvi);
		}
		ListView_SetItemText(hListView, 0, 1, szDisplayName);
	}
	else
	{
		DataViewer * tmp = pViewerList;
		DWORD i=0;

		while (tmp)
		{
			ListView_SetTextColor(hListView, 0x000000 );

			tmp->UpdateView();

			strcpy(buffer, tmp->szInstanceName);
			while ((ptr = strchr(buffer, 1)) != 0)
				strcpy(ptr, ptr+1);

			if (numItems <= int(i))
			{
				lvi.iItem       = i;
				lvi.pszText     = buffer;

				ListView_InsertItem(hListView, &lvi);
				numItems++;
			}
			else
				ListView_SetItemText(hListView, i, 0, buffer);
			ListView_SetItemText(hListView, i, 1, tmp->szDisplayName);

			if( GetType(tmp->symbol)->kind == ARRAYSYM && tmp->symbol->datatype->modifiers & MODIFIER_FILENAME )
			{
				strcat( buffer, " [fn] " );
				ListView_SetItemText(hListView, i, 0, buffer);
			}

			i++;
			tmp = tmp->pNext;
		}
	}

//	if (iItemToEdit >= 0)
//	{
//		ListView_SetItemState(hListView, iItemToEdit, 3, 0xF);
//		ListView_EnsureVisible(hListView, iItemToEdit, 0);
//		ListView_EnsureVisible(hListView, iItemToEdit+4, 0);
//	}

#endif
	return 1;
}
//----------------------------------------------------------------------------
//
BOOL DataViewer::SetListViewHeader (void)
{
#if !defined(_XBOX)
	LV_COLUMN lvc;
	char buffer[64];
	
	// Initialize the LV_COLUMN structure.

	lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt     = LVCFMT_LEFT;
	lvc.cx      = 75;

	// Add the columns.

	lvc.iSubItem = 0;		// which subitem to display
	LoadString(hInstance, IDS_NAME, buffer, 64);
	lvc.pszText  = buffer;
	lvc.cx = (3 * (ListView_GetStringWidth(hListView, buffer) + 0)) / 2;

	if (-1 == ListView_InsertColumn(hListView, 0, &lvc))
		return FALSE;

	lvc.iSubItem = 1;
	LoadString(hInstance, IDS_VALUE, buffer, 64);
	lvc.pszText = buffer;
	lvc.cx = ListView_GetStringWidth(hListView, buffer) + 0;
	if (-1 == ListView_InsertColumn(hListView, 1, &lvc))
		return FALSE;

	ListView_SetColumnWidth(hListView, 0, (rect.right-rect.left)/2);
#endif
	return TRUE;
}
//----------------------------------------------------------------------------
//
LONG CALLBACK DataViewer::EditControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
#if !defined(_XBOX)
	DataViewer *pParent = (DataViewer *) GetWindowLong(GetParent(hwnd), DWL_USER);

	switch (message)
	{
		case WM_CHAR:
			switch (LOWORD(wParam))
			{
				case 13:
					PostMessage(pParent->hMainWindow, WM_COMMAND, IDOK, (LONG)hwnd);	// send message to parent
					return 0;
				case 27:
					PostMessage(pParent->hMainWindow, WM_COMMAND, IDCANCEL, (LONG)hwnd);	// send message to parent
					return 0;

				default:
				{
					char str[128];
					sprintf(str,"Key = %d\n", LOWORD(wParam) );
					OutputDebugString(str);
					break;
				}
			}
			break;
	}

#ifdef STRICT
	return CallWindowProc((WNDPROC)pParent->lpfnOldEditProcedure, hwnd, message, wParam, lParam);
#else
	return CallWindowProc((FARPROC)pParent->lpfnOldEditProcedure, hwnd, message, wParam, lParam);
#endif

#else // #if !defined(_XBOX)
	return 0;
#endif
}
//--------------------------------------------------------------------------
//
BOOL DataViewer::MainDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result = 0;

#if !defined(_XBOX)

	switch (message)
	{
	case WM_INITDIALOG:
		hMainWindow = hwnd;
		switch (GetType(symbol)->kind)
		{
		case ENUMSYM:
			if (isReadOnly() == 0)
			{
				hCombo = GetDlgItem(hwnd, IDC_COMBO1);
				ShowWindow(hCombo, SW_SHOW);
				InitCombo();
				break;
			}

			// fall through intentional

		case BASICTYPE:
			hEdit = GetDlgItem(hwnd, IDC_EDIT1);
			ShowWindow(hEdit, SW_SHOW);
			InitEdit();
			break;
		
		case ARRAYSYM:
			if (GetType(symbol)->datatype && IsStringBaseType(GetType(symbol)->datatype, 0))
			{
				hEdit = GetDlgItem(hwnd, IDC_EDIT1);
				ShowWindow(hEdit, SW_SHOW);
				InitEdit();
				break;
			}

			// fall through intentional

		default:
			hListView = GetDlgItem(hwnd, IDC_LIST1);
			ShowWindow(hListView, SW_SHOW);
			ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
			SetListViewHeader();
			break;
		}
  		MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top , FALSE);
		result=0;	// don't set focus
		break;	// end WM_INITDIALOG case


	case WM_PAINT:
		if (bChildControl)
		{
			if (bPaintCount++)
				SetFocus(hwnd);
		}
		break;

	case WM_MOVE:
		GetWindowRect(hwnd, &rect);
		break;

	case WM_SIZE:
		{
			WORD wWidth, wHeight;

			wWidth = LOWORD(lParam);
			wHeight = HIWORD(lParam);
			GetWindowRect(hwnd, &rect);
			if (hListView)
			{
				MoveWindow (hListView, 0, 0, wWidth, wHeight , TRUE);
				S32 iWidth = ListView_GetColumnWidth(hListView, 0);
				ListView_SetColumnWidth(hListView, 1, wWidth - iWidth - GetSystemMetrics(SM_CXVSCROLL));
			}
			else
			if (hCombo)
			{
				if (pParentViewer)
				{
					RECT rect;

					SendMessage(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LONG) &rect); 
					MoveWindow(hCombo, 0, 0, wWidth, rect.bottom - rect.top, TRUE);
				}
				else
				{
					MoveWindow(hCombo, 0, 0, wWidth, wHeight, TRUE);
				}
			}
			else
			if (hEdit)
				MoveWindow(hEdit, 0, 0, wWidth, wHeight , TRUE);
		}
		break;

	case WM_SYSCOMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_ADD_ELEMENT:
			onAddRequested();
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (isReadOnly()==false && bWinDataChanged)
			{
				char buffer[256];

				buffer[0] = buffer[255] = 0;
				GetWindowText((HWND)lParam, buffer, sizeof(buffer)-1);
				if (hCombo)
				{
					SendMessage(hCombo, CB_GETLBTEXT, SendMessage(hCombo, CB_GETCURSEL, 0, 0), (LPARAM)buffer);
				}
			
				// if this is a variable sized string (array of char)
				SYMBOL sym = GetType(symbol);
				if (sym->size == 0 && sym->kind == ARRAYSYM && sym->datatype && IsStringBaseType(sym->datatype, 0))
				{
					int len = _mbstrlen(buffer) * sym->datatype->size;	// amount of bytes for string
					len -= dwLastDataSize;
					if (len > 0)
					addToFile(len);		// add this many bytes to the file
				}

				if (SetSymValue(symbol, pLastData, buffer, symbol->size ? symbol->size : dwLastDataSize))
				{
					WriteNewData();
				}
			}

			// fall through intentional

		case IDCANCEL:
			bWinDataChanged=0;
			if (pParentViewer)
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case IDC_EDIT1:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				bWinDataChanged=1;
				break;
			case EN_SETFOCUS:
				if (isReadOnly())
					SendMessage(hEdit, EM_SETSEL, -1, -1);
				break;
			case EN_KILLFOCUS:
				PostMessage(hwnd, WM_COMMAND, IDOK, lParam);	// send message to parent
				break;
			}
			break;

		case IDC_COMBO1:
			switch (HIWORD(wParam))
			{
			case CBN_SELENDOK:
				bWinDataChanged=1;
				break;
		
//			case CBN_SELENDCANCEL:
//				bWinDataChanged=0;
				//
				// fall through intentional !
				//
			case CBN_KILLFOCUS:
				PostMessage(hwnd, WM_COMMAND, IDOK, lParam);	// send message to parent
				break;
			}
			break;

		}
		break;

	case WM_SETFOCUS:
		if (bVisible)
		{	
			if (hListView)
				SetFocus(hListView);
			else
			if (hEdit)
				SetFocus(hEdit);
			else
			if (hCombo)
				SetFocus(hCombo);
		}
		result = 1;
		break;


	case WM_NOTIFY:
		{
			NMHDR * notify = (NMHDR *) lParam;
			int i;

			switch (notify->code)
			{
			case NM_SETFOCUS:
//				UpdateView();	// can't do this because it flushes changes before the IDOK message is processed
				break;

			case NM_RCLICK:
				if (notify->hwndFrom == hListView)
					if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) >= 0)
						JumpToChild(i);
				break;

			case NM_RETURN:
			case NM_DBLCLK:
				if (notify->hwndFrom == hListView)
					if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) >= 0)
						ViewChild(i);
				break;

			case LVN_KEYDOWN:
				if (((LV_KEYDOWN *)notify)->wVKey == VK_ESCAPE)
				{
					if (pParentViewer)
						PostMessage(pParentViewer->hMainWindow, USR_CLOSE, 0, (LPARAM) hwnd);
					else
						PostMessage(hwnd, WM_CLOSE, 0, 0);
				}
				if (((LV_KEYDOWN *)notify)->wVKey == VK_RIGHT)
				{
					if (notify->hwndFrom == hListView)
						if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) >= 0)
							ModifyChild( i, 1 );
				}
				if (((LV_KEYDOWN *)notify)->wVKey == VK_LEFT)
				{
					if (notify->hwndFrom == hListView)
						if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) >= 0)
							ModifyChild( i, -1 );
				}
				if (((LV_KEYDOWN *)notify)->wVKey == VK_F2)
				{
					if (notify->hwndFrom == hListView)
						if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) >= 0)
							ViewChild(i,true);
				}
				if (((LV_KEYDOWN *)notify)->wVKey == VK_DELETE)
				{
					if (notify->hwndFrom == hListView)
						if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) >= 0)
							ClearChild(i);
				}
				break;
			}
		}
		break;

	case USR_CLOSE:		// child window needs to close
		SetFocus(hwnd);
		PostMessage((HWND)lParam, WM_CLOSE, 0, 0);
		break;

	case WM_DTV_SET_DATA_PATH:
	{
		strcpy( szDataPath, (const char*)wParam );
		break;
	}

	case WM_CLOSE:
		if (pParentViewer)
			DestroyWindow(hwnd);
		else
			set_display_state(0);
		result = 1;
		break;

	case WM_DESTROY:
		if (hMainWindow==hwnd)
		{
			hEdit = hCombo = hListView = 0;
			bVisible=0;
			hMainWindow=0;
			bPaintCount=0;
			bWinDataChanged=0;
		}
		break;


	} // end switch (message)
#endif

	return result;
}
//--------------------------------------------------------------------------
//
BOOL CALLBACK DataViewer::StaticDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
#if !defined(_XBOX)
	if (message == WM_INITDIALOG)
		SetWindowLong(hwnd, DWL_USER, lParam);

	DataViewer *viewer = (DataViewer *) GetWindowLong(hwnd, DWL_USER);

	return viewer->MainDlgProc(hwnd, message, wParam, lParam);
#else
	return 0;
#endif
}
//--------------------------------------------------------------------------
//
const char *DataViewer::Dereference (SYMBOL sym, const char *pData)
{
	switch (sym->kind)
	{
	case VARSYM:
	case TYPESYM:
		if (sym->datatype == 0)
			return pData;
		return Dereference(sym->datatype, pData);
	
	case POINTERSYM:
		if (sym->datatype == 0)
			return 0;
		if (IsBadReadPtr(pData, sym->size))
			return 0;
		return *((char **)pData);
	}

	return pData;
}
		
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::UpdateData (const char *pData, U32 dwSize)
{
	DataViewer *tmp = pViewerList;

	pLastData = (char *) pData;
	dwLastDataSize = dwSize;

	//
	// if I am an array and have 0 official elements, re-init myself
	//
	if (GetType(symbol)->kind==ARRAYSYM && GetType(symbol)->arrayval == 0)
	{
		init();
		tmp = pViewerList;
	}
	
	if (tmp)
	{
		const char *_pData = Dereference(symbol, pData);
		char buffer[MAX_NAME_LENGTH];
		int len=0, childlen;

		while (tmp)
		{
			// if I am a record and we are on the last member, give variable data size
			tmp->UpdateData(_pData + tmp->offset, (GetType(symbol)->kind != RECORDSYM || tmp->pNext!=0 || tmp->symbol->size!=0) ? tmp->symbol->size : (dwSize - tmp->offset));
			// concat child result to local buffer
			if (len < MAX_NAME_LENGTH)
			{
				childlen = strlen(tmp->szDisplayName);
				if (len+childlen >= MAX_NAME_LENGTH-5-4)
				{
					if (len)
					{
						buffer[len] = ',';
						len++;
					}
					strcpy(buffer+len, szDefaultValue);
					len = MAX_NAME_LENGTH+1;
				}
				else
				{
					if (len)
					{
						buffer[len] = ',';
						len++;
					}
					strcpy(buffer+len, tmp->szDisplayName);
					len += childlen;
				}
			}

			tmp = tmp->pNext;
		}

		szDisplayName[0] = 0;
		GetSymValue(symbol, symModifiers|userModifiers, pData, szDisplayName, dwLastDataSize, stringSet);

		//
		// if we are an array of char, don't print the array
		//

		if (GetType(symbol)->kind != ARRAYSYM || IsStringBaseType(GetType(symbol)->datatype, 0)==0)
		{
			len = strlen(szDisplayName);
			childlen = strlen(buffer);
			if (len + childlen < MAX_NAME_LENGTH - 5)
			{
				strcpy(szDisplayName+len, "  {");
				strcpy(szDisplayName+len+3, buffer);
				strcpy(szDisplayName+len+3+childlen, "}");
			}
		}
	}
	else
		GetSymValue(symbol, symModifiers|userModifiers, pData, szDisplayName, dwLastDataSize, stringSet);

	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::SetDataDirectory(const char *pDirName)
{
	strncpy( szDataPath, pDirName, MAX_PATH );
	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::SubstituteInstanceName (const char *pName)
{
	DataViewer *tmp=pViewerList;
	char buffer[MAX_NAME_LENGTH];
	char *ptr, *ptr2;

	memset(buffer, 0, sizeof(buffer));
	if ((ptr = strchr(szInstanceName, 1)) != 0)
	{
		int len;

		len = ptr - szInstanceName + 1;
		memcpy(buffer, szInstanceName, len);

		if ((ptr2 = strchr(ptr+1, 1)) != 0)
		{
			int len2, len3, len4;

			len2 = strlen(ptr2) + 1;
			len3 = sizeof(buffer) - len - len2;
			len4 = strlen(pName);
			len4 = __min(len4, len3);
			if (len4 > 0)
				memcpy(buffer+len, pName, len4);
			memcpy(buffer+len+len4, ptr2, len2);
		}

		strcpy(szInstanceName, buffer);
	}

	while (tmp)
	{
		tmp->SubstituteInstanceName(pName);
		tmp = tmp->pNext;
	}
	
	return 1;
}
//--------------------------------------------------------------------------
//
int DataViewer::GetBitField32  (const char *pData, int bit_offset, int bit_count)
{
	/*
	__int64 result;

	result = *((__int64 *)pData);
	result <<= (64 - bit_offset - bit_count);
	result >>= (64 - bit_count);

	return (int) result;
	*/

	int result = 64 - bit_offset - bit_count;

	if (result > 31)
	{
		result = *((int *)pData) << (result - 32);
		result >>= (32 - bit_count);
	}
	else
	__asm 
	{
		mov edx, DWORD PTR [pData]
		mov ecx, [result]
		mov eax, DWORD PTR [edx+4]
		mov edx, DWORD PTR [edx]
		shld eax, edx, cl
		add ecx, [bit_offset]
		sar eax, cl
		mov [result],eax
	}

	return result;
}
//--------------------------------------------------------------------------
//
int DataViewer::GetBitFieldU32  (const char *pData, int bit_offset, int bit_count)
{
	/*
	unsigned __int64 result;

	result = *((__int64 *)pData);
	result <<= (64 - bit_offset - bit_count);
	result >>= (64 - bit_count);

	return (int) result;
	*/

	U32 result = 64 - bit_offset - bit_count;

	if (result > 31)
	{
		result = *((U32 *)pData) << (result - 32);
		result >>= (32 - bit_count);
	}
	else
	__asm 
	{
		mov edx, DWORD PTR [pData]
		mov ecx, [result]
		mov eax, DWORD PTR [edx+4]
		mov edx, DWORD PTR [edx]
		shld eax, edx, cl
		add ecx, [bit_offset]
		shr eax, cl
		mov [result],eax
	}

	return result;
}
//--------------------------------------------------------------------------
//
__int64 DataViewer::GetBitField64  (const char *pData, int bit_offset, int bit_count)
{
	__int64 low;
	U32 shift;

	low  = ((__int64 *)pData)[0];

	shift = 128 - bit_offset - bit_count;

	if (shift > 63)
	{
		low <<= (shift - 64);
		low >>= (shift - 64 + bit_offset);
		return low;
	}
	else
	{
		__int64 high = ((__int64 *)pData)[1];

		high <<= shift;
		high |= (((unsigned __int64)low) >> (64 - shift));
		high >>= (shift + bit_offset - 64);
		return high;
	}

}
//--------------------------------------------------------------------------
//
unsigned __int64 DataViewer::GetBitFieldU64  (const char *pData, int bit_offset, int bit_count)
{
	unsigned __int64 low;
	U32 shift;

	low  = ((unsigned __int64 *)pData)[0];

	shift = 128 - bit_offset - bit_count;

	if (shift > 63)
	{
		low <<= (shift - 64);
		low >>= (shift - 64 + bit_offset);
		return low;
	}
	else
	{
		unsigned __int64 high = ((unsigned __int64 *)pData)[1];

		high <<= shift;
		high |= (((unsigned __int64)low) >> (64 - shift));
		high >>= (shift + bit_offset - 64);
		return high;
	}
}
//--------------------------------------------------------------------------
//
int DataViewer::SetBitField32 (const char *pData, int bit_offset, int bit_count, U32 value)
{
	unsigned result;

	value <<= (32 - bit_count);
	value >>= (32 - bit_count);		// get rid of extra bits
	
	if (bit_offset + bit_count <= 32)
	{
		// create a bit mask to erase old value
		result = 0xFFFFFFFF >> (32 - bit_count);
		result <<= bit_offset;

		result = (*((int *)pData) & (~result)) | (value << bit_offset);
	}
	else
	__asm 
	{
		// note: assumes that compiler saves non-volatile registers

		xor eax,eax
		mov ecx, 32
		lea edx,[eax-1]
		sub ecx, [bit_count]
		shr edx, cl

		mov ecx, [bit_offset]
		shld eax, edx, cl
		shl edx, cl
		not eax
		not edx

		// eax : edx is now our mask

		xor esi,esi
		mov ebx, [pData]
		mov edi, [value]

		// esi : edi  == value

		and eax, DWORD PTR [ebx+4]
		shld esi, edi, cl
		and edx, DWORD PTR [ebx]
		shl  edi, cl

		or  eax, esi
		or  edx, edi

		// write the result to memory (return value is not enough)
		
		mov DWORD PTR [ebx+4], eax
		mov DWORD PTR [ebx], edx
		mov [result], edx
	}

	return result;
}
//--------------------------------------------------------------------------
//
__int64 DataViewer::SetBitField64 (const char *pData, int bit_offset, int bit_count, unsigned __int64 value)
{
	unsigned __int64 result;

	value <<= (64 - bit_count);
	value >>= (64 - bit_count);		// get rid of extra bits

	if (bit_offset + bit_count <= 64)
	{
		// create a bit mask to erase old value
		result = 0xFFFFFFFFFFFFFFFF >> (64 - bit_count);
		result <<= bit_offset;

		result = (*((__int64 *)pData) & (~result)) | (value << bit_offset);
	}
	else
	{
		unsigned __int64 high;
		unsigned __int64 *pResult = (unsigned __int64 *) pData;

		result = 0xFFFFFFFFFFFFFFFF >> (64 - bit_count);
		high = (result >> (64 - bit_offset));
		result <<= bit_offset;

//		result = (*((int *)pData) & (~result)) | (value << bit_offset);

		result = (~result) & pResult[0];
		high   = (~high) & pResult[1];

		result |= (value << bit_offset);
		high |=   (value >> (64 - bit_offset));

		pResult[0] = result;
		pResult[1] = high;
	}
		
	return result;
}		
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::CopySymValue (SYMBOL _src, U32 srcModifiers, const char *srcData, 
								 SYMBOL _dst, U32 dstModifiers, char *dstData)
{
	int bit_offset, bit_count;
	int bit_offset2, bit_count2;
	BOOL32 result;

	bit_offset = _src->bit_offset;
	bit_count  = _src->bit_count;

	bit_offset2 = _dst->bit_offset;
	bit_count2  = _dst->bit_count;

	srcModifiers |= _src->modifiers;

	SYMBOL src = GetType(_src);
	SYMBOL dst = GetType(_dst);

	result = (dst->size == src->size);

	switch (src->kind)
	{
	case BASICTYPE:
		switch (src->basicdt)
		{
		case REAL:		// double or float
		case STRINGTYPE:
			memcpy(dstData, srcData, __min(dst->size, src->size));
			break;

		default:	// integer case
			if (bit_count)
			{
				switch (src->size)
				{
				case 1:
				case 2:
				case 4:
					if ((srcModifiers & MODIFIER_UNSIGNED) == 0)
						*((int *)dstData) = SetBitField32(dstData, bit_offset2, bit_count2, GetBitField32(srcData, bit_offset, bit_count));
					else
						*((int *)dstData) = SetBitField32(dstData, bit_offset2, bit_count2, GetBitFieldU32(srcData, bit_offset, bit_count));
					break;
				case 8:
					if ((srcModifiers & MODIFIER_UNSIGNED) == 0)
						*((__int64 *)dstData) = SetBitField64(dstData, bit_offset2, bit_count2, GetBitField64(srcData, bit_offset, bit_count));
					else
						*((__int64 *)dstData) = SetBitField64(dstData, bit_offset2, bit_count2, GetBitFieldU64(srcData, bit_offset, bit_count));
					break;
				}
			}
			else
				memcpy(dstData, srcData, __min(dst->size, src->size));
			break;
		} // end switch
		break;  // end case BASICTYPE

	case ENUMSYM:
		{
			char value[80];

			if (GetSymValue(_src, srcModifiers, srcData, value, _src->size,NULL))
				result &= SetSymValue(_dst, dstData, value, _dst->size);
			else
			if (bit_count==0)
				memcpy(dstData, srcData, __min(dst->size, src->size));

			// note: if value was bad before, it continues to be bad.
			// do not return error in this case
			
		}
		break;  // end case ENUMSYM
	} // end switch (src->kind)

	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::IsEqual (SYMBOL pOldSymbol, SYMBOL pNewSymbol)
{
	BOOL32 result;
	pNewSymbol = GetType(pNewSymbol);
	pOldSymbol = GetType(pOldSymbol);

	result = ((pNewSymbol->size == pOldSymbol->size) && 
			  (pNewSymbol->name == pOldSymbol->name) &&
			  (pNewSymbol->kind == pOldSymbol->kind) &&
			  (pNewSymbol->bit_count == pOldSymbol->bit_count));

	if (result)
	switch (pNewSymbol->kind)
	{
	case BASICTYPE:
		break;

	case ENUMSYM:
	{
		pOldSymbol = pOldSymbol->datatype;	// this should be a CONSTSYM
		pNewSymbol = pNewSymbol->datatype;	// this should be a CONSTSYM
		while (pNewSymbol && pOldSymbol)
		{
			result = ((pNewSymbol->name == pOldSymbol->name) && 
					 (pNewSymbol->constval.intnum == pOldSymbol->constval.intnum));
			if (result==0)
				break;
			pNewSymbol = pNewSymbol->link;
			pOldSymbol = pOldSymbol->link;
		}
		if ((pNewSymbol!=0) ^ (pOldSymbol!=0))
			result=0;
	}
		break;

	case RECORDSYM:
	{
		pOldSymbol = pOldSymbol->datatype;	// this should be a VARSYM
		pNewSymbol = pNewSymbol->datatype;	// this should be a VARSYM
		while (pNewSymbol && pOldSymbol)
		{
			result = (pNewSymbol->name == pOldSymbol->name);	// check the variable name
			if (result==0 || (result = IsEqual(pOldSymbol->datatype, pNewSymbol->datatype)) == 0)
				break;

			pNewSymbol = pNewSymbol->link;
			pOldSymbol = pOldSymbol->link;
		}
		if ((pNewSymbol!=0) ^ (pOldSymbol!=0))
			result=0;
	}
		break;

	case ARRAYSYM:
	{
		if (pNewSymbol->arrayval != pOldSymbol->arrayval)
			result = 0;
		else
			result = IsEqual(pOldSymbol->datatype, pNewSymbol->datatype);
	}
	break;
	} // end of switch


	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::HasVariableSize (SYMBOL symbol)
{
	BOOL32 result = 0;
	symbol = GetType(symbol);

	switch (symbol->kind)
	{
	case BASICTYPE:
	case ENUMSYM:
		break;

	case RECORDSYM:
	{
		symbol = symbol->datatype;	// this should be a VARSYM
		if (symbol)		// can be an empty structure
		{
			ASSERT(symbol->kind == VARSYM);
			while (symbol)
			{
				if (symbol->link == 0)	// can only happen on last element of a structure
				{
					if (result || (result = HasVariableSize(symbol->datatype)) != 0)
						break;
				}

				symbol = symbol->link;
			}
		}
	}
	break;

	case ARRAYSYM:
	{
		result = (symbol->arrayval == 0);		// has variable size if array has no official elements
	}
	break;
	} // end of switch

	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::CreateView (void)
{
	int resource = (bChildControl) ? IDD_DIALOG2 : IDD_DIALOG1;

	if (hMainWindow)
		DestroyWindow(hMainWindow);

	switch (GetType(symbol)->kind)
	{
	case BASICTYPE:
	case ENUMSYM:
		break;

	case ARRAYSYM:
		if (GetType(symbol)->datatype && IsStringBaseType(GetType(symbol)->datatype, 0))
			break;

		// fall through intentional

	default:
		// would use a listview, so always use non-child style
		resource = IDD_DIALOG1;
		break;
	}

#if !defined(_XBOX)	
	if ((hMainWindow = CreateDialogParam(hInstance, MAKEINTRESOURCE(resource), hParentWindow, (DLGPROC)StaticDlgProc, (LPARAM) this)) == 0)
		return 0;
#endif

	UpdateView();
	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::UpdateView (void)
{
	if (hMainWindow)
	{
		if (hListView)
			return UpdateListView();
		else
		if (hCombo)
			return UpdateCombo();
		else
		if (hEdit)
			return UpdateEdit();
	}
	else
	if (pViewerList)
		pViewerList->UpdateView();	// would be case if we are a pointer symbol

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::ViewChild (int i, bool skipModifiers)
{
#if !defined(_XBOX)

	DataViewer *tmp = pViewerList;
	int j=i;

	while (tmp && j>0)
	{
		tmp = tmp->pNext;
		j--;
	}

	//
	// Open up FILENAME type
	//
	if (tmp)
	{
		SYMBOL s = GetType(tmp->symbol);

		if( !skipModifiers && s->kind == ARRAYSYM && tmp->symbol->datatype->modifiers & MODIFIER_FILENAME )
		{
			int fnoff = 0;
			const char* fn = DataViewer_GetFile("All Files(*.*)\0*.*\0\0", fnoff);

			if( fn )
			{
				strncpy( szReturnString, fn + fnoff, 256 );
				if( strlen(szReturnString) > (DWORD)s->arrayval )
				{
					::MessageBox( NULL, szReturnString, "Warning: String Too Long", MB_ICONWARNING | MB_OK );

					// keep most of the end of the string
					fnoff += strlen(szReturnString) - s->arrayval;
					strncpy( szReturnString, fn + fnoff, s->arrayval );
				}

				DataViewer::SetSymValue( s, tmp->pLastData, szReturnString, tmp->dwLastDataSize );
				tmp->UpdateData(tmp->pLastData, tmp->dwLastDataSize);
				WriteNewData();
				return TRUE;
			}
		}

		else if( !skipModifiers && s->kind == ARRAYSYM && s->datatype->kind == BASICTYPE && s->modifiers & MODIFIER_SPELLCHECK )
		{
			if( stringSet )
			{
				int ret = ::DialogBoxParam( hInstance, 
											MAKEINTRESOURCE(IDD_DIALOG_DVTREE), 
											hParentWindow, 
											DataViewer_TreeViewDlgProc, 
											(LPARAM)this );

				if( strlen(szReturnString) > (DWORD)s->arrayval )
				{
					::MessageBox( NULL, szReturnString, "Warning: String Too Long", MB_ICONWARNING | MB_OK );

					// keep most of the end of the string
					char tempString[256];
					int fnoff = strlen(szReturnString) - s->arrayval;
					strncpy( tempString, szReturnString + fnoff, s->arrayval );
					strcpy( szReturnString, tempString );
				}

				if( ret == IDOK )
				{
					DataViewer::SetSymValue( s, tmp->pLastData, szReturnString, tmp->dwLastDataSize );
					tmp->UpdateData(tmp->pLastData, tmp->dwLastDataSize);
					WriteNewData();
					return TRUE;
				}
			}
		}

		else if( !skipModifiers && s->kind == BASICTYPE && tmp->symbol->modifiers & MODIFIER_COLOR )
		{
			CHOOSECOLOR cc;                 // common dialog box structure 
			COLORREF    color;

			// Initialize CHOOSECOLOR 
			ZeroMemory(&cc, sizeof(CHOOSECOLOR));
			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.hwndOwner = hMainWindow;
			cc.lpCustColors = (LPDWORD)&color;
			cc.rgbResult = RGB(0,0,0);
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;

			if (ChooseColor(&cc)==TRUE) 
			{
				char szNumber[128];
				itoa( cc.rgbResult, szNumber, 10 );

				DataViewer::SetSymValue( s, tmp->pLastData, szNumber, tmp->dwLastDataSize );
				tmp->UpdateData(tmp->pLastData, tmp->dwLastDataSize);
				WriteNewData();
				return TRUE;
			}
		}
	}

	if (tmp)
	{
		if (tmp->pViewerList == 0)
		{
			tmp->init();
			tmp->UpdateData(tmp->pLastData, tmp->dwLastDataSize);
		}

		if (GetType(tmp->symbol)->kind == POINTERSYM && tmp->pViewerList)
			tmp = tmp->pViewerList;

		tmp->hParentWindow = hListView;
		tmp->bChildControl = true;
		tmp->bTopMost = bTopMost;

		tmp->rect = rect;
		j = GetSystemMetrics(SM_CYCAPTION);
		tmp->rect.left += j;
		tmp->rect.top += j;
		tmp->rect.right += j;
		tmp->rect.bottom += j;

		tmp->CreateView();

		if (tmp->hListView == 0)
		{
			tmp->set_auto_close(1);
			RECT rect;
			int iWidth;

			if (ListView_GetItemRect(hListView, i, &rect, LVIR_BOUNDS))
			{
				rect.bottom -= rect.top;
				rect.right -= rect.left;
				rect.bottom = __max(rect.bottom, tmp->iMinHeight);

				iWidth = ListView_GetColumnWidth(hListView, 0);
				rect.right -= iWidth;
				rect.left += iWidth;

				tmp->rect = rect;
				MoveWindow(tmp->hMainWindow, rect.left, rect.top, rect.right, rect.bottom, TRUE);
			}
		}

		tmp->set_display_state(1);
	}

#endif //#if !defined(_XBOX)

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::JumpToChild (int i)
{
	DataViewer *tmp = pViewerList;
	int j=i;

	while (tmp && j>0)
	{
		tmp = tmp->pNext;
		j--;
	}

	if (tmp)
	{
		if (GetType(tmp->symbol)->kind == ARRAYSYM && tmp->pViewerList)
			tmp = tmp->pViewerList;
		if(tmp && stringSet && 
			((tmp->userModifiers | tmp->symModifiers | tmp->symbol->modifiers) & MODIFIER_SPELLCHECK) && 
			(stringSet->HasString(tmp->pLastData)))
		{
			stringSet->CallBack(tmp->pLastData);
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
struct EraseSymbols : IStructEnumerator
{
	DWORD dwRefs;

	BEGIN_DACOM_MAP_INBOUND(EraseSymbols)
		DACOM_INTERFACE_ENTRY(IStructEnumerator)
	END_DACOM_MAP()

	int level;

	EraseSymbols() : level(0), dwRefs(0)
	{
	}

	void print( const char* _type, const char* _name, int _level, const char* _value = 0 )
	{
		char buf[128];

		if( _value )
			sprintf(buf, "%s %s = %s", _type, _name, _value );
		else
			sprintf(buf, "%s %s", _type, _name );
		
		for( int i = 0; i < _level; i++ )
		{
			OutputDebugString("\t");
		}

		OutputDebugString(buf);
		OutputDebugString("\n");
	}

	DEFMETHOD_(BOOL32,BeginEnumStruct) (IViewer *viewer, const char * structName, const char *instanceName)
	{
		print( structName, instanceName, level++ );
		return 1;
	}

	DEFMETHOD_(BOOL32,EndEnumStruct) (IViewer *viewer)
	{
		level--;
		return 1;
	}

	DEFMETHOD_(BOOL32,BeginEnumArray) (IViewer *viewer, const char * structName, const char *instanceName, int size)
	{
		print( structName, instanceName, level++ );
		return 1;
	}

	DEFMETHOD_(BOOL32,EndEnumArray) (IViewer *viewer)
	{
		level--;
		return 1;
	}

	DEFMETHOD_(BOOL32,EnumMember) (IViewer *viewer, const char * typeName, const char * instanceName, const char * value)
	{
		print( typeName, instanceName, level, value );

		DataViewer* dataViewer = static_cast<DataViewer*>( viewer );

		SYMBOL sym = DataViewer::GetType(dataViewer->symbol);
		char* data = dataViewer->pLastData;
		const char* valueName = "0";
		U32 symbolSize = sym->size;

		if( sym->kind == ENUMSYM && sym->datatype )
		{
			valueName = sym->datatype->name;
		}


		dataViewer->SetSymValue( sym, data, valueName, symbolSize );
		dataViewer->WriteNewData();

		return 1;
	}

	/* IDAComponent members */

	virtual GENRESULT COMAPI QueryInterface (const C8 *interface_name, void **instance)
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
	virtual U32 COMAPI AddRef (void)
	{
		dwRefs++;
		return dwRefs;
	}
	virtual U32 COMAPI Release (void)
	{
		if (dwRefs > 0)
			dwRefs--;
		if (dwRefs <= 1)
		{
			if (dwRefs == 0)
			{
 				dwRefs++;	// prevent infinite looping
				delete this;
				return 0;
			}
		}

		return dwRefs;
	}

	/* IComponentFactory members */

	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance)
	{
		*instance = 0;
		return GR_GENERIC;
	}

};

//--------------------------------------------------------------------------
//

BOOL32 DataViewer::ClearChild(int i)
{
	DataViewer *tmp = pViewerList;
	int j=i;

	// get that certain child by index (aka i)
	while (tmp && j>0)
	{
		tmp = tmp->pNext;
		j--;
	}

	// go through each value in the child and clear it to "zero"
	if( tmp )
	{
		EraseSymbols eraseSymbols;
		tmp->enumerate( &eraseSymbols );

//		char pTypeName[32];
//		if( tmp->GetSymTypeName(tmp->symbol, 0, pTypeName) )
//		{
//			bool writeValue = false;
//			char pValue[128];
//
//			if( !strcmp(pTypeName,"unsigned char ") )
//			{
//				unsigned char cVal = static_cast<BYTE>( atoi( tmp->szDisplayName ) + val );
//				itoa( cVal, pValue, 10 );
//				writeValue = true;
//			}
//
//			if( !strcmp(pTypeName,"long ") )
//			{
//				unsigned int cVal = static_cast<DWORD>( atoi( tmp->szDisplayName ) + val );
//				itoa( cVal, pValue, 10 );
//				writeValue = true;
//			}
//
//			if( !strcmp(pTypeName,"bool ") )
//			{
//				bool cVal = strcmp(tmp->szDisplayName,"true") ? false : true;
//				strcpy( pValue, cVal ? "false" : "true" );
//				writeValue = true;
//			}
//
//			if( writeValue )
//			{
//				SYMBOL sym  = DataViewer::GetType(tmp->symbol);
//				char*  data = tmp->pLastData;
//				DWORD  size = sym->size;
//
//				if( DataViewer::SetSymValue(sym, data, pValue,size) )
//				{
//					WriteNewData();
//					return TRUE;
//				}
//			}
//		}
	}

	return FALSE;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::ModifyChild( int i, int val )
{
	DataViewer *tmp = pViewerList;
	int j=i;

	while (tmp && j>0)
	{
		tmp = tmp->pNext;
		j--;
	}

	if (tmp)
	{
		char pTypeName[32];
		if( tmp->GetSymTypeName(tmp->symbol, 0, pTypeName) )
		{
			bool writeValue = false;
			char pValue[128];

			if( !strcmp(pTypeName,"unsigned char ") )
			{
				unsigned char cVal = static_cast<BYTE>( atoi( tmp->szDisplayName ) + val );
				itoa( cVal, pValue, 10 );
				writeValue = true;
			}

			if( !strcmp(pTypeName,"long ") )
			{
				unsigned int cVal = static_cast<DWORD>( atoi( tmp->szDisplayName ) + val );
				itoa( cVal, pValue, 10 );
				writeValue = true;
			}

			if( !strcmp(pTypeName,"bool ") )
			{
				bool cVal = strcmp(tmp->szDisplayName,"true") ? false : true;
				strcpy( pValue, cVal ? "false" : "true" );
				writeValue = true;
			}

			if( writeValue )
			{
				SYMBOL sym  = DataViewer::GetType(tmp->symbol);
				char*  data = tmp->pLastData;
				DWORD  size = sym->size;

				if( DataViewer::SetSymValue(sym, data, pValue,size) )
				{
					WriteNewData();
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::InitCombo (void)
{
#if !defined(_XBOX)

	SYMBOL sym = symbol->datatype;
	BOOL32 bLocalUpdated=bWinDataChanged;

	SetWindowPos(hCombo, HWND_TOP, 0,0,0,0,SWP_NOOWNERZORDER|SWP_NOSIZE);

	if (sym->kind == ENUMSYM)
		sym = sym->datatype;

	while (sym)
	{
	 	SendMessage(hCombo, CB_ADDSTRING, 0, (LONG) sym->name);
		sym = sym->link;
	}

	// find iMinHeight
	{
		RECT rect;

		GetWindowRect(hCombo, &rect);

		iMinHeight = rect.bottom - rect.top;
	}

	// hook into window procedure

	if ((lpfnOldEditProcedure = (WNDPROC) GetWindowLong(hCombo, GWL_WNDPROC)) != 0)
		SetWindowLong(hCombo, GWL_WNDPROC, (LONG) EditControlProcedure);

	// remove the border style
	if (pParentViewer)
	{
		LONG oldStyle = GetWindowLong(hMainWindow, GWL_STYLE);

		SetWindowLong(hMainWindow, GWL_STYLE, oldStyle & ~WS_BORDER);
	}
	
	
	bWinDataChanged = bLocalUpdated;

#endif
	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::UpdateCombo (void)
{
#if !defined(_XBOX)

	char buffer[256];
	char *ptr;
	int i;
	BOOL32 bLocalUpdated=bWinDataChanged;

	// set title
	if (szClassName[0])
		wsprintf(buffer, "%s  (%s)", szInstanceName, szClassName);
	else
		strcpy(buffer, szInstanceName);
	while ((ptr = strchr(buffer, 1)) != 0)
		strcpy(ptr, ptr+1);
	SetWindowText(hMainWindow, buffer);

	i = SendMessage(hCombo, CB_FINDSTRINGEXACT, -1, (LONG) szDisplayName);

	if (i >= 0)
		SendMessage(hCombo, CB_SETCURSEL, i, 0);

	bWinDataChanged = bLocalUpdated;

#endif
	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::UpdateEdit (void)
{
#if !defined(_XBOX)

	char buffer[256];
	char *ptr;
	BOOL32 bLocalUpdated=bWinDataChanged;

	// set title
	if (szClassName[0])
		wsprintf(buffer, "%s  (%s)", szInstanceName, szClassName);
	else
		strcpy(buffer, szInstanceName);
	while ((ptr = strchr(buffer, 1)) != 0)
		strcpy(ptr, ptr+1);
	SetWindowText(hMainWindow, buffer);

	//
	// if array of char[], edit only the text, not the enclosing quotes
	//

	if (GetType(symbol)->kind == ARRAYSYM && ((szDisplayName[0] == '\"') || (szDisplayName[0] == '<')))
	{
		if ((ptr = strchr(szDisplayName, '\"')) != 0)
		{
			strcpy(buffer, ptr+1);
			if ((ptr = strchr(buffer, '\"')) != 0)
				*ptr = 0;
		}

		SetWindowText(hEdit, buffer);
	}
	else
	{
		// check for spaces e.g. 77 'M'
		strcpy(buffer, szDisplayName);
		if ((ptr = strchr(buffer, ' ')) != 0)
			*ptr = 0;

		SetWindowText(hEdit, buffer);
	}

	SendMessage(hEdit, EM_SETSEL, 0, -1);
	bWinDataChanged = bLocalUpdated;

#endif
	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::InitEdit (void)
{
#if !defined(_XBOX)

	char buffer[256];
	char *ptr;
	BOOL32 bLocalUpdated=bWinDataChanged;

	// set title
	if (szClassName[0])
		wsprintf(buffer, "%s  (%s)", szInstanceName, szClassName);
	else
		strcpy(buffer, szInstanceName);
	while ((ptr = strchr(buffer, 1)) != 0)
		strcpy(ptr, ptr+1);
	SetWindowText(hMainWindow, buffer);

	//
	// if array of char[], edit only the text, not the enclosing quotes or the <Spelling> tag
	//

	if (GetType(symbol)->kind == ARRAYSYM && ((szDisplayName[0] == '\"') || (szDisplayName[0] == '<')))
	{
		if ((ptr = strchr(szDisplayName, '\"')) != 0)
		{
			strcpy(buffer, ptr+1);
			if ((ptr = strchr(buffer, '\"')) != 0)
				*ptr = 0;
		}

		SetWindowText(hEdit, buffer);
	}
	else
	{
		// check for spaces e.g. 77 'M'
		strcpy(buffer, szDisplayName);
		if ((ptr = strchr(buffer, ' ')) != 0)
			*ptr = 0;

		SetWindowText(hEdit, buffer);
	}

	if (isReadOnly())
	{
		SendMessage(hEdit, EM_SETREADONLY, 1, 0);
		SendMessage(hEdit, EM_SETSEL, -1, -1);
	}
	else
		SendMessage(hEdit, EM_SETSEL, 0, -1);

	// find iMinHeight
	{
		RECT rect;

		GetWindowRect(hEdit, &rect);

		iMinHeight = rect.bottom - rect.top;
	}

	// hook into window procedure

	if ((lpfnOldEditProcedure = (WNDPROC) GetWindowLong(hEdit, GWL_WNDPROC)) != 0)
		SetWindowLong(hEdit, GWL_WNDPROC, (LONG) EditControlProcedure);

	bWinDataChanged = bLocalUpdated;

#endif
	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::SetNewReadOnly (BOOL32 newVal)
{
#if !defined(_XBOX)

	DataViewer * tmp = pViewerList;

	if (hCombo || hEdit)
		SendMessage(hMainWindow, WM_CLOSE, 0, 0);
	
	if (newVal)
		userModifiers |= MODIFIER_READONLY;
	else
		userModifiers &= ~MODIFIER_READONLY;

	while (tmp)
	{
		tmp->SetNewReadOnly(newVal);
		tmp = tmp->pNext;
	}
#endif

	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::SetNewHexValue (BOOL32 newVal)
{
#if !defined(_XBOX)
	DataViewer * tmp = pViewerList;

	if (newVal)
		userModifiers |= MODIFIER_HEXVIEW;
	else
		userModifiers &= ~MODIFIER_HEXVIEW;

	while (tmp)
	{
		tmp->SetNewHexValue(newVal);
		tmp = tmp->pNext;
	}
#endif

	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 DataViewer::SetNewSpellCheckValue (BOOL32 newVal)
{
#if !defined(_XBOX)
	DataViewer * tmp = pViewerList;

	if (newVal)
		userModifiers |= MODIFIER_SPELLCHECK;
	else
		userModifiers &= ~MODIFIER_SPELLCHECK;

	while (tmp)
	{
		tmp->SetNewHexValue(newVal);
		tmp = tmp->pNext;
	}
#endif
	return 1;
}
//--------------------------------------------------------------------------
//
inline void DataViewer::ConvertToString8 (U8 value, char * string)
{
	U8 first = value >> 4;
	value &= 0xF;

	if (first < 10)
		*string++ = '0' + first;
	else
		*string++ = 'A' + first - 10;

	if (value < 10)
		*string++ = '0' + value;
	else
		*string++ = 'A' + value - 10;
}
//--------------------------------------------------------------------------
//
void DataViewer::ConvertToString16 (U16 value, char * string)
{
	ConvertToString8(U8(value>>8), string);
	ConvertToString8(U8(value), string+2);
}
//--------------------------------------------------------------------------
//
void DataViewer::ConvertToString32 (U32 value, char * string)
{
	ConvertToString16(U16(value>>16), string);
	ConvertToString16(U16(value), string+4);
}
//--------------------------------------------------------------------------
//
void DataViewer::ConvertToString64 (U64 value, char * string)
{
	ConvertToString32(U32(value>>32), string);
	ConvertToString32(U32(value), string+8);
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::enumerateBasic (struct IStructEnumerator *enumerator)
{
	GENRESULT result=GR_GENERIC;

	if (enumerator->EnumMember(this, szClassName, szInstanceName, szDisplayName))
		result = GR_OK;

	return result;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::enumerateArray (struct IStructEnumerator *enumerator)
{
	GENRESULT result=GR_GENERIC;
	DataViewer * tmp = pViewerList;
	
	if (enumerator->BeginEnumArray(this, szClassName, szInstanceName, GetType(symbol)->arrayval) == 0)
		goto Done;

	while (tmp)
	{
		if ((result = tmp->enumerate(enumerator)) != GR_OK)
			goto Done;
		tmp = tmp->pNext;
	}

	result=GR_GENERIC;
	if (enumerator->EndEnumArray(this) == 0)
		goto Done;
	result = GR_OK;
Done:	
	return result;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::enumerateRecord (struct IStructEnumerator *enumerator)
{
	GENRESULT result=GR_GENERIC;
	DataViewer * tmp = pViewerList;
	
	if (enumerator->BeginEnumStruct(this, szClassName, szInstanceName) == 0)
		goto Done;

	while (tmp)
	{
		if ((result = tmp->enumerate(enumerator)) != GR_OK)
			goto Done;
		tmp = tmp->pNext;
	}

	result=GR_GENERIC;
	if (enumerator->EndEnumStruct(this) == 0)
		goto Done;
	result = GR_OK;
Done:	
	return result;
}
//--------------------------------------------------------------------------
//
GENRESULT DataViewer::enumerate (struct IStructEnumerator * enumerator)
{
	GENRESULT result=GR_GENERIC;

	switch (GetType(symbol)->kind)
	{
	case ARRAYSYM:
		if (IsStringBaseType(GetType(symbol)->datatype, 0) == 0)
		{
			result = enumerateArray(enumerator);
			break;
		}
		// fall through intentional!

	case BASICTYPE:
	case ENUMSYM:
		result = enumerateBasic(enumerator);
		break;

	case RECORDSYM:
		result = enumerateRecord(enumerator);
		break;
	}

	return result;
}
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//
const char* __stdcall DataViewer_GetFile( const char* _mask, int& _offset )
{
#if !defined(_XBOX)

	char cwd[_MAX_PATH];
	_getcwd(cwd,_MAX_PATH);

	OPENFILENAME ofn;   

	static TCHAR szFile[MAX_PATH];     

	// Initialize variables     
	szFile[0] = __TEXT('\0');     

	ofn.lStructSize       = sizeof(OPENFILENAME);
	ofn.hwndOwner         = NULL;
	ofn.hInstance         = 0;
	ofn.lpstrFilter       = _mask;     
	ofn.lpstrCustomFilter = (LPTSTR) NULL;     
	ofn.nMaxCustFilter    = 0L;     
	ofn.nFilterIndex      = 1L;     
	ofn.lpstrFile         = szFile;     
	ofn.nMaxFile          = sizeof(szFile);     
	ofn.lpstrFileTitle    = NULL;     
	ofn.nMaxFileTitle     = 0;     
	ofn.lpstrInitialDir   = NULL;     
	ofn.lpstrTitle        = __TEXT("Filename");     
	ofn.nFileOffset       = 0;     
	ofn.nFileExtension    = 0;     
	ofn.lpstrDefExt       = NULL;  
	ofn.lCustData         = 0;      

	ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_LONGNAMES;

	if( GetOpenFileName(&ofn) )
	{
		_chdir( cwd );

		char szPath[MAX_PATH*3];
		strcpy( szPath, ofn.lpstrFile );
		szPath[ofn.nFileOffset] = 0;

		if( strstr(szPath,szDataPath) && !strchr(szDataPath,';') )
		{
			_offset = strlen(szDataPath) + 1;
		}
		else
			_offset = ofn.nFileOffset;

		return szFile;
	}

	_chdir( cwd );
#else
	assert(0);
#endif
	return NULL;
}
//--------------------------------------------------------------------------
//
#if !defined(_XBOX)

BOOL CALLBACK DataViewer_TreeViewDlgProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result = 0;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			HWND hTree  = ::GetDlgItem(hwnd,IDC_TREE_ARCHS);

			IStringSet* set = ((DataViewer*)lParam)->stringSet;
			const char* string = set->GetString(NULL);
			while( string )
			{
				TreeView_CreateBranchByParsing( hTree, string, "!!", (DWORD)string );
				string = set->GetString(string);
			}
			SetWindowLong(hwnd, DWL_USER, lParam);

			break;
		}
		case WM_COMMAND:
		{
			HWND hTree  = ::GetDlgItem(hwnd,IDC_TREE_ARCHS);

			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					HTREEITEM hTreeItem = TreeView_GetSelection( hTree );

					if( hTreeItem )
					{
						TVITEM item;
						memset( &item, 0, sizeof(item) );

						item.mask = TVIF_HANDLE | TVIF_PARAM;
						item.hItem = hTreeItem;

						BOOL ret = TreeView_GetItem( hTree, &item );
						if( ret )
						{
							DataViewer* dataViewer = (DataViewer*)GetWindowLong(hwnd,DWL_USER);
							dataViewer->szReturnString[0] = 0;

							if( item.lParam != 0xFFFFFFFF )
							{
								const char* data = (const char*)item.lParam;
								strcpy( dataViewer->szReturnString, data );
							}
						}
					}
					::EndDialog(hwnd,IDOK);
					break;
				}
				case IDCANCEL:
				{
					::EndDialog(hwnd,IDCANCEL);
					break;
				}
			}
			break;
		}
	}

	return result;
}
#endif

//--------------------------------------------------------------------------
//
void RegisterViewConstructor (ICOManager * DACOM);
void RegisterDocument (ICOManager * DACOM);
void RegisterStringSet (ICOManager * DACOM);

//--------------------------------------------------------------------------
//  
#if !defined(_XBOX)

void main (void)
{
}
#endif
//--------------------------------------------------------------------------
//  
static void SetDllHeapMsg (void)
{
	DWORD dwLen;
	char buffer[260];
	
	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
 
	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			break;
		}
		dwLen--;
	}

#if !defined(_XBOX)
	SetDefaultHeapMsg(buffer+dwLen);
#endif
}
//--------------------------------------------------------------------------//
//

//BOOL WINAPI DllMain (HINSTANCE hinstance, DWORD fdwReason, LPVOID lpvReserved)	// changed to STDCALL to make intel compiler happy
BOOL __stdcall DllMain (HINSTANCE hinstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			hInstance = hinstance;
			HEAP = HEAP_Acquire();
			SetDllHeapMsg();
			
			if ((DACOM = DACOM_Acquire()) != 0)
			{
				RegisterViewConstructor(DACOM);
				RegisterDocument(DACOM);
				RegisterStringSet(DACOM);
			}
		}
	}
	return 1;
}
//--------------------------------------------------------------------------//
//---------------------------END DataView.cpp-------------------------------//
//--------------------------------------------------------------------------//
