// States.cpp
//
//
// Not for use yet!

//

#include <windows.h>

//

#include "DACOM.h"
#include "TSmartPointer.h"
#include "IMaterial.h"
#include "IStateMaterial.h"
#include "IMesh.h"

//
// Set the given material as the material to use on all facegroups on the
// given mesh.
//
HRESULT SetMaterialOnMesh( IMesh *mesh, IMaterial *material )
{
	U32 fgc;

	mesh->get_num_facegroups( &fgc );

	for( U32 fg = 0; fg < fgc; fg++ ) {
		mesh->lock_facegroup( fg, true );
		mesh->set_facegroup_material( material );
		mesh->unlock_facegroup();
	}

	return S_OK;	
}

//

HRESULT CreateDepthTestedMaterial( IDAComponent *System, IMaterial **out_material )
{

	DACOMDESC desc( "StateMaterial" );

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)out_material ) ) ) {
		return E_FAIL;
	}

	if( FAILED( (*out_material)->initialize( System ) ) ) {
		return E_FAIL;
	}

	COMPTR<IStateMaterial> ISM;
	if( FAILED( (*out_material)->QueryInterface( IID_IStateMaterial, (void**) &ISM ) ) ) {
		(*out_material)->Release();
		*out_material = NULL;
		return E_FAIL;
	}

	ISM->set_render_state( D3DRS_ZENABLE,		TRUE );
	ISM->set_render_state( D3DRS_ZFUNC,		D3DCMP_LESS );
	ISM->set_render_state( D3DRS_ZWRITEENABLE, TRUE );

	return S_OK;
}

//

HRESULT CreateTextureViewMaterial( IDAComponent *System, IMaterial **out_material )
{

	DACOMDESC desc( "StateMaterial" );

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)out_material ) ) ) {
		return E_FAIL;
	}

	if( FAILED( (*out_material)->initialize( System ) ) ) {
		return E_FAIL;
	}

	COMPTR<IStateMaterial> ISM;
	if( FAILED( (*out_material)->QueryInterface( IID_IStateMaterial, (void**) &ISM ) ) ) {
		(*out_material)->Release();
		*out_material = NULL;
		return E_FAIL;
	}

	ISM->set_render_state( D3DRS_ZENABLE,		TRUE );
	ISM->set_render_state( D3DRS_ZFUNC,		D3DCMP_LESS );
	ISM->set_render_state( D3DRS_ZWRITEENABLE, TRUE );

	return S_OK;
}

//

// EOF