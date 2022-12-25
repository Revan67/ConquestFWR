#ifndef __ParticleSystemParameters_h__
#define __ParticleSystemParameters_h__

//

#include "vector.h"
#include "vector4.h"

//

#define PSP_IFS_KEY "ParticleSystemParameters"	// use this filename to save/load from IFileSystem

//

#define PSP_NUM_COLOR_KEYS		32
#define PSP_TEXTURE_NAME_LEN	16

//

#define PSP_F_RELATIVE_TRANSFORM	(1<<0)	// Particle system transform is relative to parents transform
#define PSP_F_RELATIVE_VELOCITY		(1<<1)	// Particles emitted relative to parent's velocity
#define PSP_F_IGNORE_ORIENTATION	(1<<2)	// Particle system ignore orientation

#define PSP_F_RENDER_PARTICLE_LIFE	(1<<3)	// Render system until last particle is dead.
#define PSP_F_RENDER_DITHER			(1<<4)	// Render with dithering enabled.
#define PSP_F_RENDER_FOG			(1<<5)	// Render with fog enabled.

// ...................................................................................
//
// ParticleSystemParameters
//
// Used to describe a particle system.
//
//
struct ParticleSystemParameters
{
	U32			psp_f_flags;

	// Rendering Related
	//
	Vector4		color_frames[PSP_NUM_COLOR_KEYS];		// R,G,B,A key frames of particles in system	
	U32			color_key_frame_bits;					// Specifies which key frames in color_key_frames are
														// actually keyframes.  This is used by the editor and
														// is actually not really important to the runtime.
														

	char		texture_name[PSP_TEXTURE_NAME_LEN];		// Name of the texture to use when rendering.
	float		texture_fps;							// Framerate of the texture if animated. [frames/second]

	U32			src_blend;								// D3DBLEND_* framebuffer source blend factor
	U32			dst_blend;								// D3DBLEND_* framebuffer dest blend factor

	// Simulation related
	//
	Vector		gravity;								// Applied to particle direction to effect gravity. [units/second]

	Vector		emitter_direction;						// Direction along which particles are emitted. [units]
	float		emitter_nozzle_size;					// Determines scale along the emitted direction. (-infinity, +infinity)
	Vector		emitter_nozzle_damp;					// Determines how much the nozzle affects an axis. x,y,z should be in the range [0,1]

	S32			initial_particle_count;					// Number of initial particle in the system.
	S32			max_particle_count;						// Maximum number of particles to emit over lifetime.
	float		lifetime;								// Amount of time to allow particle emission. Zero means forever. [seconds]
	float		frequency;								// Frequency of particle emission. [particles/second]

	float		particle_lifetime;						// Initial lifetime of a particle (in which it is active). Zero means forever. [seconds] 
	float		particle_position_randomizer;			// Determines amount of random offset to add to the initial particle position. [units]

	float		particle_velocity;						// Speed of newly emitted particles. [units/second]
	float		particle_velocity_randomizer;			// Determines amount of random velocity added to
														// the emitted velocity.  This should be in the range [0,1]

	float		particle_twist_velocity;				// Speed at which particle cloud rotates in XY plane. [units/second]

	float		particle_size;							// Initial size of particles in system [units]
	float		particle_size_velocity;					// Particle size velocity [units/second]

	// Extents related
	//
	float		bounding_sphere_radius;					// Maximum radius of effect over its rendering lifetime. [units]
};


#endif