// IStateMaterial.h
//
//
//

#ifndef __IStateMaterial_h__
#define __IStateMaterial_h__

//

#include "3dmath.h"
#include "rendpipeline.h"

//

//
// IStateMaterial
//
//
#define IID_IStateMaterial MAKE_IID("IStateMaterial",1)
//
struct IStateMaterial : public IDAComponent
{
	// NOTE: the methods on this interface can be as bad as O(s) where s is the
	// NOTE: number of states of the particular type.  i.e. get_/set_render_state() can
	// NOTE: be as bad as O(num_render_states_in_material).

	//
	virtual GENRESULT COMAPI set_render_state( D3DRENDERSTATETYPE state, U32 value ) = 0;

	// if state is not found in the material, returns GR_GENERIC
	virtual GENRESULT COMAPI get_render_state( D3DRENDERSTATETYPE state, U32 *out_value ) = 0;

	//
	virtual GENRESULT COMAPI set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value ) = 0;

	// if state is not found in the material, returns GR_GENERIC
	virtual GENRESULT COMAPI get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 *out_value ) = 0;
	
	//
	virtual GENRESULT COMAPI set_texture_stage_texture( U32 stage, U32 irp_texture_id ) = 0;

	// if state is not found in the material, returns GR_GENERIC
	virtual GENRESULT COMAPI get_texture_stage_texture( U32 stage, U32 *out_irp_texture_id ) = 0;
};


#endif // EOF

