#ifndef DLLSCRIPTMAIN_H
#define DLLSCRIPTMAIN_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DLLScriptMain.h                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Include/DLLScriptMain.h 7     6/12/00 4:57p Jasony $

   Should be included after ScriptDef.h.
   Can only be included by one .cpp in the project.
*/
//--------------------------------------------------------------------------//

#include "Resource.h"

IHeap * HEAP;
//--------------------------------------------------------------------------
//  
void main (void)
{
}
//--------------------------------------------------------------------------
//  
static void setDllHeapMsg (HINSTANCE hInstance)
{
#if 0
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

	SetDefaultHeapMsg(buffer+dwLen);
#endif
}
//--------------------------------------------------------------------------//
// returns handle to local.dll
//
inline HINSTANCE loadLocalDll (HINSTANCE hInstance)
{
#if 0
	DWORD dwLen;
	char buffer[260];
	
	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
 
	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			strcpy(buffer+dwLen, "Local.dll");
			break;
		}
		dwLen--;
	}

	return LoadLibrary(buffer);	
#else
	return hInstance;
#endif
}
//--------------------------------------------------------------------------//
//
static void setScriptData (HINSTANCE hInstance)
{
	HRSRC hRes;

	if ((hRes = FindResource(hInstance, MAKEINTRESOURCE(IDR_PARSER_1), "PARSER")) != 0)	
	{																					
		HGLOBAL hGlobal;																
																						
		if ((hGlobal = LoadResource(hInstance, hRes)) != 0)								
		{																				
			void * ptr = LockResource(hGlobal);											
																						
			if (ptr)																	
			{																			
				g_CQScriptDataDesc.preprocessSize = SizeofResource(hInstance, hRes);
				g_CQScriptDataDesc.preprocessData = ptr;
				g_CQScriptDataDesc.hLocal = loadLocalDll(hInstance);	//LoadLibrary("Scripts\\Local.dll");
//				MScript::SetScriptData(ptr, resourceSize, #DataClass, &DataInstance, sizeof(DataInstance));	
				MScript::SetScriptData(g_CQScriptDataDesc);
			}																			
			else																		
				CQERROR0("Failed to load parser resource");								
		}																				
		else																			
			CQERROR0("Failed to load parser resource");									
	}
	else
		CQERROR0("Failed to load parser resource");											
}
//--------------------------------------------------------------------------//
//
BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			HEAP = HEAP_Acquire();
			setDllHeapMsg(hInstance);
			setScriptData(hInstance);
		}
		break;
		case DLL_PROCESS_DETACH:
		{
			if (g_CQScriptDataDesc.hLocal && g_CQScriptDataDesc.hLocal!=hInstance)
			{
				FreeLibrary(g_CQScriptDataDesc.hLocal);
				g_CQScriptDataDesc.hLocal = 0;
			}
		}
		break;
	}

	return 1;
}

//--------------------------------------------------------------------------//
//------------------------------End DLLScriptMain.h-------------------------//
//--------------------------------------------------------------------------//
#endif