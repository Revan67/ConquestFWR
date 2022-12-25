#ifndef PSYS_H
#define PSYS_H
//
// PSys.h - A new initiative in particle systems for DA
//

//
// Include files
//

#include <xform.h>
#include <icamera.h>
#include <irenderprimitive.h>

//
// Class and structure definitions
//

// Forward declarations
class IParticleSystem;

// The shared information for each particle.
struct Particle
{
	// *** TODO: Make this structure smaller.
	Transform xform;       // current transform.
	float     birthTime;   // when the particle was created, in system time.
	bool      alive:1;     // indicates if this particle is participating in the system
};

// Structure used to specify the generic starting data for a particle.
struct ParticleStartData
{
	Transform xform;   // starting position and orientation
	Vector    vel;     // starting linear velocity
	Vector    avel;    // starting angular velocity
};

// The interface for modifying particle position and orientation, as well as killing particles.
class IParticleBehavior
{
public:
	// Informs this behavior of its associated system. If system is NULL, it is being detached from afs system.
	virtual bool set_system (IParticleSystem *system, unsigned int maxParticles) = 0;

	// Applies this behavior to the particles in the list.
	virtual bool update (float dt) = 0;
	
	// Informs this behavior that a new particle has been created and what its index is.
	virtual bool create_particle (unsigned int which, const ParticleStartData &data) = 0;

	// Informs this behavior that a particle has been destroyed.
	virtual void destroy_particle (unsigned int which) = 0;
};

// The interface for rendering individual particles, and modifying their visual state over time.
class IParticleRenderer
{
public:
	// Informs this renderer of its associated system. If system is NULL, it is being detached from a system.
	virtual bool set_system (IParticleSystem *system, unsigned int maxParticles) = 0;

	// Renders the particles of the associated system using the given pipeline, camera, and transform.
	virtual void render (IRenderPrimitive *prim, ICamera *cam, const Transform &xform) = 0;
	
	// Updates the render information for all of the particles
	virtual void update (float dt) = 0;

	// Informs this renderer that a new particle has been created and what its index is.
	virtual bool create_particle (unsigned int which, const ParticleStartData &data) = 0;

	// Informs this renderer that a particle has been destroyed.
	virtual void destroy_particle (unsigned int which) = 0;
};

// The interface for emitting particles.
class IParticleEmitter
{
public:
	// Informs this emitter of its associated system. If system is NULL, it is being detached from a system.
	virtual bool set_system (IParticleSystem *system, unsigned int maxParticles) = 0;

	// Emits particles when required
	virtual void update (float dt) = 0;
};

//: The interface for an entire particle system.
// This interface is used for both simple and complex particle systems. The particle system can be stopped and started,
// reset to time 0, and emission of particles can be controlled seperately from the particle's motion. The normal method
// of creating particles is via an emitter, but the app can explicitly create particles as well.
// The updating and rendering of the particle system is under app control.
class IParticleSystem
{
public:
	// Initialization
	virtual bool init (unsigned int maxParticles, IParticleBehavior *_behave, IParticleRenderer *_renderer, IParticleEmitter *_emit) = 0;
	virtual void shutdown () = 0;

	// Draw the particles for this system
	virtual void render (IRenderPrimitive *prim, ICamera *cam) = 0;  // renders the particle system

	// Update the particle positions for this system.
	// true if particle system still has live particles, false otherwise
	virtual bool update (float dt) = 0;   // updates the emitter, behavior, and renderer, in that order.

	// Method for injecting particles into the system.
	virtual bool create_particle (const ParticleStartData &data) = 0; // creates a particle with the given data.
	virtual void destroy_particle (unsigned int which) = 0;           // destroys the indexed particle.

	// Get and set the coordinate system for this system.
	virtual void set_transform (const Transform &_xform) = 0;  // sets the base transform for this particle system
	virtual const Transform & get_transform() = 0;             // gets the base transform for this particle system

	// Start and stop particle emission
	virtual void allow_emission (bool allowed=true) = 0; // starts and stops the emission of particles

	// Start, stop, and reset the system time, thus the behavior
	virtual void start () = 0;  // starts the emission and behavior of the particle system.
	virtual void stop () = 0;   // pauses the emission and behavior of the particle system.
	virtual void reset () = 0;  // kills all the particles and sets the particle system time to 0.

	// Particle access. This is guaranteed to be efficient when accessed linearly.
	virtual unsigned int get_particle_count () = 0;         // retrieves the count of active particles in the system.
	virtual unsigned int get_max_particles () = 0;          // retrieves the max number of particles in the system.
	virtual Particle &get_particle(unsigned int which) = 0; // retrieves a reference to an individual particle
	virtual Particle *get_particle_array () = 0;            // retrieves a pointer to the entire particle array

	// Misc.
	virtual float get_time () = 0;  // gets the current particle system time.
};

// Global functions

extern bool init_particles ();

#endif
