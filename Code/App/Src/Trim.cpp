//--------------------------------------------------------------------------//
//                                                                          //
//                               Trim.cpp                                   //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Trim.cpp 2     10/26/98 6:46p Rmarr $
*/			    
//--------------------------------------------------------------------------//
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <HeapObj.h>
#include <Time.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------
//  
void main (void)
{
}
//--------------------------------------------------------------------------
//  
static void SetDllHeapMsg (HINSTANCE hInstance)
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

	SetDefaultHeapMsg(buffer+dwLen);
}
//--------------------------------------------------------------------------//
//
BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	
	srand(time(NULL));
	rand();

	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			HEAP = HEAP_Acquire();
			SetDllHeapMsg(hInstance);
		}
		break;
	}

	return 1;
}

//---------------------------------------------------------------------------
//--------------------------End Trim.cpp-------------------------------------
//---------------------------------------------------------------------------
