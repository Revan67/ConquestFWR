//--------------------------------------------------------------------------//
//                                                                          //
//                              DllMain.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 2003 Warthog Texas, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Libs/Src/VideoSystem/DllMain.cpp 1     10/06/03 2:09p Tmauer $

   Audio player
*/
//--------------------------------------------------------------------------//

#include <windows.h>

HINSTANCE hInstance;
#include "DACOM.h"
#include "da_heap_utility.h"
#include "HeapObj.h"

ICOManager * DACOM;

void RegisterTheVideoSystem (ICOManager *DACOM);

//--------------------------------------------------------------------------
//  
void main (void)
{
}
//--------------------------------------------------------------------------
//  
BOOL WINAPI DllMain(HINSTANCE hinstDLL, 
                    DWORD     fdwReason,
                    LPVOID    lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			hInstance = hinstDLL;

			HEAP = HEAP_Acquire();
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);
			DACOM = DACOM_Acquire();
			if (DACOM)
			{
				RegisterTheVideoSystem(DACOM);
			}
		}
		break;
		
	case DLL_PROCESS_DETACH:
		if (DACOM != NULL)
		{
			DACOM->Release();
		}
		break;
	}
	
	return TRUE;
}