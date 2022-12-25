// ParticleSystemArchetype.cpp
//
//
//
//

#include <windows.h>

#include "ParticleSystemArchetype.h"

//

#include "Tfuncs.h"

//

inline U32 gcd( U32 m, U32 n )
{
	if( n == 0 ) {
		return m;
	}

	return gcd( n, m % n );
}

//

ParticleSystemArchetype::ParticleSystemArchetype( void ) 
{
	texturelibrary = NULL;
	texture_id = ITL_INVALID_ID;

	initialize();	// init psp
}

//

ParticleSystemArchetype::~ParticleSystemArchetype( void ) 
{
	if( texture_id != ITL_INVALID_ID ) {
		if( texturelibrary ) {
			texturelibrary->release_texture_id( texture_id );
		}
	}

	texture_id = ITL_INVALID_ID;
	texturelibrary = NULL;
}


//

ParticleSystemArchetype::ParticleSystemArchetype( const ParticleSystemArchetype &psa ) 
{
	operator=( psa );
}

//

ParticleSystemArchetype &ParticleSystemArchetype::operator=( const ParticleSystemArchetype &psa )
{
	if( texture_id != ITL_INVALID_ID ) {
		if( texturelibrary ) {
			texturelibrary->release_texture_id( texture_id );
			texture_id = ITL_INVALID_ID;
		}
	}

	texturelibrary = psa.texturelibrary;

	set_parameters( &psa.parameters );
	
	return *this;
}

//

bool ParticleSystemArchetype::load_from_filesystem( IFileSystem *filesys, ITextureLibrary *_texturelibrary )
{
	ASSERT( filesys );

	texturelibrary = _texturelibrary;

	if( FAILED( read_type( filesys, PSP_IFS_KEY, &parameters ) ) ) {
		if( !load_from_filesystem_old( filesys, texturelibrary ) ) {
			return false;
		}
	}
	else if( parameters.texture_name[0] ) {
		if( FAILED( texturelibrary->load_texture( filesys, parameters.texture_name ) ) ) {
			return false;
		}
	}

	//The bounding sphere parameter should not be read from file
	//initializing it to zero here.

	parameters.bounding_sphere_radius	=0.0f;

	if( parameters.texture_name[0] ) {

		if( texture_id != ITL_INVALID_ID ) {
			texturelibrary->release_texture_id( texture_id );
			texture_id = ITL_INVALID_ID ;
		}

		if( FAILED( texturelibrary->get_texture_id( parameters.texture_name, &texture_id ) ) ) {
			GENERAL_TRACE_1( "ParticleSystemArchetype: load_from_filesystem: unable to find particle texture\n" );
			return false;
		}
	}

	return true;
}

//

bool COMAPI ParticleSystemArchetype::initialize( void )
{
	parameters.psp_f_flags = 0;

	for( int ck=0; ck<PSP_NUM_COLOR_KEYS; ck++ ) {
		parameters.color_frames[ck].x = 1.0f;
		parameters.color_frames[ck].y = 1.0f;
		parameters.color_frames[ck].z = 1.0f;
		parameters.color_frames[ck].w = 1.0f;
	}
	
	parameters.color_key_frame_bits = 0x80000001;	// first and last frames valid
														
	parameters.texture_name[0] = 0;
	parameters.texture_fps = 0.0f;							

	parameters.src_blend = D3DBLEND_ONE;
	parameters.dst_blend = D3DBLEND_ONE;

	parameters.gravity.zero();								

	parameters.emitter_direction.set( 1.0f, 1.0f, 1.0f );						
	parameters.emitter_nozzle_size = 0.0f;
	parameters.emitter_nozzle_damp.zero();

	parameters.initial_particle_count = 0;					
	parameters.max_particle_count = 0;
	parameters.lifetime = 0.0f;
	parameters.frequency = 0.0f;

	parameters.particle_lifetime = 0.0f;
	parameters.particle_position_randomizer = 0.0f;

	parameters.particle_twist_velocity = 0.0f;

	parameters.particle_velocity = 0.0f;
	parameters.particle_velocity_randomizer = 0.0f;
														
	parameters.particle_size = 0.0f;
	parameters.particle_size_velocity = 0.0f;

	parameters.bounding_sphere_radius = 0.0f;

	return true;
}

//

bool COMAPI ParticleSystemArchetype::reset( void )
{
	if( texture_id != ITL_INVALID_ID ) {
		texturelibrary->release_texture_id( texture_id );
		texture_id = ITL_INVALID_ID ;
	}

	if( parameters.texture_name[0] ) {
		if( FAILED( texturelibrary->get_texture_id( parameters.texture_name, &texture_id ) ) ) {
			texture_id = ITL_INVALID_ID;
			GENERAL_TRACE_1( "ParticleSystemArchetype: set_parameters: unable to find particle texture\n" );
		}
	}

	return true;
}

//

bool COMAPI ParticleSystemArchetype::is_active( void )
{
	// archetypes are never active
	return false;
}


//

bool COMAPI ParticleSystemArchetype::set_parameters( const ParticleSystemParameters *new_parameters )
{
	if( texture_id != ITL_INVALID_ID ) {
		texturelibrary->release_texture_id( texture_id );
		texture_id = ITL_INVALID_ID ;
	}

	memcpy( &parameters, new_parameters, sizeof(parameters) );

	Vector	ZeroVec;

	ZeroVec.zero();
	if(ZeroVec.equal(parameters.emitter_direction, 0.001f))
	{
		parameters.emitter_direction.set(1.0f, 1.0f, 1.0f);
	}


	if( parameters.texture_name[0] ) {
		if( FAILED( texturelibrary->get_texture_id( parameters.texture_name, &texture_id ) ) ) {
			texture_id = ITL_INVALID_ID;
			GENERAL_TRACE_1( "ParticleSystemArchetype: set_parameters: unable to find particle texture\n" );
		}
	}

	return true;
}

//

bool COMAPI ParticleSystemArchetype::get_parameters( ParticleSystemParameters *out_parameters )
{
	memcpy( out_parameters, &parameters, sizeof(parameters) );
	return true;
}

//

GENRESULT COMAPI ParticleSystemArchetype::QueryInterface( const C8 *interface_name, void **out_iif )
{
	if( strcmp( interface_name, IID_IDAComponent ) == 0 ) {
		*out_iif = static_cast<IParticleSystem*>(this);
		return GR_OK;
	}
	else if( strcmp( interface_name, IID_IParticleSystem ) == 0 ) {
		*out_iif = static_cast<IParticleSystem*>(this);
		return GR_OK;
	}

	return GR_GENERIC;
}

//

U32 COMAPI ParticleSystemArchetype::AddRef( void )
{
	// Archetypes are not ref-counted here, their ref-count
	// is managed by the Optics component itself
	return 1;
}

//

U32 COMAPI ParticleSystemArchetype::Release( void )
{
	// Archetypes are not ref-counted here, their ref-count
	// is managed by the Optics component itself
	return 1;
}

//

#if !defined(PSA_LOAD_OLD_FORMAT)

bool ParticleSystemArchetype::load_from_filesystem_old( IFileSystem *, ITextureLibrary * )
{
	return false;
}

#else

#include "EventDef.h"

bool ParticleSystemArchetype::load_from_filesystem_old( IFileSystem *filesys, ITextureLibrary *texturelibrary )
{
	EventDef eventdef;
	bool loaded = false;
	int out;

	// load EventDef style effect
	//
	out = filesys->SetCurrentDirectory( "Particle Event" );
	
	memset( &eventdef, 0, sizeof(EventDef) );

	if( SUCCEEDED( read_chunk( filesys, "particle1.Def", 0, (char*)&eventdef ) ) ) {

		parameters.lifetime = eventdef.lifetime * 0.001f;
		parameters.frequency = eventdef.frequency;
		parameters.initial_particle_count = eventdef.nParticles;
		parameters.max_particle_count = eventdef.maxParticles;
		parameters.emitter_direction = eventdef.direction;

		//ensure direction isn't zerod
		Vector	ZeroVec;

		ZeroVec.zero();
		if(ZeroVec.equal(parameters.emitter_direction, 0.001f))
		{
			parameters.emitter_direction.set(1.0f, 1.0f, 1.0f);
		}

		parameters.emitter_nozzle_size = eventdef.nozzle;
		parameters.emitter_nozzle_damp = eventdef.nozzleDamp;
		
		parameters.gravity = eventdef.gravityVec;
		parameters.gravity.z += eventdef.gravity;
		parameters.gravity.scale( 1000.0f );

		parameters.particle_lifetime = eventdef.partLife * 0.001f;
		parameters.particle_position_randomizer = eventdef.randPosition;
		parameters.particle_size = eventdef.size;
		parameters.particle_size_velocity = eventdef.sizeVelocity * 1000.0f;
		parameters.particle_twist_velocity = eventdef.twistSpeed * 1000.0f;
		parameters.particle_velocity = eventdef.velocity * 1000.0f;
		parameters.particle_velocity_randomizer = eventdef.velocityRand;


		strcpy( parameters.texture_name, eventdef.textureName );
		parameters.texture_fps = eventdef.fps;

		if( eventdef.dither ) {
			parameters.psp_f_flags |=   PSP_F_RENDER_DITHER;
		}
		else {
			parameters.psp_f_flags &= ~(PSP_F_RENDER_DITHER);
		}

		if( eventdef.bFog ) {
			parameters.psp_f_flags |=   PSP_F_RENDER_FOG;
		}
		else {
			parameters.psp_f_flags &= ~(PSP_F_RENDER_FOG);
		}

		if( eventdef.bUseParticleLifetime ) {
			parameters.psp_f_flags |=   PSP_F_RENDER_PARTICLE_LIFE;
		}
		else {
			parameters.psp_f_flags &= ~(PSP_F_RENDER_PARTICLE_LIFE);
		}

		if( eventdef.bInherit ) {
			parameters.psp_f_flags |=   PSP_F_RELATIVE_VELOCITY;
		}
		else {
			parameters.psp_f_flags &= ~(PSP_F_RELATIVE_VELOCITY);
		}

		if( eventdef.bBound ) {
			parameters.psp_f_flags |=   PSP_F_RELATIVE_TRANSFORM;
		}
		else {
			parameters.psp_f_flags &= ~(PSP_F_RELATIVE_TRANSFORM);
		}

#if 0
		if( eventdef.bIgnoreOrientation ) {
			parameters.psp_f_flags |= PSP_F_IGNORE_ORIENTATION;
		}
		else {
			parameters.psp_f_flags &= ~(PSP_F_IGNORE_ORIENTATION);
		}
#endif

		if( eventdef.src_blend > 0 ) {
			parameters.src_blend = eventdef.src_blend;
		}

		if( eventdef.dst_blend > 0 ) {
			parameters.dst_blend = eventdef.dst_blend;
		}

		parameters.bounding_sphere_radius = eventdef.radius;

		if( eventdef.PrimaryKeys ) {
			
			for( U32 ckf = 0; ckf < PSP_NUM_COLOR_KEYS; ckf++ ) {
				parameters.color_frames[ckf] = eventdef.ColorKeyFrames[ckf];
			}

			// the old code (editor) did not mark the last frame as a key frame
			// the new code (editor/runtime) requires the first and last frame to
			// be marked as key frames.
			//
			parameters.color_key_frame_bits = eventdef.PrimaryKeys | 0x80000001;
		}
		else if( eventdef.colorVelocity.magnitude() != 0 ) {
			
			float df, r,g,b,a;

			if( eventdef.partLife ) {
				df = ((float)eventdef.partLife) / ((float)PSP_NUM_COLOR_KEYS);
			}
			else {
				df = ((float)eventdef.lifetime) / ((float)PSP_NUM_COLOR_KEYS);
			}

			r = eventdef.color.x;
			g = eventdef.color.y;
			b = eventdef.color.z;
			a = eventdef.alpha;

			for( U32 ck=0; ck<PSP_NUM_COLOR_KEYS; ck++ ) {
				
				parameters.color_frames[ck].x = Tclamp( 0.0f, 1.0f, r );
				parameters.color_frames[ck].y = Tclamp( 0.0f, 1.0f, g );
				parameters.color_frames[ck].z = Tclamp( 0.0f, 1.0f, b );
				parameters.color_frames[ck].w = Tclamp( 0.0f, 1.0f, a );

				r += df * 0.001f * eventdef.colorVelocity.x;
				g += df * 0.001f * eventdef.colorVelocity.y;
				b += df * 0.001f * eventdef.colorVelocity.z;
				a += df * 0.001f * eventdef.alphaDecay;
			}

			parameters.color_key_frame_bits = 0x80000001;	// first and last are key frames
		}
		else {
			for( U32 ck=0; ck<PSP_NUM_COLOR_KEYS; ck++ ) {
				parameters.color_frames[ck].x = eventdef.color.x;
				parameters.color_frames[ck].y = eventdef.color.y;
				parameters.color_frames[ck].z = eventdef.color.z;
				parameters.color_frames[ck].w = eventdef.alpha;
			}

			parameters.color_key_frame_bits = 0x80000001;	// first and last are key frames 
		}

		if( parameters.texture_name[0] ) {
			if( FAILED( texturelibrary->load_texture( filesys, parameters.texture_name ) ) ) {
				return false;
			}
		}

		loaded = true;
	}

	if( out ) {
		filesys->SetCurrentDirectory( ".." );
	}

	return loaded;
}

#endif

