// ParticleSystemInstance.cpp
//
//
//
//

//

#define WINDOWS_LEAN_AND_MEAN 1
#include <windows.h>
#include <stdio.h>

#include "typedefs.h"
#include "3dmath.h"
#include "tempstr.h"
#include "packed_argb.h"
#include "ICamera.h"
#include "ITextureLibrary.h"
#include "RPUL.h"

//

#include "Tfuncs.h"

//

#include "ParticleSystemInstance.h"
#include "render_buffers.h"

//

const float f_rand_max   = (float)(RAND_MAX);
const float f_rand_max_2 = (float)(RAND_MAX/2);

//

// returns a "random" number between -1.0 and 1.0
inline float f_rand( void )
{
	return (((float)rand()) / f_rand_max_2) - 1.0f;
}

// returns a "random" number between 0.0 and 1.0
inline float f_abs_rand( void )
{
	return (((float)rand()) / f_rand_max) ;
}

//

ParticleSystemInstance::ParticleSystemInstance( )
{
	texturelibrary = NULL;
	inst_index = INVALID_INSTANCE_INDEX;

	initialize();
}

//

ParticleSystemInstance::ParticleSystemInstance( const ParticleSystemInstance &psi )
{
	texturelibrary = psi.texturelibrary;
	inst_index = INVALID_INSTANCE_INDEX;

	initialize();
	set_parameters( &psi.parameters );
}

//

bool COMAPI ParticleSystemInstance::initialize( void )
{
	psi_f_flags = 0;
												
	particles_array_size = 0;
	particles = NULL;
	active_particles_indices = NULL;
	num_active_particles = 0;
												
	num_created_particles = 0;
					
	lru_active_index = 0;
	next_active_particle_index = 0;
	
	num_partial_particles = parameters.initial_particle_count;
	emitter_lifetime = 0.0f;
				
	texture_ref_id = ITL_INVALID_REF_ID;
	texture_frame_count = 0;

	// 'one-time' init
	//
	if( !(psi_f_flags & PSI_F_RELEASE_VB) ) {
		add_ref_vertex_buffer();
		add_ref_vector_buffer();
		psi_f_flags |= PSI_F_RELEASE_VB ;
	}

	max_active_particles = 0;



	parameters.psp_f_flags = 0;

	for( int ck=0; ck<PSP_NUM_COLOR_KEYS; ck++ ) {
		parameters.color_frames[ck].x = 1.0f;
		parameters.color_frames[ck].y = 1.0f;
		parameters.color_frames[ck].z = 1.0f;
		parameters.color_frames[ck].w = 1.0f;
	}
	
	parameters.color_key_frame_bits = 0x80000001;	// first and last are key frames 
														
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

bool COMAPI ParticleSystemInstance::reset( void ) 
{
	// 'reset-time' init
	//
	num_created_particles = 0;
	num_active_particles = 0;

	seconds_since_last_new_particle = 0;
	seconds_since_last_engine_position = 0;
	last_engine_position.zero();

	psi_f_flags = PSI_F_ACTIVE | PSI_F_CREATE;
	emitter_lifetime = parameters.lifetime;
	num_partial_particles = parameters.initial_particle_count;

	return true;
}

//


bool COMAPI ParticleSystemInstance::is_active( void )
{
	return (psi_f_flags & PSI_F_ACTIVE);
}


//

bool COMAPI ParticleSystemInstance::set_parameters( const ParticleSystemParameters *new_parameters )
{
	S32 max_possible_particles;

	if( new_parameters->particle_lifetime > 0.0f ) {
		max_possible_particles = 1.25f * new_parameters->frequency * new_parameters->particle_lifetime + new_parameters->initial_particle_count;
	}
	else if( new_parameters->lifetime > 0.0f ) {
		max_possible_particles = 1.25f * (1.0f + new_parameters->frequency) + new_parameters->initial_particle_count;
	}
	else {
		max_possible_particles = 128 + new_parameters->initial_particle_count;
	}

	if( max_possible_particles > particles_array_size ) {

//		GENERAL_TRACE_5( TEMPSTR( "ParticleSystemInstance: set_parameters: allocating %d-entry particle buffer\n", max_possible_particles ) );

		// Allocate and initialize arrays for particles
		//
		Particle *new_particles;
		S32 *new_active_indices;

		if( (new_particles = new Particle[max_possible_particles]) == NULL ) {
			return false;
		}

		if( (new_active_indices = new S32[max_possible_particles]) == NULL ) {
			delete[] new_particles;
			return false;
		}

		if( particles_array_size ) {
			ASSERT( active_particles_indices );
			ASSERT( particles );

			memcpy( new_particles, particles, sizeof(Particle) * particles_array_size );
			
			if( num_active_particles ) {
				memcpy( new_active_indices, active_particles_indices, sizeof(S32) * num_active_particles );
			}
		}

		delete[] particles;
		particles = new_particles;

		delete[] active_particles_indices;
		active_particles_indices = new_active_indices;

		lru_active_index = 0;

		particles_array_size = max_possible_particles;

		verify_vertex_buffer( particles_array_size * 6 );
	}

	memcpy( &parameters, new_parameters, sizeof(parameters) );

	Vector	ZeroVec;

	ZeroVec.zero();
	if( ZeroVec.equal( parameters.emitter_direction, 0.001f ) ) {
		parameters.emitter_direction.set( 1.0f, 1.0f, 1.0f );
	}

	// set up fast integer based color keying
	//
	for( int i=0; i<PSP_NUM_COLOR_KEYS; i++ ) {
		color_frames[i] = ARGB_MAKE( ((int)(parameters.color_frames[i].x*255)), 
									 ((int)(parameters.color_frames[i].y*255)), 
									 ((int)(parameters.color_frames[i].z*255)), 
									 ((int)(parameters.color_frames[i].w*255)) );
	}

	// grab a texture ref
	//
	ITL_TEXTURE_ID tid;
	ITL_TEXTURE_REF_ID trid;

	if( texture_ref_id != ITL_INVALID_REF_ID ) {
		texturelibrary->release_texture_ref( texture_ref_id );
		texture_ref_id = ITL_INVALID_REF_ID ;
	}

	if( SUCCEEDED( texturelibrary->get_texture_id( parameters.texture_name, &tid ) ) ) {
		if( SUCCEEDED( texturelibrary->add_ref_texture_id( tid, &trid ) ) ) {
			texture_ref_id = trid;
			texturelibrary->get_texture_frame_count( tid, &texture_frame_count );
		}
		texturelibrary->release_texture_id( tid );
	}


	return true;
}

//

bool COMAPI ParticleSystemInstance::get_parameters( ParticleSystemParameters *out_parameters )
{
	memcpy( out_parameters, &parameters, sizeof(parameters) );
	return true;
}

//

GENRESULT COMAPI ParticleSystemInstance::QueryInterface( const C8 *interface_name, void **out_iif )
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

U32 COMAPI ParticleSystemInstance::AddRef( void )
{
	// Instances are not ref-counted here, their ref-count
	// is managed by the Optics component itself
	return 1;
}

//

U32 COMAPI ParticleSystemInstance::Release( void )
{
	// Instances are not ref-counted here, their ref-count
	// is managed by the Optics component itself
	return 1;
}


//

bool ParticleSystemInstance::initialize_from_archetype( const ParticleSystemArchetype *_archetype, ITextureLibrary *_texturelibrary, INSTANCE_INDEX engine_inst_index )
{
	ASSERT( _archetype != NULL );
	ASSERT( _texturelibrary != NULL );

	ParticleSystemParameters psp;

	inst_index = engine_inst_index;
	texturelibrary = _texturelibrary;

	const_cast<ParticleSystemArchetype*>(_archetype)->get_parameters( &psp );
	
	set_parameters( &psp );
	reset();

	return true;
}

//

bool ParticleSystemInstance::cleanup( )
{
	delete[] particles;
	particles = NULL;

	delete[] active_particles_indices;
	active_particles_indices = NULL;

	particles_array_size = 0;

	if( psi_f_flags & PSI_F_RELEASE_VB ) {
		del_ref_vertex_buffer();
		del_ref_vector_buffer();
	}

	if( texture_ref_id != ITL_INVALID_REF_ID ) {
		if( texturelibrary != NULL ) {
			texturelibrary->release_texture_ref( texture_ref_id );
		}
		else {
			GENERAL_TRACE_1( "ParticleSystemInstance: cleanup: orphaning texture reference (in dtor??)\n" );
		}
		texture_ref_id = ITL_INVALID_REF_ID ;
	}
	
	return true;
}

//

bool ParticleSystemInstance::update_system( SINGLE dt_s, IEngine *engine )
{
	if( !(psi_f_flags & PSI_F_POSITION_INIT) ) {
		last_engine_position = engine->get_position( inst_index );
		seconds_since_last_engine_position = 0;
		psi_f_flags |= PSI_F_POSITION_INIT;
	}
	else {
		seconds_since_last_engine_position += dt_s;
	}

    // check if event has expired
	//
	if(dt_s > 0.0f)
	{
		emitter_lifetime -= dt_s;
    
		if( parameters.lifetime > 0.0f && emitter_lifetime <= 0 ) {
			psi_f_flags &= ~(PSI_F_CREATE);
			if( num_active_particles == 0 || !(parameters.psp_f_flags & PSP_F_RENDER_PARTICLE_LIFE)  ) {
				psi_f_flags &= ~(PSI_F_ACTIVE);
				return false;		
			}
		}

		// advance active particles
		//
		S32 ai = 0;

		while( ai < num_active_particles ) {
			if( update_particle( &particles[active_particles_indices[ai]], dt_s, ai, true ) ) {
				ai++;
			}
			//	else {
			//		the particle that was at active_particles_indices[ai] has just 
			//		been deactivated and active_particles_indices[ai] now points to 
			//		a new particle, so we have to update this index again (but it
			//		is now a different particle
			//	}
		}
	}

	// create new particles... emitter needs update even on zero dt
	if( psi_f_flags & PSI_F_CREATE ) {
		update_emitter( dt_s, engine );
	}

	if (dt_s <= 0.0f)
		return true;	//can early out the rest during a zero dt

	// determine radius of the particle system
	//
	float dist_sq, max_d;
	Particle *particle;
	S32 p, *apa ;
	const Transform &object_to_world = engine->get_transform( inst_index );

	max_d = 0.0f;

	for( apa=active_particles_indices, p=0; p < num_active_particles; p++, apa++ ) {

		particle = &particles[ *apa ];

		if( parameters.psp_f_flags & PSP_F_RELATIVE_TRANSFORM ) {
			dist_sq = particle->position.magnitude_squared() ;
		}
		else {
			dist_sq = (particle->position - object_to_world.translation).magnitude_squared();
		}

		dist_sq += particle->size * particle->size;

		if( dist_sq > max_d ) {
			max_d = dist_sq;
		}
	}

	max_d = sqrt( max_d );

	engine->set_instance_bounding_sphere( inst_index, 0, max_d, Vector(0,0,0) );

	seconds_since_last_new_particle += dt_s;

	return true;
}

//

bool ParticleSystemInstance::update_particle( Particle *particle, float dt_s, S32 active_particles_index, bool check_freshness )
{
	SINGLE twist;

	if( dt_s < 0.0f ) {
		return true;
	}

	// do this early and see if we can skip out
	//
	if( parameters.particle_lifetime > 0.0f ) {
		if( (particle->lifetime - dt_s) <= 0 ) {

			particle->lifetime = 0.0f;
			particle->p_f_flags |= P_F_DYING;

			deactivate_particle( active_particles_index );
			return false ;
		}
	}

    // update twist
	//
    if( parameters.particle_twist_velocity != 0.0f ) {
        twist = parameters.particle_twist_velocity * dt_s;
        particle->direction.x -= particle->direction.y * twist;
        particle->direction.y += particle->direction.x * twist;
    }

    // update size
	//
    if( parameters.particle_size_velocity ) {
        particle->size += parameters.particle_size_velocity * dt_s;            
    }

    // update color
	//
	const float f_max_color_key = (float)(PSP_NUM_COLOR_KEYS - 1);
	const int i_max_color_key = (int)(PSP_NUM_COLOR_KEYS - 1);
	S32 r,g,b,a ;
	S32 current_color, next_color;
	int	idx;
	float scale;
	float norm_life;

	if( parameters.particle_lifetime ) {
		norm_life = (float)particle->lifetime / (float)parameters.particle_lifetime ;
		idx	= i_max_color_key - (int)( norm_life * f_max_color_key );
		scale = (idx - ( f_max_color_key * ( 1.0f - norm_life))) - 1.0f;
	}
	else {
		norm_life = (1.0f - parameters.lifetime) * (f_max_color_key * 0.001f);
		idx = abs( ((int)norm_life) % i_max_color_key );
		scale = fabs( norm_life );
		scale = idx - fmod( scale, f_max_color_key );
	}

	idx = Tclamp<int>( 0, i_max_color_key, idx );

	current_color = color_frames[idx];
	r = ARGB_R(current_color);
	g = ARGB_G(current_color);
	b = ARGB_B(current_color);
	a = ARGB_A(current_color);


	if( (idx + 1) < i_max_color_key ) {
		S32 int_scale = 1024 * scale;
		next_color = color_frames[idx+1];
		r += ((r - ARGB_R(next_color)) * int_scale) >> 10;
		g += ((g - ARGB_G(next_color)) * int_scale) >> 10;
		b += ((b - ARGB_B(next_color)) * int_scale) >> 10;
		a += ((a - ARGB_A(next_color)) * int_scale) >> 10;
	}

	particle->color = ARGB_MAKE( r, g, b, a );


    // update position & direction
	//
	particle->direction	+= parameters.gravity * dt_s;
    particle->position += (particle->direction * (particle->velocity * dt_s));

	particle->lifetime -= dt_s;

	return true;
}

//

inline void psi_add_quad( RPVertex **pvb, int *vb_cnt, Particle *particle, ITL_TEXTUREFRAME_IRP *frame )
{
	RPVertex *vb = *pvb;

	vb->pos.x = particle->position_in_camera.x - particle->size ;
	vb->pos.y = particle->position_in_camera.y + particle->size ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = frame->u0;
	vb->v = frame->v1;
	vb++; (*vb_cnt)++;

	vb->pos.x = particle->position_in_camera.x + particle->size ;
	vb->pos.y = particle->position_in_camera.y + particle->size ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = frame->u1;
	vb->v = frame->v1;
	vb++; (*vb_cnt)++;

	vb->pos.x = particle->position_in_camera.x - particle->size ;
	vb->pos.y = particle->position_in_camera.y - particle->size ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = frame->u0;
	vb->v = frame->v0;
	vb++; (*vb_cnt)++;

	vb->pos.x = particle->position_in_camera.x + particle->size ;
	vb->pos.y = particle->position_in_camera.y - particle->size ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = frame->u1;
	vb->v = frame->v0;
	vb++; (*vb_cnt)++;

	vb->pos.x = particle->position_in_camera.x - particle->size ;
	vb->pos.y = particle->position_in_camera.y - particle->size ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = frame->u0;
	vb->v = frame->v0;
	vb++; (*vb_cnt)++;

	vb->pos.x = particle->position_in_camera.x + particle->size ;
	vb->pos.y = particle->position_in_camera.y + particle->size ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = frame->u1;
	vb->v = frame->v1;
	vb++; (*vb_cnt)++;

	*pvb = vb;
}

//

inline void psi_add_tri( RPVertex **pvb, int *vb_cnt, Particle *particle, ITL_TEXTUREFRAME_IRP *frame )
{
	RPVertex *vb = *pvb;

	// center the triangle to particle center and draw it
	float up = 0.525 * particle->size;

	vb->pos.x = particle->position_in_camera.x ;
	vb->pos.y = particle->position_in_camera.y + up ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = 0.0f;
	vb->v = 1.0f;
	vb++; (*vb_cnt)++;

	vb->pos.x = particle->position_in_camera.x + particle->size ;
	vb->pos.y = particle->position_in_camera.y - up ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = 1.0f;
	vb->v = 1.0f;
	vb++; (*vb_cnt)++;

	vb->pos.x = particle->position_in_camera.x - particle->size ;
	vb->pos.y = particle->position_in_camera.y - up ;
	vb->pos.z = particle->position_in_camera.z ;
	vb->color = particle->color ;
	vb->u = 0.0f;
	vb->v = 0.0f;
	vb++; (*vb_cnt)++;

	*pvb = vb;
}

//

vis_state ParticleSystemInstance::render_system( IRenderPrimitive *renderprimitive, IEngine *engine, ICamera *camera, float lod_fraction )
{
	ASSERT( camera != NULL );

	if( !(psi_f_flags & PSI_F_ACTIVE) || (num_active_particles <= 0) ) {
		return VS_NOT_VISIBLE;
	}

	const Transform &view_to_world = camera->get_transform() ;
	const Transform &world_to_view = camera->get_inverse_transform();
	const Transform &object_to_world = engine->get_transform( inst_index );

	S32 p, *apa ;


	// transform active particles to camera space
	//

	// If the particles are supposed to be (bound) relative to the emitter,
	// then concatenate the current emitter transform onto the world-to-view
	// transform.
	//
	if( parameters.psp_f_flags & PSP_F_RELATIVE_TRANSFORM ) {

		Transform particle_to_view( world_to_view * object_to_world );
		
		for( apa=active_particles_indices, p=0; p < num_active_particles; p++, apa++ ) {
			ASSERT( *apa < particles_array_size );
			particles[*apa].position_in_camera = particle_to_view * particles[*apa].position;
		}

	}
	else {
		for( apa=active_particles_indices, p=0; p < num_active_particles; p++, apa++ ) {
			ASSERT( *apa < particles_array_size );
			particles[*apa].position_in_camera = world_to_view * particles[*apa].position;
		}
	}


	// build draw_primitive buffer
	//
	S32 ai = 0;
	RPVertex *vb;
	int vb_count, use_quads;
	ITL_TEXTUREFRAME_IRP frame;
	
	use_quads = ( fabs( parameters.texture_fps ) < 0.0001f ) ? 0 : 1;
	if( !use_quads ) {
		texturelibrary->get_texture_ref_frame( texture_ref_id, 0, &frame );
	}

	vb = pe_vertex_buffer;
	vb_count = 0;

	for( ai=0; ai<num_active_particles; ai++ ) {

		Particle *particle = &particles[ active_particles_indices[ai] ];

		if (use_quads) {
			U32 frame_to_get = (U32)floor( fmod( (parameters.particle_lifetime - particle->lifetime) * parameters.texture_fps, texture_frame_count ) );
			texturelibrary->get_texture_ref_frame( texture_ref_id, frame_to_get, &frame );

			psi_add_quad( &vb, &vb_count, particle, &frame );
		}
		else {
			psi_add_tri( &vb, &vb_count, particle, &frame );
		}
	}

	// render vertex buffer
	//
	U32 pushZWrite;
	U32 pushDither;
	U32 pushFogEnable;
	
	renderprimitive->set_state( RPR_STATE_ID, frame.rp_texture_id );

	renderprimitive->set_modelview( Transform() );

	renderprimitive->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );

	renderprimitive->set_render_state( D3DRS_LIGHTING, FALSE );

	renderprimitive->get_render_state( D3DRS_ZWRITEENABLE, &pushZWrite );
	renderprimitive->get_render_state( D3DRS_DITHERENABLE, &pushDither );
	renderprimitive->get_render_state( D3DRS_FOGENABLE,	&pushFogEnable );

	renderprimitive->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
	renderprimitive->set_render_state( D3DRS_DITHERENABLE, (parameters.psp_f_flags & PSP_F_RENDER_DITHER)? 1 : 0 );
	renderprimitive->set_render_state( D3DRS_FOGENABLE,	(parameters.psp_f_flags & PSP_F_RENDER_FOG)? 1 : 0 );
	
	renderprimitive->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0 );
	renderprimitive->set_texture_stage_texture( 0, frame.rp_texture_id );

	renderprimitive->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	renderprimitive->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	renderprimitive->set_texture_stage_texture( 1, 0 );
	
	if( parameters.src_blend == D3DBLEND_ONE && parameters.dst_blend == D3DBLEND_ZERO ) {
		renderprimitive->set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
	}
	else {
		renderprimitive->set_render_state(D3DRS_ALPHABLENDENABLE, TRUE );
		renderprimitive->set_render_state(D3DRS_SRCBLEND, parameters.src_blend );
		renderprimitive->set_render_state(D3DRS_DESTBLEND, parameters.dst_blend );
	}

	renderprimitive->draw_primitive( D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, pe_vertex_buffer, vb_count, 0 );

	renderprimitive->set_render_state( D3DRS_DITHERENABLE, pushDither );
	renderprimitive->set_render_state( D3DRS_ZWRITEENABLE, pushZWrite );
	renderprimitive->set_render_state( D3DRS_FOGENABLE,	pushFogEnable );

	renderprimitive->set_state( RPR_STATE_ID, 0 );

	return VS_PARTIALLY_VISIBLE; //vs;
}

//

bool ParticleSystemInstance::update_emitter( float dt_s, IEngine *engine )
{
	const Vector &engine_position = engine->get_position( inst_index );

	//the emitter needs this update even in the case of a zero dt
	//the engine position can change during updates with a zero dt
	//without the update, the emission of instantly moved particle systems
	//will be lerped over the current update's dt, which would cause
	//particles to be created along the path of the warp, also making
	//for a very large bounding radius.

	if ( dt_s > 0.0f )
	{
		S32 num_new_particles, first_new_particle;
		
		if(parameters.frequency > 0)
		{
			num_partial_particles += dt_s * parameters.frequency;
		}
		
		num_new_particles = (S32)num_partial_particles;
		
		if( num_new_particles < 1 || (parameters.max_particle_count && (num_created_particles > parameters.max_particle_count)) ) {
			return true;
		}
		
		// clamp the number of new particles to a number 
		// we can handle with the current particle array
		//
		if( num_new_particles <= particles_array_size ) {
			first_new_particle = 0;
		}
		else {
			first_new_particle = num_new_particles - particles_array_size;
			//		GENERAL_TRACE_3( TEMPSTR( "update_emitter: new:%d max:%d first:%d\n", num_new_particles, particles_array_size, first_new_particle ) );
		}
		
		
		bool degenerate_damp;
		Vector position, delta_position, velocity;
		SINGLE period;
		float random;
		Particle *particle;
		S32 active_particles_index;
		
		const Matrix &engine_orientation = engine->get_orientation( inst_index );
   
		const Vector &engine_velocity = engine->get_velocity( inst_index );
		
		if( parameters.frequency ) {
			period = 1.0f / parameters.frequency;
		}
		else {
			period = 0.0f;
		}
		
		// correct current position (if necessary) for time that passed since the last 
		// emitter update.  simply lerp our position based on the last known position
		//
		if( seconds_since_last_engine_position ) {
			velocity = -(engine_position - last_engine_position) / seconds_since_last_engine_position;
			delta_position = velocity * period;
			random = ( seconds_since_last_engine_position - seconds_since_last_new_particle + period );
			position = engine_position + velocity * random;
		}
		else {
			position = engine_position;
		}
		
		degenerate_damp = (parameters.emitter_nozzle_damp.magnitude_squared()==0);
		
		// generate new particles
		//
		for( S32 i = first_new_particle; i < num_new_particles ; i++ ) {
			
			float p_dt_s = (seconds_since_last_new_particle - i * period) ;
			
			num_partial_particles -= 1.0f;
			
			if( parameters.max_particle_count && (++num_created_particles > parameters.max_particle_count) ) {
				num_created_particles--;
				return false;
			}
			
			if( p_dt_s < 0.0f ) {
				continue;
			}
			
			activate_particle( &particle, &active_particles_index );
			
			ASSERT( particle != NULL );
			
			// position, velocity, and direction
			//
			
			if( !degenerate_damp ) {
				particle->direction = parameters.emitter_direction;
				
				particle->direction.scale( parameters.emitter_nozzle_size );
				
				particle->direction.x += f_rand() * parameters.emitter_nozzle_damp.x;
				particle->direction.y += f_rand() * parameters.emitter_nozzle_damp.y;
				particle->direction.z += f_rand() * parameters.emitter_nozzle_damp.z;
				
				if( particle->direction.magnitude_squared() ) {
					particle->direction.normalize();
				}
				
				// If the particles are supposed to be bound (relative) to the emitter
				// then ignore the emitter orientation when creating this particle.
				//
				if( !(parameters.psp_f_flags & PSP_F_RELATIVE_TRANSFORM) ) {
					particle->direction = engine_orientation * particle->direction;
				}
				
				random = f_abs_rand() * parameters.particle_velocity_randomizer;
				
				particle->velocity = parameters.particle_velocity + parameters.particle_velocity * random;
			}
			else {
				particle->direction = parameters.emitter_direction;
				particle->velocity = 0;
			}
			
			// If the particles are supposed to be bound (relative) to the emitter
			// then create the particle *at* the emitter which means that the position
			// of the particle is initially zero (0,0,0) in the emitter coordinate space.
			//
			// Otherwise, create the particle at the position of the emitter in world
			// space.
			//
			if( parameters.psp_f_flags & PSP_F_RELATIVE_TRANSFORM ) {
				particle->position.zero();
			}
			else {
				particle->position = position;
				position += delta_position;
			}
			
			// add a random position offset
			//
			if( parameters.particle_position_randomizer ) {
				particle->position.x += f_rand() * parameters.particle_position_randomizer ;
				particle->position.y += f_rand() * parameters.particle_position_randomizer ;
				particle->position.z += f_rand() * parameters.particle_position_randomizer ;
			}
			
			// Inherit velocity of the emitter by adding it into the initial velocity
			// of the created particle.
			//
			if( parameters.psp_f_flags & PSP_F_RELATIVE_VELOCITY ) {
				velocity = particle->direction * particle->velocity + engine_velocity;
				particle->velocity = velocity.magnitude();
				particle->direction = velocity.normalize();
			}
			
			// advance the simulation for this particle up to the point
			// that it should be...
			//
			update_particle( particle, p_dt_s, active_particles_index, false );
		}
	}
	
	//update position tracking variables
	last_engine_position = engine_position;
	seconds_since_last_engine_position = 0;
	seconds_since_last_new_particle = 0;

	return true;
}

//

bool ParticleSystemInstance::activate_particle( Particle **out_particle, S32 *out_active_particle_index )
{
	S32 pi, ai;
	Particle *particle = NULL;

    // If all available slots are taken, use the least
	// recently activated particle slot.
	//
    if( num_active_particles >= particles_array_size ) {
		*out_active_particle_index = lru_active_index;
		particle = &particles[lru_active_index];
		lru_active_index = (lru_active_index + 1) % particles_array_size ;

		//GENERAL_TRACE_3( "using lru slot\n" );
    }
	else {
		// search for inactive particle 
		//
		pi = 0;
		do {
			ai = next_active_particle_index ;
			next_active_particle_index = (next_active_particle_index+1) % particles_array_size;

			if( !(particles[ai].p_f_flags & P_F_ACTIVE) ) {
				break;
			}

			pi++;
		} while( pi<particles_array_size );

		active_particles_indices[num_active_particles] = ai;
		*out_active_particle_index = num_active_particles;
		num_active_particles++;

		particle = &particles[ai];

		max_active_particles = Tmax( num_active_particles, max_active_particles );
	}

	// activate the particle
	//
	*out_particle = particle;
	if( particle ) {
		particle->p_f_flags = P_F_ACTIVE | P_F_FRESH;
		particle->size = parameters.particle_size;
		particle->lifetime = parameters.particle_lifetime;
		particle->color = color_frames[0];
	}

	return true;
}

//

void ParticleSystemInstance::deactivate_particle( S32 active_particle_index )
{
	ASSERT( active_particle_index < particles_array_size );

	S32 ai = active_particles_indices[active_particle_index];

    particles[ ai ].p_f_flags &= ~(P_F_ACTIVE);
	next_active_particle_index = ai;

    active_particles_indices[active_particle_index] = active_particles_indices[num_active_particles-1]; 

	num_active_particles--;
}


// EOF
