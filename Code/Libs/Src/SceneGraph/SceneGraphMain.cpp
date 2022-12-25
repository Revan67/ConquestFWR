//SceneGraph.cpp
//
//

#include <windows.h>

#include <ISceneGraph.h>
#include "SceneGraph.h"
#include "dacom.h"
#include "da_heap_utility.h"


//--------------------------------------------------------------------------
//  
BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			ICOManager *DACOM = DACOM_Acquire();
			IComponentFactory *server1;

			// Register System aggragate factory
			if( DACOM && (server1 = new DAComponentFactory2<DAComponentAggregate<SceneGraph>, AGGDESC>(CLSID_SceneGraph)) != NULL ) {
				DACOM->RegisterComponent( server1, CLSID_SceneGraph, DACOM_NORMAL_PRIORITY );
				server1->Release();
			}
			
			break;
		}

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}
