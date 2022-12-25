// IMaterialProperties
//
//
//

#ifndef __IMaterialProperties_h__
#define __IMaterialProperties_h__

//

#include "3dmath.h"
#include "ITextureLibrary.h"
#include "TextureCoord.h"

//

//
// IMaterialProperties
//
//
#define IID_IMaterialProperties MAKE_IID("IMaterialProperties",1)
//
struct IMaterialProperties : public IDAComponent
{
	// Textures
	//
	virtual GENRESULT COMAPI get_num_textures( U32 *out_num_textures ) = 0;

	// addrefs texture ref, releases old texture ref
	virtual GENRESULT COMAPI set_texture( U32 texture_num, ITL_TEXTURE_REF_ID trid ) = 0;

	// addrefs texture ref, sets out to invalid if texture_num is not valid
	virtual GENRESULT COMAPI get_texture( U32 texture_num, ITL_TEXTURE_REF_ID *out_trid ) = 0;

	virtual GENRESULT COMAPI set_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE mode ) = 0;
	virtual GENRESULT COMAPI get_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE *out_mode ) = 0;

	virtual GENRESULT COMAPI set_texture_wrap_mode( U32 texture_num, TC_WRAPMODE mode ) = 0;
	virtual GENRESULT COMAPI get_texture_wrap_mode( U32 texture_num, TC_WRAPMODE *out_mode ) = 0;

	// Constants
	//
	virtual GENRESULT COMAPI get_num_constants( U32 *out_num_constants ) = 0;

	virtual GENRESULT COMAPI set_constant( U32 constant_num, U32 num_values, float *values ) = 0;
	virtual GENRESULT COMAPI get_constant( U32 constant_num, U32 max_num_values, float *out_values ) = 0;
	
	//
	// NOTE: set_constant_length doesn't make sense as its length is defined completely by the material
	//
	virtual GENRESULT COMAPI get_constant_length( U32 constant_num, U32 *out_number_of_float_values ) = 0;

};


#endif // EOF

