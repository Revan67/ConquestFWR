// Materials.cpp
//
// This file declares/defines all of the DLL-related and shared code
// used throughout the Material-related components.
//

#include <windows.h>

//

#include "DACOM.h"
#include "da_heap_utility.h"
#include "TComponent.h"

//

#include "Materials.h"

//

// prottype
void InstallProceduralManager();
void UnInstallProceduralManager();

// 
// DLL Related code
// 

// This is for a linker bug
void main (void) {}

// DllMain
//  
// Register all of the materials.
//
BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch( fdwReason ) {

	case DLL_PROCESS_ATTACH:

		DA_HEAP_ACQUIRE_HEAP(HEAP);
		DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

		REGISTER_MATERIAL(MaterialLibrary);
		
		REGISTER_MATERIAL(DebugMaterial);
		REGISTER_MATERIAL(NullMaterial);
		REGISTER_MATERIAL(SimpleMaterial);
		REGISTER_MATERIAL(StateMaterial);
		REGISTER_MATERIAL(DcDtMaterial);
		REGISTER_MATERIAL(DcDtEcMaterial);
		REGISTER_MATERIAL(DcDtOcMaterial);
		REGISTER_MATERIAL(DcDtBtEcMaterial);
		REGISTER_MATERIAL(DcDtBtMaterial);
		REGISTER_MATERIAL(DcDtEcEtMaterial);
		REGISTER_MATERIAL(DcDtEtMaterial);
		REGISTER_MATERIAL(EcEtMaterial);
		REGISTER_MATERIAL(DcDtOcOtMaterial);

		// 24 Mar. 2000 Yito adds
		REGISTER_MATERIAL( ToonShaderMaterial				);
		REGISTER_MATERIAL( EmbossBumpMaterial				);
		REGISTER_MATERIAL( SpecularMaterial					);
		REGISTER_MATERIAL( ReflectionMaterial				);
		REGISTER_MATERIAL( SpecularGlossMaterial		);
		REGISTER_MATERIAL( ReflectionGlossMaterial	);
		REGISTER_MATERIAL( DcDtTwoMaterial					);
		REGISTER_MATERIAL( DcDtOcTwoMaterial				);
		REGISTER_MATERIAL( DcDtOcOtTwoMaterial			);
		REGISTER_MATERIAL( DcDtEcTwoMaterial				);

		REGISTER_MATERIAL( ProceduralMaterial				);

		InstallProceduralManager();

		break;

	case DLL_PROCESS_DETACH:

		UnInstallProceduralManager();

		break;
	}

	return TRUE;
}

// EOF

