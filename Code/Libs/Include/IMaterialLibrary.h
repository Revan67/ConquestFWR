// IMaterialLibrary
//
//
//

#ifndef __IMaterialLibrary_h__
#define __IMaterialLibrary_h__

//

#include "DACOM.h"
#include "IMaterial.h"

//

//

#define IID_IMaterialLibrary MAKE_IID("IMaterialLibrary",1)

//

struct IMaterialLibrary : public IDAComponent
{
	virtual GENRESULT COMAPI load_library( IFileSystem *IFS ) = 0;
	virtual GENRESULT COMAPI free_library( void ) = 0;
	virtual GENRESULT COMAPI verify_library( U32 max_num_passes, float max_detail_level ) = 0;

	virtual GENRESULT COMAPI add_material_map( const char *material_class_spec, const char *clsid ) = 0;
	virtual GENRESULT COMAPI find_material_map( const char *material_class_spec, U32 clsid_buffer_len, char *out_clsid ) = 0;
	virtual GENRESULT COMAPI remove_material_map( const char *material_class_spec ) = 0;

	virtual GENRESULT COMAPI create_material( const char *material_name, const char *material_class, IMaterial **out_material ) = 0;
	virtual GENRESULT COMAPI add_material( const char *material_name, IMaterial *material ) = 0;
	virtual GENRESULT COMAPI find_material( const char *material_name, IMaterial **out_material ) = 0;
	virtual GENRESULT COMAPI remove_material( const char *material_name ) = 0;

	virtual GENRESULT COMAPI get_material_count( U32 *out_material_count ) = 0;
	virtual GENRESULT COMAPI get_material( U32 num_material, IMaterial **out_material ) = 0;
};


#endif // EOF

