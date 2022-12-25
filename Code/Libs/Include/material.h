//
//
//

#ifndef MATERIAL_H
#define MATERIAL_H

//

#include <stdlib.h>
#include "typedefs.h"
#include "ITextureLibrary.h"

//

//
// Material flags allow optimization by omitting light contributions.
//
#define MF_WHITE						0x0001		// R == G == B for all light.
#define MF_EMITTER						0x0002		// Set if emission rgb != 0
#define MF_AMBIENT						0x0004		// Set if ambient rgb != 0
#define MF_DIFFUSE						0x0008		// Set if diffuse rgb != 0
#define MF_SPECULAR						0x0010		// Set if specular rgb != 0
#define MF_NO_DIFFUSE1_PASS				0x0080		// do not do first diffuse pass
#define MF_NO_DIFFUSE2_PASS				0x0100		// do not do second diffuse pass
#define MF_NO_EMITTER_PASS				0x0200		// do not do emissive texture pass
#define MF_NO_SPECULAR_PASS				0x0400		// do not do specular highlights
#define MF_NO_LIGHTING_PASS				0x0800		// do not use the diffuse lighting values at all.
#define MF_ENABLE_DEPTH_WRITES_ALWAYS	0x1000		// always enable depth writes in single pass cases (even when using alpha)
#define MF_ENABLE_ALPHA_BLEND_NEVER		0x2000		// never enable alpha blending in single pass cases (even when using alpha)


#define TF_F_HAS_ALPHA		(1<<31)		// if texture_flags & TF_F_HAS_ALPHA, then there is alpha

//
// The valid range for material rgb is 0 to 255.
//
typedef struct
{
	U8	r;
	U8	g;
	U8	b;
} MaterialRGB;

//

struct Material
{
	char *		name;

	U32			flags;

	U32			texture_id;
	void		*not_used_2;
	
//
// These rgb colors indicate how the material radiates the various types of 
// light.
//
	MaterialRGB	ambient;
	MaterialRGB diffuse;
	MaterialRGB specular;
	MaterialRGB emission;

// Shininess is used to determine the blending mode for specular highlight maps.
	U8			shininess;					// This is actually shininess strength/magnitude/height

// Transparency is the vertex alpha value.
	U8			transparency;
	
	U8			no_longer_used;

// Num passes this thing took last time we rendered it.
	U32			src_blend;				// D3DBLEND value
	U32			dst_blend;				// D3DBLEND value

	int			num_passes;

	// Use these macros to extract/pack the address mode into the _flags data for
	// one of the textures.
	// i.e. 
	// MAT_SET_ADDR_MODE( mat->texture_flags, 0, TC_ADDR_REPEAT )
#define MAT_SET_ADDR_MODE( tflags, u_or_v, tc_addrmode )	((tflags) |= (((tc_addrmode)&3)<<((u_or_v)<<1)))
#define MAT_GET_ADDR_MODE( tflags, u_or_v)					((((tflags) >> ((u_or_v)<<1)) & 3)+1)

	U32			texture_flags;					// wrap and addressing modes, etc.

	U32			second_diffuse_texture_id;
	U32			second_diffuse_texture_flags;	// wrap and addressing modes, etc.
	
	U32			emissive_texture_id;			// texture coords must be the same as first diffuse texture coords
	U32			emissive_texture_flags;			// wrap and addressing modes, etc.
	U8			emissive_blend;

	float		shininess_width;				// the true shininess factor

	U32			unique;

	ITL_TEXTURE_REF_ID diffuse_texture_ref;
	ITL_TEXTURE_REF_ID second_diffuse_texture_ref;
	ITL_TEXTURE_REF_ID emissive_texture_ref;

	Material( void );
	~Material( void );
	
	void initialize( void );
	void cleanup( void );
	void copy_from( const Material *source );

	void update( float dt );

	void set_ambient_and_diffuse( int r, int g, int b );
	void set_name( const char *name );

#if MATERIAL_H_INCLUDE_LOAD_CODE
#if !defined(FILESYS_UTILITY_H)
#error( "Material.h requires FileSys_Utility.h when compiling with MATERIAL_H_INCLUDE_LOAD_CODE" )
#endif

	HRESULT load_property_from_filesystem( IFileSystem *parent, const char *name, U32 num_constants, float *default_constants, float *out_constants, ITL_TEXTURE_REF_ID *out_map_ref_id, U32 *out_map_flags, float *out_map_blend );
	HRESULT load_from_filesystem_new( IFileSystem *parent );
	HRESULT load_from_filesystem_old( IFileSystem *parent );
	HRESULT load_from_filesystem( IFileSystem *parent );
#endif

	void set_texture_library( IDAComponent *dacomp );	// for use by polymesh & deform.lib only

private:
	ITextureLibrary *texturelib;

	Material & operator = (const Material &);	// no one should use or implement this; use copy_from()
};

//

inline Material::Material( void ) 
{
	initialize();
}

//

inline Material::~Material( void )
{
	cleanup();
}

//

inline void Material::initialize( void ) 
{
	name = NULL;

	flags = MF_AMBIENT | MF_DIFFUSE;

	ambient.r = ambient.g = ambient.b = 
	diffuse.r = diffuse.g = diffuse.b = 0xff;

	specular.r = specular.g = specular.b = 
	emission.r = emission.g = emission.b = 0x0;

	shininess_width = 0x00;				
	shininess = 0x00;

	transparency = 0xFF;

	src_blend = 5;	//D3DBLEND_SRCALPHA;
	dst_blend = 6;	//D3DBLEND_INVSRCALPHA;

	num_passes = -1;

	diffuse_texture_ref = ITL_INVALID_REF_ID;
	texture_id = 0;
	texture_flags = 0;					
	
	second_diffuse_texture_ref = ITL_INVALID_REF_ID;
	second_diffuse_texture_id = 0;
	second_diffuse_texture_flags = 0;	
	
	emissive_texture_ref = ITL_INVALID_REF_ID;
	emissive_texture_id = 0;			
	emissive_texture_flags = 0;			
	emissive_blend = 0xFF;

	unique = 0;

	not_used_2 = NULL;
	no_longer_used = 0;

	texturelib = NULL;
}

//

inline void Material::cleanup( void )
{
	set_name( NULL );

	if( texturelib ) {
		
		texturelib->release_texture_ref( diffuse_texture_ref );
		diffuse_texture_ref = ITL_INVALID_REF_ID;

		texturelib->release_texture_ref( emissive_texture_ref );
		emissive_texture_ref = ITL_INVALID_REF_ID;
		
		texturelib->release_texture_ref( second_diffuse_texture_ref );
		second_diffuse_texture_ref = ITL_INVALID_REF_ID;
	}

	set_texture_library( NULL );
}

//

inline void Material::set_ambient_and_diffuse(int r, int g, int b)
{
	ambient.r = diffuse.r = r;
	ambient.g = diffuse.g = g;
	ambient.b = diffuse.b = b;
}

//

inline void Material::set_name( const char *_name )
{
	delete[] name;
	name = NULL;
	if( _name ) {
		name = new char[ strlen(_name) + 1];
		strcpy( name, _name );
	}
}

//

inline void Material::copy_from( const Material *source )
{
	set_name( source->name );

	flags = source->flags;

	ambient.r = source->ambient.r;
	ambient.g = source->ambient.g;
	ambient.b = source->ambient.b;

	diffuse.r = source->diffuse.r;
	diffuse.g = source->diffuse.g;
	diffuse.b = source->diffuse.b;

	specular.r = source->specular.r;
	specular.g = source->specular.g;
	specular.b = source->specular.b;

	emission.r = source->emission.r;
	emission.g = source->emission.g;
	emission.b = source->emission.b;

	shininess_width = source->shininess_width;
	shininess = source->shininess;

	transparency = source->transparency;

	src_blend = source->src_blend;
	dst_blend = source->dst_blend;

	num_passes = source->num_passes;

	diffuse_texture_ref = source->diffuse_texture_ref;
	texture_id = source->texture_id;
	texture_flags = source->texture_flags;

	second_diffuse_texture_ref = source->second_diffuse_texture_ref;
	second_diffuse_texture_id = source->second_diffuse_texture_id;
	second_diffuse_texture_flags = source->second_diffuse_texture_flags;

	emissive_texture_ref = source->emissive_texture_ref;
	emissive_texture_id = source->emissive_texture_id;
	emissive_texture_flags = source->emissive_texture_flags;
	emissive_blend = source->emissive_blend;

	set_texture_library( source->texturelib );

	if( texturelib ) {
		texturelib->add_ref_texture_ref( diffuse_texture_ref );
		texturelib->add_ref_texture_ref( emissive_texture_ref );
		texturelib->add_ref_texture_ref( second_diffuse_texture_ref );
	}

	unique = source->unique;

	not_used_2 = source->not_used_2;
	no_longer_used = source->no_longer_used;
}

//

inline void Material::set_texture_library( IDAComponent *dacomp )
{
	if( texturelib != NULL )
	{
	//	texturelib->Release();		// already released
		texturelib = NULL;
	}

	if( dacomp ) 
	{
		if (dacomp->QueryInterface( IID_ITextureLibrary, (void**) &texturelib ) == GR_OK)
			dacomp->Release();		// release the reference early
	}
}

//

inline void Material::update( float dt )
{
	ITL_TEXTUREFRAME_IRP frame;
	
	if( SUCCEEDED( texturelib->get_texture_ref_frame( diffuse_texture_ref, ITL_FRAME_CURRENT, &frame ) ) ) {
		texture_id = frame.rp_texture_id;
	}
	else {
		texture_id = 0;
	}

	if( SUCCEEDED( texturelib->get_texture_ref_frame( second_diffuse_texture_ref, ITL_FRAME_CURRENT, &frame ) ) ) {
		second_diffuse_texture_id = frame.rp_texture_id;
	}
	else {
		second_diffuse_texture_id = 0;
	}

	if( SUCCEEDED( texturelib->get_texture_ref_frame( emissive_texture_ref, ITL_FRAME_CURRENT, &frame ) ) ) {
		emissive_texture_id = frame.rp_texture_id;
	}
	else {
		emissive_texture_id = 0;
	}

}


//

#if MATERIAL_H_INCLUDE_LOAD_CODE

//

inline void floats_to_uchars( float *in_3_floats, float *in_3_compares, unsigned char *out_3_chars, U32 *flags, U32 bit )
{
	out_3_chars[0] = (U8)(in_3_floats[0] * 255);
	out_3_chars[1] = (U8)(in_3_floats[1] * 255);
	out_3_chars[2] = (U8)(in_3_floats[2] * 255);

	if( in_3_floats[0] != in_3_compares[0] || 
		in_3_floats[1] != in_3_compares[1] || 
		in_3_floats[2] != in_3_compares[2]  ) {
		
		*flags |= bit;
	}
}

//

inline HRESULT Material::load_property_from_filesystem( IFileSystem *parent, const char *name, U32 num_constants, float *default_constants, float *out_constants, ITL_TEXTURE_REF_ID *out_map_ref_id, U32 *out_map_flags, float *out_map_blend )
{
#define MESH_MATERIAL_MAP_MAX_NAME_LEN 256

	char map_name[MESH_MATERIAL_MAP_MAX_NAME_LEN+1];

	if( parent->SetCurrentDirectory( name ) ) {

		if( FAILED( read_chunk( parent, "Constant", num_constants * sizeof(float), (char*)out_constants ) ) ) {
			memcpy( out_constants, default_constants, num_constants * sizeof(float) );
		}

		if( out_map_ref_id && out_map_flags ) {
			
			*out_map_ref_id = ITL_INVALID_REF_ID;
			*out_map_flags = 0;
			
			if( out_map_blend ) {
				*out_map_blend = 0.0f;
			}

			if( parent->SetCurrentDirectory( "Map" ) ) {

				if( SUCCEEDED( read_string( parent, "Name", MESH_MATERIAL_MAP_MAX_NAME_LEN, map_name ) ) ) {
					
					if( FAILED( read_type( parent, "Flags", out_map_flags ) ) ) {
						*out_map_flags = 0;
					}


					ITL_TEXTURE_ID tid ;
					PixelFormat pf;

					if( SUCCEEDED( texturelib->get_texture_id( map_name, &tid ) ) ) {
						texturelib->add_ref_texture_id( tid, out_map_ref_id );
						
						if( SUCCEEDED( texturelib->get_texture_format( tid, 0, &pf ) ) ) {
							if( pf.has_alpha_channel() ) {
								*out_map_flags |= TF_F_HAS_ALPHA;
							}
						}
						
						texturelib->release_texture_id( tid );
					}

					float fps;
					if( SUCCEEDED( read_type( parent, "FPS", &fps ) ) ) {
						texturelib->set_texture_ref_frame_rate( *out_map_ref_id, fps );
					}


					if( out_map_blend ) {
						if( FAILED( read_type( parent, "Blend", out_map_blend ) ) ) {
							*out_map_blend = 1.0f;
						}
					}
				}

				parent->SetCurrentDirectory( ".." );
			}
		}

		parent->SetCurrentDirectory( ".." );

		return S_OK;
	}
	else {
		if( num_constants ) {
			memcpy( out_constants, default_constants, num_constants * sizeof(float) );
		}
	}

	return E_FAIL;
}

//

inline HRESULT Material::load_from_filesystem_new( IFileSystem *parent )
{
	float def_zero[3] = { 0.0f, 0.0f, 0.0f };	
	float def_one[3]  = { 1.0f, 1.0f, 1.0f };	
	float constants[3];
	float blend = 1.0f;
	
	load_property_from_filesystem( parent, "Ambient", 3, def_one, constants, NULL, NULL, NULL );
	floats_to_uchars( constants, def_zero, (unsigned char*)&ambient, &flags, MF_AMBIENT );

	load_property_from_filesystem( parent, "Diffuse", 3, def_one, constants, &diffuse_texture_ref, &texture_flags, NULL );
	floats_to_uchars( constants, def_zero, (unsigned char*)&diffuse, &flags, MF_DIFFUSE );

	load_property_from_filesystem( parent, "Specular", 3, def_zero, constants, NULL, NULL, NULL );
	floats_to_uchars( constants, def_zero, (unsigned char*)&specular, &flags, MF_SPECULAR );

	load_property_from_filesystem( parent, "Emission", 3, def_zero, constants, &emissive_texture_ref, &emissive_texture_flags, &blend );
	floats_to_uchars( constants, def_zero, (unsigned char*)&emission, &flags, MF_EMITTER );
	emissive_blend = (U8)(blend * 255);

	load_property_from_filesystem( parent, "Bump", 3, def_zero, constants, &second_diffuse_texture_ref, &second_diffuse_texture_flags, NULL );

	load_property_from_filesystem( parent, "Shininess", 2, def_zero, constants, NULL, NULL, NULL );
	shininess_width = constants[0] ;		// width
	shininess = (U8)(constants[1] * 255);	// shininess[1] has the strenght/height

	load_property_from_filesystem( parent, "Transparency", 1, def_one, constants, NULL, NULL, NULL );
	transparency = (U8)(constants[0] * 255);


	return S_OK;
}

//

inline HRESULT Material::load_from_filesystem_old( IFileSystem *parent )
{
	float def_zero[3] = { 0.0f, 0.0f, 0.0f };	
	float def_one[3]  = { 1.0f, 1.0f, 1.0f };	
	float constants[3];
	
	if( FAILED( read_chunk( parent, "Ambient", 3*sizeof(float), (char*)constants ) ) ) {
		memcpy( constants, def_one, 3*sizeof(float) );
	}
	floats_to_uchars( constants, def_zero, (unsigned char*)&ambient, &flags, MF_AMBIENT );


	if( FAILED( read_chunk( parent, "Diffuse", 3*sizeof(float), (char*)constants ) ) ) {
		memcpy( constants, def_one, 3*sizeof(float) );
	}
	floats_to_uchars( constants, def_zero, (unsigned char*)&diffuse, &flags, MF_DIFFUSE );


	if( FAILED( read_chunk( parent, "Specular", 3*sizeof(float), (char*)constants ) ) ) {
		memcpy( constants, def_zero, 3*sizeof(float) );
	}
	floats_to_uchars( constants, def_zero, (unsigned char*)&specular, &flags, MF_SPECULAR );

	
	if( FAILED( read_chunk( parent, "Emission", 3*sizeof(float), (char*)constants ) ) ) {
		memcpy( constants, def_zero, 3*sizeof(float) );
	}
	floats_to_uchars( constants, def_zero, (unsigned char*)&emission, &flags, MF_EMITTER );
	emissive_blend = 0xFF;


	second_diffuse_texture_ref = ITL_INVALID_REF_ID;
	second_diffuse_texture_flags = 0;

	emissive_texture_ref = ITL_INVALID_REF_ID;
	emissive_texture_flags = 0;


	if( FAILED( read_type( parent, "Shininess", &constants[0] ) ) ) {
		constants[0] = 0.0f;
	}
	shininess_width = 0.0f ;				// width
	shininess = (U8)(constants[0] * 255);	// shininess[1] has the strenght/height


	if( FAILED( read_type( parent, "Transparency", &constants[0] ) ) ) {
		constants[0] = 1.0f;
	}
	transparency = (U8)(constants[0] * 255);

	char map_name[MESH_MATERIAL_MAP_MAX_NAME_LEN+1];

	if( FAILED( read_string( parent, "Animated texture name", MESH_MATERIAL_MAP_MAX_NAME_LEN, map_name ) ) ) {
		if( FAILED( read_string( parent, "Texture name", MESH_MATERIAL_MAP_MAX_NAME_LEN, map_name ) ) ) {
			map_name[0] = 0;
		}
	}
		
	if( map_name[0] != 0 ) {
		
		ITL_TEXTURE_ID tid;
		PixelFormat pf;

		if( SUCCEEDED( texturelib->get_texture_id( map_name, &tid ) ) ) {
			texturelib->add_ref_texture_id( tid, &diffuse_texture_ref );

			if( FAILED( texturelib->get_texture_format( tid, 0, &pf ) ) ) {
				pf.init( 16, 5,6,5,0 );	// pulling this out of my ass.
			}

			texturelib->release_texture_id( tid );
		}

		float fps;
		if( SUCCEEDED( read_type( parent, "FPS", &fps ) ) ) {
			texturelib->set_texture_ref_frame_rate( diffuse_texture_ref, fps );
		}

		if( FAILED( read_type( parent, "Flags", &texture_flags ) ) ) {
			texture_flags = 0;
		}

		if( pf.has_alpha_channel() ) {
			texture_flags |= TF_F_HAS_ALPHA;
		}
	}

	return S_OK;
}

//

inline HRESULT Material::load_from_filesystem( IFileSystem *parent )
{
	HRESULT hr = E_FAIL;

	if( texturelib == NULL ) {
		return S_OK;
	}

	if( parent->SetCurrentDirectory( "Ambient" ) ) {
		parent->SetCurrentDirectory( ".." );
		hr = load_from_filesystem_new( parent );
	}
	else {
		hr = load_from_filesystem_old( parent );
	}

	update( 0.0f );

	return hr;
}

#endif // end of MATERIAL_H_INCLUDE_LOAD_CODE

#endif
