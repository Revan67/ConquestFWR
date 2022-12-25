// ParticleSystemInstance.h
//
//
//

#ifndef __ParticleSystemInstance_h__
#define __ParticleSystemInstance_h__

//

#include "Engine.h"
#include "IRenderPrimitive.h"

//

#include "ParticleSystemArchetype.h"

//

// ........................................................................
//
// Particle
//
// A single quanta of particle system goodness.  In a particle system
// of 'N' particles, there will be and array (or something) of 'N' of these.
//
struct Particle
{
#define P_F_ACTIVE		(1<<0)
#define P_F_FRESH		(1<<1)
#define P_F_DYING		(1<<2)

	Vector		position;			// Particle's position in object space
	SINGLE		size;				// Particle's onscreen size
	Vector		position_in_camera;	// Particle's position in camera space.
	SINGLE		velocity;			// Particle's speed
	Vector		direction;			// Particle's direction (normalized)
	SINGLE		lifetime;			// Particle's lifetime (in ms)
	PACKEDARGB	color;				// Particle's color (including alpha)
	U32			p_f_flags;			// Particle's state flags

	Particle( void );
};

//

inline Particle::Particle( void ) 
{
	p_f_flags = 0;
}

//

#define PSI_F_ACTIVE		(1<<0)	// Set if this particle system is alive.
#define PSI_F_CREATE		(1<<1)	// Set if this particle system can still create new particles.
#define PSI_F_POSITION_INIT	(1<<2)
#define PSI_F_RELEASE_VB	(1<<3)	// Set if we need to release our reference to the vertex buffer

//

// ........................................................................
//
//
//
//
//
struct ParticleSystemInstance : public IParticleSystem
{
public: // Interface

	// IParticleSystem
	bool COMAPI initialize( void );
	bool COMAPI reset( void ) ;
	bool COMAPI is_active( void );
	bool COMAPI set_parameters( const ParticleSystemParameters *new_parameters );
	bool COMAPI get_parameters( ParticleSystemParameters *out_parameters );

	// IDAComponent
	GENRESULT COMAPI QueryInterface( const C8 *interface_name, void **out_iif );
	U32 COMAPI AddRef( void );
	U32 COMAPI Release( void );

	ParticleSystemInstance( );
	ParticleSystemInstance( const ParticleSystemInstance &psi );
	
	bool initialize_from_archetype( const ParticleSystemArchetype *_archetype, ITextureLibrary *texturelibrary, INSTANCE_INDEX engine_inst_index );
	bool cleanup( void );

	bool update_system( float dt_s, IEngine *engine );
	vis_state render_system( IRenderPrimitive *renderprimitive, IEngine *engine, ICamera *camera, float lod_fraction );

protected: // Interface
	bool update_emitter( float dt_s, IEngine *engine );
	bool update_particle( Particle *particle, float dt_s, S32 active_particle_index, bool check_freshness );
	
	bool activate_particle( Particle **out_particle, S32 *out_active_particle_index );
	void deactivate_particle( S32 active_particle_index );

public: // Data

	INSTANCE_INDEX	inst_index;

	ParticleSystemParameters parameters;		// Parameters that define the system
											
	U32				psi_f_flags;				// 
												
	S32				particles_array_size;		// Number of entries in particles and active_particles.
	Particle *		particles;					// All of the particles created for this system
	S32 *			active_particles_indices;	// Indices into particles of all of the active particles.
	S32				num_active_particles;		// Total number of active indices in 'particles' 
												// (also the entry of the first invalid index in active_particles_indices)
	S32				num_created_particles;		// Number of particles created since birth.

	S32				lru_active_index;			// index into 'active_particles' of particle to replace when 
												// there are too many active particles.
	S32				next_active_particle_index;	// index into 'particles' to start searching for non-active index to use
										
	// these are derived from the parameter color key frame values
	// we do any necessary repacking once upfront
	//
	PACKEDARGB		color_frames[PSP_NUM_COLOR_KEYS];	

	ITextureLibrary    *texturelibrary;
	ITL_TEXTURE_REF_ID	texture_ref_id;			// reference to the texture to use in rendering.
	U32					texture_frame_count;	// number of frames of animation in the texture 

	//
	float			emitter_lifetime;			// number of seconds left in the emitters life
	float			num_partial_particles;		// Accumulator for particle creation
					
	// used to interpolate new particles created "between updates"
	//
	float			seconds_since_last_new_particle;
	float			seconds_since_last_engine_position;
	Vector			last_engine_position;		

	S32				max_active_particles;	// temporary

};


#endif	// EOF


