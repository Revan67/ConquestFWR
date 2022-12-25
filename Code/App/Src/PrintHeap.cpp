//--------------------------------------------------------------------------//
//                                                                          //
//                             PrintHeap.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "CQTrace.h"

#include <HeapObj.h>

//--------------------------------------------------------------------------//
//
void __cdecl _localprintf (const char *fmt, ...)
{
	char buffer[256];
	va_list ap;

	va_start(ap, fmt);
	wvsprintf(buffer, fmt, ap);
	va_end(ap);

	OutputDebugString(buffer);
}
//--------------------------------------------------------------------------//
//
int __stdcall _localMessageBox (HWND hWnd, U32 textID, U32 titleID, UINT uType)
{
	wchar_t buffer[256];

	wcscpy(buffer, _localLoadStringW(titleID));

	return MessageBoxW(hWnd, _localLoadStringW(textID), buffer, uType);
}
//--------------------------------------------------------------------------//
//
int __stdcall _localAnsiToWide (const char * input, wchar_t * output, U32 bufferSize)
{
		// 932 is Japanese code page
	int result = MultiByteToWideChar(CP_ACP, 0, input, -1, output, (bufferSize/sizeof(output[0])) );
	if (bufferSize>1)
		output[(bufferSize/sizeof(wchar_t))-1] = 0;
	return result;
}
//--------------------------------------------------------------------------//
//
int __stdcall _localWideToAnsi (const wchar_t * input, char * output, U32 bufferSize)
{
		// 932 is Japanese code page
	int result = WideCharToMultiByte(CP_ACP, 0, input, -1, output, bufferSize, 0, 0);
	if (bufferSize)
		output[bufferSize-1] = 0;
	return result;
}
//--------------------------------------------------------------------------//
//
const char * __cdecl _localLoadString (U32 dwID)
{
	static char buffer[256];

	buffer[0] = 0;
	LoadString(hResource, dwID, buffer, sizeof(buffer)-1);

	return buffer;
}
//--------------------------------------------------------------------------//
//
#if 0
const wchar_t * __cdecl _localLoadStringW (U32 dwID)
{
	static wchar_t buffer[256];

	buffer[0] = 0;
	if (LoadStringW(hResource, dwID, buffer, 255) == 0)		// assume it's not implemented
	{
		const char * src = _localLoadString(dwID);
		
		_localAnsiToWide(src, buffer, sizeof(buffer));
	}

	return buffer;
}
#else

LANGID userLang = 0;
LANGID findUserLang()
{
	if(!userLang)
	{
		LANGID sysDefault = GetUserDefaultLangID();
		WORD lang = PRIMARYLANGID(sysDefault);
		switch(lang)
		{
		case LANG_FRENCH:
			userLang = MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH);
			break;
		case LANG_SPANISH:
			userLang = MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH);
			break;
		case LANG_GERMAN:
			userLang = MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN);
			break;
		case LANG_ITALIAN:
			userLang = MAKELANGID(LANG_ITALIAN,SUBLANG_ITALIAN);
			break;
		case LANG_POLISH:
			userLang = MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT);
			break;
		case LANG_CHINESE:
			userLang = MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL);
			break;
		case LANG_KOREAN:
			userLang = MAKELANGID(LANG_KOREAN,SUBLANG_KOREAN);
			break;
		default:
			userLang = MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US);
			break;
		}
	}
	return userLang;
}

void SetLangID(LANGID id)
{
	userLang = id;
}

const wchar_t * __cdecl _localLoadStringW (U32 dwID)
{
	HRSRC hRes;
	static wchar_t buffer[1024];

	buffer[0] = 0;

	//
	// find address of string resource group (16 in a group)
	//
	if ((hRes = FindResourceEx(hResource,  RT_STRING, MAKEINTRESOURCE((dwID/16)+1),findUserLang())) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hResource, hRes)) != 0)
		{
			U16 *pData;

			if ((pData = (U16 *) LockResource(hGlobal)) != 0)
			{
				//
				// find the actual string within the group
				//
				int i = dwID % 16;
				U32 numChars;

				while (i-- > 0)
				{
					numChars = *pData++;
					pData += numChars;
				}
				
				numChars = *pData++;
				CQASSERT(numChars < 1024);

				memcpy(buffer, pData, numChars * sizeof(wchar_t));
				buffer[numChars] = 0;		// null terminate
			}
		}
	}

	return buffer;
}
#endif

//--------------------------------------------------------------------------//
//
static int __stdcall PrintBlock (struct IHeap * pHeap, void *allocatedBlock, U32 dwFlags, void *context)
{
	const char *pMsg = pHeap->GetBlockMessage(allocatedBlock);
	char buffer[64];
	U32 owner = pHeap->GetBlockOwner(allocatedBlock);

	if (owner == 0xFFFFFFFF)		// marked block
		return 1;

	wsprintf(buffer, "Owner=%08X", owner);

	_localprintf("%s block at %08X  Size = %4d  %s %s %s\n",
		(dwFlags & DAHEAPFLAG_ALLOCATED_BLOCK)?"Allocated":"Free     ",
		allocatedBlock,
		pHeap->GetBlockSize(allocatedBlock),
		IsBadReadPtr(pMsg, 32)?"":pMsg,
		(owner)?buffer:"",
		(dwFlags & DAHEAPFLAG_CORRUPTED_BLOCK)?"[CORRUPTED]":"[OK]");

	return 1;
}
//--------------------------------------------------------------------------//
//
int PrintHeap (IHeap *pHeap)
{
	int result;

	_localprintf("Largest Block=%d, HeapSize=%d.", pHeap->GetLargestBlock(), pHeap->GetHeapSize());

	if (pHeap->GetLargestBlock() == pHeap->GetHeapSize())
		_localprintf("   Heap is empty.\n");
	else
		_localprintf("   Heap is NOT empty.\n");

	if ((result = pHeap->EnumerateBlocks((IHEAP_ENUM_PROC) PrintBlock)) == 0)
		_localprintf("Enumerate failed!\n");
	
	return result;
}
//--------------------------------------------------------------------------//
//
static int __stdcall MarkBlock (struct IHeap * pHeap, void *allocatedBlock, U32 dwFlags, void *context)
{
	pHeap->SetBlockOwner(allocatedBlock, 0xFFFFFFFF);
	return 1;
}
//--------------------------------------------------------------------------//
//
int MarkAllocatedBlocks (IHeap *pHeap)
{
	return pHeap->EnumerateBlocks((IHEAP_ENUM_PROC) MarkBlock);
}

//--------------------------------------------------------------------------//
//------------------------------End PrintHeap.cpp---------------------------//
//--------------------------------------------------------------------------//
