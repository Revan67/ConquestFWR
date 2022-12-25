//
// PSys.cpp - A new initiative for particle systems at DA
//

//
// Include files
//

#include <fdump.h>
#include <tsmartpointer.h>
#include <itexturelibrary.h>
#include <engine.h>
#include <filesys.h>
#include <viewcnst.h>

#include <stdlib.h>

#include "script.h"
#include "psys.h"

//
// Imported variables
//

extern ICOManager *      DACOM;
extern IEngine *         ENGINE;
extern ITextureLibrary * TLIB;

//
// Local variables
//

static IViewConstructor *PARSER = NULL;

//
// Local routines
//
static float frand ()
{
	return ((float) rand()) / ((float) RAND_MAX);
}

static float crand (float min, float max)
{
	ASSERT (min <= max);
	return min + frand() * (max - min);
}

static Vector vrand ()
{
	// Return a random unit vector
	Vector v (crand(-1.0, 1.0), crand (-1.0, 1.0), crand (-1.0, 1.0));
	v.normalize ();
	return v;
}

static Vector cone_vrand (const Vector &dir, SINGLE angle)
{
	// Calculate a random unit vector in the cone defined by the given direction vector and angle.
	// NOTE: It is assumed that "dir" is a unit vector.
	// Algorithm:
	// 1) Rotate the vector a bit around the axis. If the vector is the same, use the y axis.
	// 2) Cross the mirror with dir to get a vector perpendicular to dir.
	// 3) Rotate a copy of dir around the perpendicular vector a random angle between 0 and +angle.
	// 4) Rotate the rotated copy around dir by a random angle between 0 and 360.

	Vector perp = cross_product (Vector (1,0,0), dir);
	SINGLE mag = perp.magnitude();
	if (mag < 1e-4)
	{
		perp = cross_product (Vector (0,1,0), dir);
		mag = perp.magnitude();
	}

	if (mag < 1e-4)
	{
		perp = cross_product (Vector (0,0,1), dir);
		mag = perp.magnitude();
	}

	perp /= mag;

	Quaternion q1 (perp, crand (0, MUL_DEG_TO_RAD * angle));
	Quaternion q2 (dir, crand (0, 2 * PI));

	Matrix m1(q1), m2(q2);

	return ((m2 * m1) * dir);
}

//
// Class and structure definitions
//

// A basic implementation of IParticleSystem

class BasicSystem : public IParticleSystem
{
private:
	Particle *          parts;        // the particles
	unsigned int        maxParts;     // the length of parts[]
	unsigned int        partCount;    // the number of valid particles in parts[]
	IParticleBehavior * behave;
	IParticleRenderer * renderer;
	IParticleEmitter *  emit;
	Transform           baseXform;
	bool                initialized;  // the particle system is properly initialized.
	float               sysTime;      // the current time for the system
	bool                timeStopped;
	bool                emitActive;

public:
	// Constructors
	BasicSystem ();
	~BasicSystem ();

	// === IParticleSystem methods === 
	virtual bool init (unsigned int maxParticles, IParticleBehavior *_behave, IParticleRenderer *_renderer, IParticleEmitter *_emit);
	virtual void shutdown ();

	// Draw the particles for this system
	virtual void render (IRenderPrimitive *prim, ICamera *cam);

	// Update the particle positions for this system
	virtual bool update (float dt);
		// true if particle system still has live particles, false otherwise

	// Method for injecting particles into the system.
	// NOTE: The emitter will use the
	virtual bool create_particle (const ParticleStartData &data);
	virtual void destroy_particle (unsigned int which);

	// Get and set the coordinate system for this system.
	virtual void set_transform (const Transform &_xform);
	virtual const Transform & get_transform();

	// Start and stop particle emission
	virtual void allow_emission (bool allowed=true);

	// Start and stop the emitter
	virtual void start ();
	virtual void stop ();
	virtual void reset ();

	// Particle access. This is guaranteed to be efficient when accessed linearly.
	virtual unsigned int get_particle_count ();
	virtual unsigned int get_max_particles ();
	virtual Particle &get_particle(unsigned int which);
	virtual Particle *get_particle_array ();

	// Misc.
	virtual float get_time ();
};

BasicSystem::BasicSystem ()
{
	partCount = 0;
	parts = NULL;
	behave = NULL;
	renderer = NULL;
	emit = NULL;
	initialized = false;
	emitActive = true;
	timeStopped = false;
}

BasicSystem::~BasicSystem ()
{
	shutdown ();
}

bool BasicSystem::init (unsigned int maxParticles, IParticleBehavior *_behave, IParticleRenderer *_renderer, IParticleEmitter *_emit)
{
	// Check the input parameters
	ASSERT(_behave != NULL);
	ASSERT(_renderer != NULL);
	ASSERT(_emit != NULL);
	ASSERT(maxParticles > 0);

	// Allocate the particle array.
	if (initialized)
	{
		GENERAL_WARNING("You should not initialize a particle system more than once!\n");
		return false;
	}

	partCount = 0;
	maxParts = maxParticles;
	parts = new Particle[maxParts];

	if (parts == NULL)
	{
		GENERAL_ERROR("Failed to allocate the particle array.\n");
		return false;
	}

	// Initialize all of the particles to dead.
	{
		int count = maxParts;
		Particle *here = parts;
		while (count--)
		{
			here->alive = false;
			++here;
		}
	}

	// Set the behavior, renderer, and emitter for this system, and inform them that this is their system.
	// If any of them fail, we will return failure.

	ASSERT(behave == NULL);
	ASSERT(renderer == NULL);
	ASSERT(emit == NULL);

	if (!_behave->set_system (this, maxParts))
	{
		GENERAL_TRACE_1("Failed to attach the behavior to this particle system.\n");
		shutdown ();
		return false;
	}
	behave = _behave;

	if (!_renderer->set_system (this, maxParts))
	{
		GENERAL_TRACE_1("Failed to attach the renderer to this particle system.\n");
		shutdown ();
		return false;
	}
	renderer = _renderer;

	if (!_emit->set_system (this, maxParts))
	{
		GENERAL_TRACE_1("Failed to attach the emitter to this particle system.\n");
		shutdown ();
		return false;
	}
	emit = _emit;

	// Everything is hooked up, so return success.
	initialized = true;
	sysTime = 0.0f;
	timeStopped = false;
	emitActive = true;
	return true;
}

void BasicSystem::shutdown ()
{
	if (parts != NULL)
	{
		delete [] parts;
		parts = NULL;
		maxParts = 0;
	}

	partCount = 0;

	if (behave != NULL)
	{
		behave->set_system (NULL, 0);
		behave = NULL;
	}

	if (renderer != NULL)
	{
		renderer->set_system (NULL, 0);
		renderer = NULL;
	}
	
	if (emit != NULL)
	{
		emit->set_system (NULL, 0);
		emit = NULL;
	}

	initialized = false;
}

void BasicSystem::render (IRenderPrimitive *prim, ICamera *cam)
{
	// Simply reflect the command to the associated renderer.
	if (renderer)
	{
		renderer->render (prim, cam, baseXform);
	}
}

bool BasicSystem::update (float dt)
{
	// If time is not stopped, update our current concept of time,
	// update the emitter, behavior, and renderer, in that order.
	if (!timeStopped)
	{
		ASSERT(dt >= 0.0);
		sysTime += dt;
		
		// NOTE: Calls to create_particle() and destroy_particle() can occur during these
		// updates.

		if (emit && emitActive)
		{
			emit->update (dt);
		}

		if (behave)
		{
			behave->update (dt);
		}

		if (renderer)
		{
			renderer->update (dt);
		}
	}

	return partCount > 0;
}

bool BasicSystem::create_particle (const ParticleStartData &data)
{
	// Find the first dead particle.

	unsigned int i = 0;
	Particle *here = parts;
	int newPart = -1;
	while (i < maxParts)
	{
		if (!here->alive)
		{
			newPart = i;
			break;
		}
		++here;
		++i;
	}

	// If we didn't find a dead particle, punt and return a failure.
	// Emitters can take this into concideration when implementing particle creation.

	if (newPart == -1)
	{
		return false;
	}

	// We have a particle. Set its initial transform, its birth time, and make it alive.
	// Then inform the components that a new particle has been created.

	parts[newPart].xform = data.xform;
	parts[newPart].alive = true;
	parts[newPart].birthTime = sysTime;

	if (behave)
	{
		if (!behave->create_particle (newPart, data))
		{
			parts[newPart].alive = false;
		}
	}

	if (renderer)
	{
		if (!renderer->create_particle (newPart, data))
		{
			parts[newPart].alive = false;
		}
	}

	// All is well.
	++partCount;
	return true;
}

void BasicSystem::destroy_particle (unsigned int which)
{
	if (which >= maxParts)
	{
		GENERAL_WARNING("Attempting to destroy an invalid particle.\n");
		return;
	}

	// Inform all of the components that the particle is dying, then mark it as dead.

	if (behave)
	{
		behave->destroy_particle (which);
	}

	if (renderer)
	{
		renderer->destroy_particle (which);
	}

	parts[which].alive = false;
	--partCount;
}

void BasicSystem::set_transform (const Transform &_xform)
{
	baseXform = _xform;
}

const Transform & BasicSystem::get_transform()
{
	return baseXform;
}

void BasicSystem::allow_emission (bool allowed)
{
	emitActive = allowed;
}

void BasicSystem::start ()
{
	timeStopped = false;
}

void BasicSystem::stop ()
{
	timeStopped = true;
}

void BasicSystem::reset ()
{
	// Destroy all of the particles.
	for (unsigned int i = 0; i < maxParts; ++i)
	{
		if (parts[i].alive)
		{
			destroy_particle (i);
		}
	}

	// Set the time back to zero. The status of time starting or stopping is not changed.
	sysTime = 0.0;
}

unsigned int BasicSystem::get_particle_count ()
{
	return partCount;
}

unsigned int BasicSystem::get_max_particles ()
{
	return maxParts;
}

Particle &BasicSystem::get_particle(unsigned int which)
{
	if (which >= maxParts)
	{
		GENERAL_WARNING("Attempting access to an invalid particle. Returning particle 0.\n");
		which = 0;
	}

	return parts[which];
}

Particle *BasicSystem::get_particle_array ()
{
	return parts;
}

float BasicSystem::get_time ()
{
	return sysTime;
}

// Some particle behavior objects.

struct BState
{
	Vector v;        // velocity vector
	Vector p;        // position
	float  deathAge; // age at which this particle dies
};

class Ballistic : public IParticleBehavior
{
private:
	// The set of velocities for each particle
	BState *     part;     // starting state for each particle.
	unsigned int maxParts;

	// Gravity control
	Vector   g;

	// The plane equation of the floor.
	Vector   floorNormal;
	SINGLE   floorD;

	// Lifespan of particles
	float    minLife;
	float    maxLife;

	// The associated particle system.
	IParticleSystem *sys;

public:
	// Constructors and destructors
	Ballistic ();
	~Ballistic ();

	// Local methods
	void set_gravity (Vector _g)
	{
		g = _g;
	}

	void set_floor (Vector normal, Vector pointOnFloor)
	{
		floorNormal = normal;
		floorNormal.normalize();
		floorD = -dot_product (floorNormal, pointOnFloor);
	}

	void set_min_lifespan (float _minLife)
	{
		minLife = _minLife;
		if (maxLife < minLife)
		{
			maxLife = minLife;
		}
	}

	void set_max_lifespan (float _maxLife)
	{
		maxLife = _maxLife;
		if (minLife > maxLife)
		{
			minLife = maxLife;
		}
	}

	// ==== IParticleBehavior methods ====
	// Informs this behavior of its associated system. If system is NULL, it is being detached from a system.
	virtual bool set_system (IParticleSystem *system, unsigned int maxParticles);

	// Applies this behavior to the particles in the list.
	virtual bool update (float dt);
	
	// Informs this behavior that a new particle has been created and what its index is.
	virtual bool create_particle (unsigned int which, const ParticleStartData &data);

	// Informs this behavior that a particle has been destroyed.
	virtual void destroy_particle (unsigned int which);
};

Ballistic::Ballistic ()
{
	g.set (0, -9.8f, 0);
	part = NULL;
	sys = NULL;
	maxParts = 0;
	minLife = maxLife = -1.0;
}

Ballistic::~Ballistic ()
{
	if (sys != NULL)
	{
		GENERAL_ERROR("Attempt to destroy an IParticleBehavior still attached to a particle system.\n");
	}

	if (part)
	{
		delete [] part;
		part = NULL;
	}
}

// Informs this behavior of its associated system. If system is NULL, it is being detached from a system.
bool Ballistic::set_system (IParticleSystem *system, unsigned int maxParticles)
{
	if (system == NULL)
	{
		// We are being detached. Free any allocated resources.
		if (part != NULL)
		{
			delete [] part;
			part = NULL;
		}
	}
	else
	{
		if (sys == NULL)
		{
			// Allocate a new state array of the appropriate size
			part = new BState[maxParticles];
		}
		else
		{
			// If we are already attached, generate a warning and return failure.
			GENERAL_WARNING ("Attempt made to attach a particle system to an attached behavior.\n");
			return false;
		}
	}

	sys = system;
	maxParts = maxParticles;
	return true;
}

// Applies this behavior to the particles in the list. Returns false if there are no active particles.
bool Ballistic::update (float dt)
{
	bool result = false;
	if (sys)
	{
		// Calculate the new position for each live particle, detecting and resolving collisions with the floor.
		float now = sys->get_time ();
		Vector halfAccel = g * 0.5;
		Particle *sysParts = sys->get_particle_array ();
		for (unsigned int i = 0; i < maxParts; ++i)
		{
			Particle &p = sysParts[i];
			if (p.alive && now != p.birthTime)
			{
				float age = now - p.birthTime;

				if (part[i].deathAge >= 0.0 && age >= part[i].deathAge)
				{
					// Kill the particle if its age has exceeded its deathAge.
					sys->destroy_particle (i);
				}
				else
				{
					// Calculate where the particle should be based in its birthTime, the current time, its
					// starting velocity, and its starting position. The delta is not used here.
					Vector newPos = part[i].p + (part[i].v + halfAccel * age) * age;

					// Check the new position for collision with the floor by evaluating the position in the
					// plane equation. If the result is positive, all is well. If it is negative, the particle
					// is in the floor. If it is zero, it is on the floor.
					// Any particle on or below the floor is destroyed, all others are set
					// *** TODO: Make the particles bounce.

					if (dot_product (floorNormal, newPos) + floorD > 0.0f)
					{
						p.xform.set_position (newPos);
					}
					else
					{
						sys->destroy_particle (i);
					}
				}
			}
			else
			{
				p.xform.set_position (part[i].p);
			}
		}
		result = (sys->get_particle_count () > 0);
	}
	return result;
}

// Informs this behavior that a new particle has been created and what its index is.
bool Ballistic::create_particle (unsigned int which, const ParticleStartData &data)
{
	if (which >= maxParts)
	{
		GENERAL_WARNING("Attempting to create particle out of range.\n");
		return false; 
	}

	// Store the starting transform and linear velocity for later use.
	part[which].p = data.xform.get_position();
	part[which].v = data.vel;
	part[which].deathAge = crand (minLife, maxLife);

	return true;
}

// Informs this behavior that a particle has been destroyed.
void Ballistic::destroy_particle (unsigned int which)
{
	// Nothing to do here.
}

// Some particle render objects

// Renders textures as a billboard, i.e. a triangle always facing the viewer.
// The triangle is still drawn in 3D, so it will be scaled with its distance from the camera.
// The controllable parameters are:
// 1) Min and Max radius
// 2) Texture
// 3) Texture animation rate

struct BillboardPart
{
	float                radius;   // radius of bounding circle for the world space triangle for this particle
	ITL_TEXTURE_REF_ID   txRefId;  // the texture reference for this particle.
//	ITL_TEXTUREFRAME_IRP fdata;    // frame data for each particle
};

class Billboard : public IParticleRenderer
{
private:
	BillboardPart *         part;      // the render information for the particles
	unsigned int            maxParts;  // length of parts[]

	// Control parameters for this renderer
	COMPTR<ITextureLibrary> txLib;
	ITL_TEXTURE_ID          txId;      // the id of the texture to use for the particles
	ITL_PLAYCOMMAND	        txPlayCmd; // how to animate the texture
	float                   minRad;    // minimum radius.
	float                   maxRad;    // maximum radius.

	// The associated particle system.
	IParticleSystem *       sys;

	// Members used to optimize rendering
	RPVertex1 *             vert;      // vert[6*maxParts], two triangles per particle.

protected:
	void free_parts ();

public:
	// Constructors and destructors
	Billboard (ITextureLibrary *_txLib);
	~Billboard ();

	// Parameters for this renderer
	void set_texture (ITL_TEXTURE_ID _txId);
	void set_radius_range (float _minRad, float _maxRad);

	// ==== IParticleRenderer methods ====
	// Informs this renderer of its associated system. If system is NULL, it is being detached from a system.
	virtual bool set_system (IParticleSystem *system, unsigned int maxParticles);

	// Renders the particles of the associated system using the given pipeline, camera, and transform.
	virtual void render (IRenderPrimitive *prim, ICamera *cam, const Transform &xform);
	
	// Updates the render information for all of the particles
	virtual void update (float dt);

	// Informs this renderer that a new particle has been created and what its index is.
	virtual bool create_particle (unsigned int which, const ParticleStartData &data);

	// Informs this renderer that a particle has been destroyed.
	virtual void destroy_particle (unsigned int which);
};

Billboard::Billboard (ITextureLibrary *_txLib)
{
	ASSERT(_txLib != NULL);

	part = NULL;
	maxParts = 0;

	txLib = _txLib;
	txId = ITL_INVALID_ID;
	txPlayCmd = ITL_PLAY_ONE_TIME;

	minRad = maxRad = 0.0f;

	sys = NULL;
}

Billboard::~Billboard ()
{
	if (sys != NULL)
	{
		GENERAL_ERROR("Attempt to destroy an IParticleRenderer still attached to a particle system.\n");
	}

	free_parts();
}

void Billboard::set_texture (ITL_TEXTURE_ID _txId)
{
	txId = _txId;
}

void Billboard::set_radius_range (float _minRad, float _maxRad)
{
	ASSERT(_minRad <= _maxRad);
	minRad = _minRad;
	maxRad = _maxRad;
}

void Billboard::free_parts ()
{
	if (part)
	{
		// Run through the parts, releasing all of our references on valid particles.
		for (unsigned int i = 0; i < maxParts; ++i)
		{
			if (part[i].txRefId != ITL_INVALID_REF_ID)
			{
				txLib->release_texture_ref (part[i].txRefId);
				part[i].txRefId = ITL_INVALID_REF_ID;
			}
		}

		// Delete the part array
		delete [] part;
		part = NULL;
	}
}

bool Billboard::set_system (IParticleSystem *system, unsigned int maxParticles)
{
	if (system == NULL)
	{
		// We are being detached. Free any allocated resources.
		free_parts();
	}
	else
	{
		if (sys == NULL)
		{
			// Allocate a new state array of the appropriate size
			part = new BillboardPart[maxParticles];
			for (unsigned int i = 0; i < maxParticles; ++i)
			{
				part[i].txRefId = ITL_INVALID_REF_ID;
			}
			vert = new RPVertex1[maxParticles * 6];
		}
		else
		{
			// If we are already attached, generate a warning and return failure.
			GENERAL_WARNING ("Attempt made to attach a particle system to an attached renderer.\n");
			return false;
		}
	}

	sys = system;
	maxParts = maxParticles;
	return true;
}

void Billboard::render (IRenderPrimitive *prim, ICamera *cam, const Transform &xform)
{
	// *** This should be optimized to combine all particles that are using the same render primline
	// *** texture id. For now, we will simply call render primitive for each particle!

	ASSERT(prim != NULL);
	ASSERT(cam != NULL);

	// NOTE: It is assumed that the model view matrix has already been set up for the given camera.
	// The camera is passed in here so that the renderer can skip particles outside of the render volume.

	// NOTE2: The given xform is ignored by this renderer. The particles are expected to be in world space
	// by the time this renderer is called.

	// Render each particle. Each particle is rendered as a quad

#if 0
	const Vector v[3] =
	{
		Vector ( 0.0,    1.0, 0.0),
		Vector (-0.866, -0.5, 0.0),
		Vector ( 0.866, -0.5, 0.0)
	};

	const Vector v0( 0.0,    1.0, 0.0);
	const Vector v1( 0.866, -0.5, 0.0);
	const Vector v2(-0.866, -0.5, 0.0);
#endif

	// Get the camera i and j vectors.
	const Transform &xfcam = cam->get_transform();
	const Vector camI = xfcam.get_i();
	const Vector camJ = xfcam.get_j();

	prim->set_texture_stage_state( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
	prim->set_texture_stage_state( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );

	prim->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	prim->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	prim->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	prim->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	prim->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	prim->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	prim->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0 );
	
	prim->set_render_state (D3DRS_ALPHABLENDENABLE, TRUE);
	prim->set_render_state (D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	prim->set_render_state (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	Particle *sysParts = sys->get_particle_array ();
#if 0
	RPVertex1 quad[4];

	for (unsigned int i = 0; i < maxParts; ++i)
	{
		Particle &p = sysParts[i];
		if (p.alive)
		{
			// Get the frame information for the current frame of this particle.
			BillboardPart &rp = part[i];

			ITL_TEXTUREFRAME_IRP fdata;
			if (txLib->get_texture_ref_frame (rp.txRefId, ITL_FRAME_CURRENT, &fdata) == GR_OK)
			{
				// Calculate the coordinates, in world space, of the billboard vertices, facing the camera.
				const Vector &pos = p.xform.get_position ();
				quad[0].pos = pos - camI * rp.radius + camJ * rp.radius;
				quad[0].color = 0xFFFFFFFF;  // white, opaque.
				quad[0].u = fdata.u0;
				quad[0].v = fdata.v0;

				quad[1].pos = pos + camI * rp.radius + camJ * rp.radius;
				quad[1].color = 0xFFFFFFFF;  // white, opaque
				quad[1].u = fdata.u1;
				quad[1].v = fdata.v0;

				quad[2].pos = pos + camI * rp.radius - camJ * rp.radius;
				quad[2].color = 0xFFFFFFFF;  // white, opaque
				quad[2].u = fdata.u1;
				quad[2].v = fdata.v1;

				quad[3].pos = pos - camI * rp.radius - camJ * rp.radius;
				quad[3].color = 0xFFFFFFFF;  // white, opaque
				quad[3].u = fdata.u0;
				quad[3].v = fdata.v1;

				// Set the texture.
				if (prim->set_texture_stage_texture(0, fdata.rp_texture_id) == GR_OK)
				{
					// Draw a triangle fan.
					if (prim->draw_primitive (D3DPT_TRIANGLEFAN, D3DFVF_RPVERTEX, &quad, 4, 0) != GR_OK)
					{
						GENERAL_WARNING("Failed to render particle fan!\n");
					}
				}
			}
		}
	}
#else
	bool textureSet = false;
	ITL_TEXTUREFRAME_IRP fdata;
	int vertCount = 0;
	for (unsigned int i = 0; i < maxParts; ++i)
	{
		Particle &p = sysParts[i];
		if (p.alive)
		{
			BillboardPart &rp = part[i];

			// Set the texture to the frame of the first active particle.
			if (!textureSet)
			{
				if (txLib->get_texture_ref_frame (rp.txRefId, ITL_FRAME_CURRENT, &fdata) == GR_OK)
				{
					if (prim->set_texture_stage_texture(0, fdata.rp_texture_id) == GR_OK)
					{
						textureSet = true;
					}
				}
			}

			// Calculate the coordinates, in world space, of the billboard vertices, facing the camera.
			const Vector &pos = p.xform.get_position ();

			// 1st triangle
			vert[vertCount].pos = pos - camI * rp.radius + camJ * rp.radius;
			vert[vertCount].color = 0xFFFFFFFF;  // white, opaque.
			vert[vertCount].u = fdata.u0;
			vert[vertCount].v = fdata.v0;

			vert[vertCount+1].pos = pos + camI * rp.radius + camJ * rp.radius;
			vert[vertCount+1].color = 0xFFFFFFFF;  // white, opaque
			vert[vertCount+1].u = fdata.u1;
			vert[vertCount+1].v = fdata.v0;

			vert[vertCount+2].pos = pos + camI * rp.radius - camJ * rp.radius;
			vert[vertCount+2].color = 0xFFFFFFFF;  // white, opaque
			vert[vertCount+2].u = fdata.u1;
			vert[vertCount+2].v = fdata.v1;

			// 2nd triangle
			vert[vertCount+3] = vert[vertCount];
			vert[vertCount+4] = vert[vertCount+2];

			vert[vertCount+5].pos = pos - camI * rp.radius - camJ * rp.radius;
			vert[vertCount+5].color = 0xFFFFFFFF;  // white, opaque
			vert[vertCount+5].u = fdata.u0;
			vert[vertCount+5].v = fdata.v1;

			vertCount += 6;
		}
	}

	// If there are any vertices to draw, draw them
	if (vertCount > 0)
	{
		if (prim->draw_primitive (D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, vert, vertCount, 0) != GR_OK)
		{
			GENERAL_WARNING("Failed to render particle list!\n");
		}
	}
#endif

	prim->flush ( RPR_OPAQUE | RPR_TRANSLUCENT_DEPTH_SORTED );
}

void Billboard::update (float dt)
{
	// *** Do we need to do anything here? We could calculate the particle vertices here.
}

bool Billboard::create_particle (unsigned int which, const ParticleStartData &data)
{
	if (which >= maxParts)
	{
		GENERAL_WARNING("Attempting to create particle out of range.\n");
		return false; 
	}

	// Get a new texture reference for the particle, setting its current time to 0
	// and its play mode to that specified.
	if (txLib->add_ref_texture_id (txId, &part[which].txRefId) != GR_OK)
	{
		GENERAL_WARNING("Failed to create a texture reference.\n");
		return false;
	}
	else
	{
		txLib->set_texture_ref_frame_time (part[which].txRefId, 0.0f);
		txLib->set_texture_ref_play_mode(part[which].txRefId, txPlayCmd);
	}

	// Calculate a random radius for this particle within the proscribed range.
	part[which].radius = crand (minRad, maxRad);

	return true;
}

void Billboard::destroy_particle (unsigned int which)
{
	if (which >= maxParts)
	{
		GENERAL_WARNING("Attempting to destroy a particle out of range.\n");
		return; 
	}

	// Release the texture reference for this particle, if valid.
	if (part[which].txRefId != ITL_INVALID_REF_ID)
	{
		txLib->release_texture_ref (part[which].txRefId);
		part[which].txRefId = ITL_INVALID_REF_ID;
	}
}

// Renders an engine instance as a particle.

class EngRender : public IParticleRenderer
{
private:
	INSTANCE_INDEX *        part;      // the render information for the particles
	unsigned int            maxParts;  // length of parts[]

	// Control parameters for this renderer
	COMPTR<IEngine>         eng;       // the engine to use.
	ARCHETYPE_INDEX         arch;      // the archetype to use for particles
	float                   minScale;  // minimum scale.
	float                   maxScale;  // maximum scale.

	// The associated particle system.
	IParticleSystem *sys;

protected:
	void free_parts ();

public:
	// Constructors and destructors
	EngRender (IEngine *_eng);
	~EngRender ();

	// Parameters for this renderer
	void set_archetype (const char *archName);
	void set_min_scale (float _minScale)
	{
		minScale = _minScale;
	}

	void set_max_scale (float _maxScale)
	{
		maxScale = _maxScale;
	}

	// ==== IParticleRenderer methods ====
	// Informs this renderer of its associated system. If system is NULL, it is being detached from a system.
	virtual bool set_system (IParticleSystem *system, unsigned int maxParticles);

	// Renders the particles of the associated system using the given pipeline, camera, and transform.
	virtual void render (IRenderPrimitive *prim, ICamera *cam, const Transform &xform);
	
	// Updates the render information for all of the particles
	virtual void update (float dt);

	// Informs this renderer that a new particle has been created and what its index is.
	virtual bool create_particle (unsigned int which, const ParticleStartData &data);

	// Informs this renderer that a particle has been destroyed.
	virtual void destroy_particle (unsigned int which);
};

EngRender::EngRender (IEngine *_eng)
{
	ASSERT(_eng != NULL);

	part = NULL;
	maxParts = 0;

	eng = _eng;
	arch = INVALID_ARCHETYPE_INDEX;
	minScale = 1.0;
	maxScale = 1.0;
	sys = NULL;
}

EngRender::~EngRender ()
{
	if (sys != NULL)
	{
		GENERAL_ERROR("Attempt to destroy an EngRender still attached to a particle system.\n");
	}

	free_parts ();

	if (arch != INVALID_ARCHETYPE_INDEX)
	{
		eng->release_archetype (arch);
		arch = INVALID_ARCHETYPE_INDEX;
	}
}

void EngRender::set_archetype (const char *archName)
{
	// If we have a valid archetype, release it. If there are any instances of the archetype
	// around, this will just decrement the archetype count, otherwise the archetype will be
	// destroyed. We don't care here.

	if (arch != INVALID_ARCHETYPE_INDEX)
	{
		eng->release_archetype (arch);
		arch = INVALID_ARCHETYPE_INDEX;
	}

	// By convention, the name of the archetype used will be the name of its .3db or .cmp file.
	// Therefore, we can check to see if the archetype exists. If it does, we will just use the existing
	// instance, otherwise we will load the archetype here.

	arch = eng->get_archetype_by_name (archName);
	if (arch == INVALID_ARCHETYPE_INDEX)
	{
		// We need to load a new archetype. Use the archetype name as a filename.
		COMPTR<IFileSystem> fs;

		if (eng->create_file_system (archName, fs) == GR_OK)
		{
			TLIB->load_library(fs, NULL);
			arch = eng->create_archetype (archName, fs);
			ASSERT (arch != INVALID_ARCHETYPE_INDEX);
		}
	}
	else
	{
		eng->hold_archetype (arch);
	}

	// We now have an archetype index and can create instances.
}

void EngRender::free_parts ()
{
	if (part)
	{
		// Run through the parts, releasing all of our references on valid particles.
		for (unsigned int i = 0; i < maxParts; ++i)
		{
			if (part[i] != INVALID_INSTANCE_INDEX)
			{
				eng->destroy_instance (part[i]);
				part[i] = INVALID_INSTANCE_INDEX;
			}
		}

		// Delete the part array
		delete [] part;
		part = NULL;
	}
}

bool EngRender::set_system (IParticleSystem *system, unsigned int maxParticles)
{
	if (system == NULL)
	{
		// We are being detached. Free any allocated resources.
		free_parts ();
	}
	else
	{
		if (sys == NULL)
		{
			// Allocate a new state array of the appropriate size
			part = new INSTANCE_INDEX[maxParticles];
			for (unsigned int i = 0; i < maxParts; ++i)
			{
				part[i] = INVALID_INSTANCE_INDEX;
			}
		}
		else
		{
			// If we are already attached, generate a warning and return failure.
			GENERAL_WARNING ("Attempt made to attach a particle system to an attached renderer.\n");
			return false;
		}
	}

	sys = system;
	maxParts = maxParticles;
	return true;
}

void EngRender::render (IRenderPrimitive *prim, ICamera *cam, const Transform &xform)
{
	ASSERT(cam != NULL);

	// NOTE: It is assumed that the model view matrix has already been set up for the given camera.
	// The camera is passed in here so that the renderer can skip particles outside of the render volume.

	// NOTE2: The given xform is ignored by this renderer. The particles are expected to be in world space
	// by the time this renderer is called.

	// Render each particle by calling render_instance() on it with the given camera.

	Particle *sysParts = sys->get_particle_array ();
	for (unsigned int i = 0; i < maxParts; ++i)
	{
		Particle &p = sysParts[i];
		if (p.alive)
		{
			eng->render_instance (cam, part[i], 0, 1.0, RF_RELATIVE_LOD, NULL);
		}
	}
}

void EngRender::update (float dt)
{
	// Update all of the valid instances by moving them to the current position of their particle.
	Particle *sysParts = sys->get_particle_array ();
	for (unsigned int i = 0; i < maxParts; ++i)
	{
		Particle &p = sysParts[i];
		if (p.alive)
		{
			eng->set_transform (part[i], p.xform);
			eng->update_instance (part[i], dt, 0);
		}
	}
}

bool EngRender::create_particle (unsigned int which, const ParticleStartData &data)
{
	if (which >= maxParts)
	{
		GENERAL_WARNING("Attempting to create particle out of range.\n");
		return false; 
	}

	if (arch == INVALID_ARCHETYPE_INDEX)
	{
		GENERAL_WARNING("Attempting to create particle without archetype.\n");
		return false;
	}

	// Create a new instance of the archetype, storing the instance in the particle array.
	part[which] = eng->create_instance2 (arch, NULL);

	// NOTE: The scale is not currently used.
	return part[which] != INVALID_INSTANCE_INDEX;
}

void EngRender::destroy_particle (unsigned int which)
{
	if (which >= maxParts)
	{
		GENERAL_WARNING("Attempting to destroy a particle out of range.\n");
		return; 
	}

	// Release the texture reference for this particle, if valid.
	if (part[which] != INVALID_INSTANCE_INDEX)
	{
		eng->destroy_instance (part[which]);
		part[which] = INVALID_INSTANCE_INDEX;
	}
}

// Some particle emitter objects

class NozzleEmitter : public IParticleEmitter
{
private:
	Vector    pos;       // position of the nozzle in the associated system's coordinate frame
	Vector    dir;       // direction of the nozzle in the associated system's coordinate frame
	SINGLE    angle;     // angle of the nozzle spray, in degrees.
	SINGLE    minVel;    // minimum emission velocity
	SINGLE    maxVel;    // maximum emission velocity
	SINGLE    rate;      // the rate of particles generated, in particles/second.
	SINGLE    rateRecip; // the reciprocal of the rate: 1.0/rate, i.e. seconds per particle.
	float     timeAccum; // accumulated time since the last particle was emitted.

	IParticleSystem *sys;

public:
	NozzleEmitter ();
	~NozzleEmitter ();

	// Parameter methods
	void set_pos (const Vector &_pos)
	{
		pos = _pos;
	}

	void set_dir (const Vector &_dir)
	{
		dir = _dir;
		dir.normalize();
	}

	void set_angle (SINGLE _angle)
	{
		angle = _angle;
	}

	void set_rate (SINGLE _rate)
	{
		if (rate > 0.0f)
		{
			rate = _rate;
			rateRecip = 1.0f/rate;
		}
	}

	void set_min_vel (SINGLE _minVel)
	{
		minVel = _minVel;
	}

	void set_max_vel (SINGLE _maxVel)
	{
		maxVel = _maxVel;
	}

	// ==== IParticleEmitter methods ====
	// Informs this emitter of its associated system. If system is NULL, it is being detached from a system.
	virtual bool set_system (IParticleSystem *system, unsigned int maxParticles);

	// Emits particles when required
	virtual void update (float dt);
};

NozzleEmitter::NozzleEmitter ()
{
	dir.set (0, 0, 1);
	pos.set (0, 0, 0);
	minVel = 1.0;
	maxVel = 1.0;
	angle = 30;  // degrees
	rate = 60;
	timeAccum = 0;
	rateRecip = 1.0/rate;
	sys = NULL;
}

NozzleEmitter::~NozzleEmitter ()
{
	if (sys != NULL)
	{
		GENERAL_ERROR("Attempt to destroy an IParticleBehavior still attached to a particle system.\n");
	}
}

bool NozzleEmitter::set_system (IParticleSystem *system, unsigned int maxParticles)
{
	if (system == NULL)
	{
		// We are being detached. Free any allocated resources.
	}
	else
	{
		if (sys == NULL)
		{
			// We are being attached. Allocate any needed resources.
		}
		else
		{
			// If we are already attached, generate a warning and return failure.
			GENERAL_WARNING ("Attempt made to attach a particle system to an attached emitter.\n");
			return false;
		}
	}

	sys = system;
	return true;
}

void NozzleEmitter::update (float dt)
{
	// Add the delta into the accumulator, then count down in rateRecip steps, generating particles
	// as we go.

	if (sys)
	{
		timeAccum += dt;
		while (timeAccum > rateRecip)
		{
//			if (sys->get_particle_count() == 0)
			{
				// Get a random vector in the cone.
				Vector emitDir = cone_vrand (dir, angle);
				// Hardwired positive Y velocity.
				ParticleStartData data;
				data.avel.set (0,0,0);
				data.vel = emitDir * crand (minVel, maxVel);
				data.xform.set_identity();
				data.xform.set_position (sys->get_transform () * pos); // position in world space.

				sys->create_particle (data);
			}

			timeAccum -= rateRecip;
		}
	}
}

//
// Script exposure stuff
//

#include <lua.h>

// Routines from renderwin.cpp
extern Vector *get_vector(lua_Object obj);
extern void add_psys (IParticleSystem *_psys);
extern void del_psys (IParticleSystem *_psys);

// Desc types

const int PSYS_BASIC = 1;

const int PEMIT_NOZZLE = 1;

const int PBEHAVE_BALLISTIC = 1;

const int PRENDER_BILLBOARD = 1;
const int PRENDER_OBJECT = 2;

static int psysTag = LUA_ANYTAG;
static int psysTableTag = LUA_ANYTAG;

IParticleEmitter *createEmitter (lua_Object desc)
{
	// Attempt to retrieve the type value from the desc. Return NULL on failure.

	IParticleEmitter *result = NULL;

	lua_pushobject (desc);
	lua_pushstring ("type");
	lua_Object oEmitType = lua_gettable ();
	if (oEmitType == LUA_NOOBJECT)
	{
		return NULL;
	}

	if (!lua_isnumber (oEmitType))
	{
		return NULL;
	}

	int type = lua_getnumber (oEmitType);
	switch (type)
	{
	case PEMIT_NOZZLE:
		{
			NozzleEmitter * ne = new NozzleEmitter ();
			lua_Object oParam;

			lua_pushobject (desc);
			lua_pushstring ("pos");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				Vector *v = get_vector (oParam);
				if (v != NULL)
				{
					ne->set_pos (*v);
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("dir");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				Vector *v = get_vector (oParam);
				if (v != NULL)
				{
					ne->set_dir (*v);
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("angle");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					ne->set_angle (lua_getnumber (oParam));
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("rate");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					ne->set_rate (lua_getnumber (oParam));
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("minVel");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					ne->set_min_vel (lua_getnumber (oParam));
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("maxVel");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					ne->set_max_vel (lua_getnumber (oParam));
				}
			}

			result = ne;
		}
		break;
	}

	return result;
}

IParticleBehavior *createBehavior (lua_Object desc)
{
	// Attempt to retrieve the type value from the desc. Return NULL on failure.

	IParticleBehavior *result = NULL;

	lua_pushobject (desc);
	lua_pushstring ("type");
	lua_Object oBehaveType = lua_gettable ();
	if (oBehaveType == LUA_NOOBJECT)
	{
		return NULL;
	}

	if (!lua_isnumber (oBehaveType))
	{
		return NULL;
	}

	int type = lua_getnumber (oBehaveType);
	switch (type)
	{
	case PBEHAVE_BALLISTIC:
		{
			Ballistic *nb = new Ballistic ();
			lua_Object oParam;

			lua_pushobject (desc);
			lua_pushstring ("gravity");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				Vector *v = get_vector (oParam);
				if (v != NULL)
				{
					nb->set_gravity (*v);
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("floorNormal");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				Vector *v = get_vector (oParam);
				if (v != NULL)
				{
					Vector fNormal = *v;
					lua_pushobject (desc);
					lua_pushstring ("floorPoint");
					oParam = lua_gettable ();
					if (oParam != LUA_NOOBJECT)
					{
						v = get_vector (oParam);
						if (v != NULL)
						{
							nb->set_floor (fNormal, *v);
						}
					}
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("minLife");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					nb->set_min_lifespan (lua_getnumber (oParam));
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("maxLife");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					nb->set_max_lifespan (lua_getnumber (oParam));
				}
			}

			result = nb;
		}
		break;
	}

	return result;
}

IParticleRenderer *createRenderer (lua_Object desc)
{
	// Attempt to retrieve the type value from the desc. Return NULL on failure.

	IParticleRenderer *result = NULL;

	lua_pushobject (desc);
	lua_pushstring ("type");
	lua_Object oRenderType = lua_gettable ();
	if (oRenderType == LUA_NOOBJECT)
	{
		return NULL;
	}

	if (!lua_isnumber (oRenderType))
	{
		return NULL;
	}

	int type = lua_getnumber (oRenderType);
	switch (type)
	{
	case PRENDER_BILLBOARD:
		{
			Billboard *nb = new Billboard (TLIB);
			lua_Object oParam;

			lua_pushobject (desc);
			lua_pushstring ("texture");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isstring(oParam))
				{
					ITL_TEXTURE_ID tid;
					if (TLIB->get_texture_id (lua_getstring (oParam), &tid) == GR_OK)
					{
						nb->set_texture (tid);
					}
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("minRad");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					float minRad = lua_getnumber(oParam);
					lua_pushobject (desc);
					lua_pushstring ("maxRad");
					oParam = lua_gettable ();
					if (oParam != LUA_NOOBJECT)
					{
						if (lua_isnumber (oParam))
						{
							float maxRad = lua_getnumber(oParam);
							nb->set_radius_range (minRad, maxRad);
						}
					}
				}
			}

			result = nb;
		}
		break;

	case PRENDER_OBJECT:
		{
			EngRender *nb = new EngRender (ENGINE);
			lua_Object oParam;

			lua_pushobject (desc);
			lua_pushstring ("objFile");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isstring(oParam))
				{
					nb->set_archetype (lua_getstring (oParam));
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("minScale");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					nb->set_min_scale (lua_getnumber(oParam));
				}
			}

			lua_pushobject (desc);
			lua_pushstring ("maxScale");
			oParam = lua_gettable ();
			if (oParam != LUA_NOOBJECT)
			{
				if (lua_isnumber (oParam))
				{
					nb->set_max_scale (lua_getnumber(oParam));
				}
			}

			result = nb;
		}
		break;
	}

	return result;
}

const int DEFAULT_MAX_PARTICLES = 32;

IParticleSystem *get_psys (lua_Object oSys)
{
	IParticleSystem *sys = NULL;
	if (oSys != LUA_NOOBJECT)
	{
		if (lua_isuserdata (oSys) && lua_tag(oSys) == psysTag)
		{
			sys = (IParticleSystem *) lua_getuserdata (oSys);
		}
	}
	return sys;
}

void setParticleSystemPosition ()
{
	// C-closure, param1 is a userdata for the this pointer
	//            param2 is the vector position to set

	lua_Object oSys = lua_getparam(1);
	lua_Object oPos = lua_getparam(2);

	IParticleSystem *sys = get_psys (oSys);
	if (!sys)
	{
		return;
	}

	Vector *v = get_vector (oPos);
	if (!v)
	{
		return;
	}

	Transform t = sys->get_transform ();
	t.set_position (*v);
	sys->set_transform (t);
}

void setParticleSystemEmission ()
{
	// C-closure, param1 is a userdata for the this pointer
	//            param2 is boolean on=true or off=false

	lua_Object oSys = lua_getparam(1);
	lua_Object oEmit = lua_getparam(2);

	IParticleSystem *sys = get_psys (oSys);
	if (!sys)
	{
		return;
	}

	if (oEmit == LUA_NOOBJECT)
	{
		return;
	}

	if (!lua_isnumber (oEmit))
	{
		return;
	}

	bool emitAllowed = (lua_getnumber(oEmit) != 0);

	sys->allow_emission (emitAllowed);
}

void startParticleSystem ()
{
	// C-closure, param1 is a userdata for the this pointer

	lua_Object oSys = lua_getparam(1);

	IParticleSystem *sys = get_psys (oSys);
	if (!sys)
	{
		return;
	}

	sys->start ();
}

void stopParticleSystem ()
{
	// C-closure, param1 is a userdata for the this pointer

	lua_Object oSys = lua_getparam(1);

	IParticleSystem *sys = get_psys (oSys);
	if (!sys)
	{
		return;
	}

	sys->start ();
}

void resetParticleSystem ()
{
	// C-closure, param1 is a userdata for the this pointer

	lua_Object oSys = lua_getparam(1);

	IParticleSystem *sys = get_psys (oSys);
	if (!sys)
	{
		return;
	}

	sys->start ();
}

void createParticleSystem ()
{
	// Load the particle texture library the first time we attempt to create a system.
	static bool initted = false;
	if (!initted)
	{
		// Load the particle texture map

		COMPTR<IFileSystem> IFS = NULL;

		if( ENGINE->create_file_system( "ptex.txm", IFS ) == GR_OK ) {
			TLIB->load_library( IFS, NULL );
		}
		initted = true;
	}

	// This is a function exposed to lua.
	// Its syntax is:
	//    CreateParticleSystem (table emitterDesc, table behaviorDesc, table rendererDesc);
	// The emitterDesc, behaviorDesc, and rendererDesc are all tables with at least one field set, a number named
	// "type". This field indicates the kind of object to create, as well as the syntax for the rest of the table.
	// All paramters for a particle component have defaults, so the table need contain nothing other than type.

	lua_Object oEmitDesc = lua_getparam(1);
	lua_Object oBehaveDesc = lua_getparam(2);
	lua_Object oRenderDesc = lua_getparam(3);
	lua_Object oMaxParticles = lua_getparam(4);

	if (oEmitDesc == LUA_NOOBJECT || oBehaveDesc == LUA_NOOBJECT || oRenderDesc == LUA_NOOBJECT)
	{
		return;
	}

	if (!lua_istable (oEmitDesc) || !lua_istable (oBehaveDesc) || !lua_istable (oRenderDesc))
	{
		return;
	}

	int maxParts = DEFAULT_MAX_PARTICLES;
	if (oMaxParticles != LUA_NOOBJECT)
	{
		if (lua_isnumber (oMaxParticles))
		{
			maxParts = lua_getnumber (oMaxParticles);
			if (maxParts <= 0)
			{
				return;
			}
		}
	}

	IParticleEmitter *emit = createEmitter (oEmitDesc);
	IParticleBehavior *behave = createBehavior (oBehaveDesc);
	IParticleRenderer *renderer = createRenderer (oRenderDesc);

	if (emit && behave && renderer)
	{
		BasicSystem *sys = new BasicSystem;

		if (sys->init (maxParts, behave, renderer, emit))
		{
			// Create a table and store the system and component pointers into it
			lua_Object sysTable = lua_createtable ();
			lua_pushobject (sysTable);
			lua_pushstring ("_sys");
			lua_pushuserdata (sys);
			lua_settable ();

			lua_pushobject (sysTable);
			lua_pushstring ("_emit");
			lua_pushuserdata (emit);
			lua_settable ();

			lua_pushobject (sysTable);
			lua_pushstring ("_behave");
			lua_pushuserdata (behave);
			lua_settable ();

			lua_pushobject (sysTable);
			lua_pushstring ("_renderer");
			lua_pushuserdata (renderer);
			lua_settable ();

			// Add some c-closures to the table for controlling the particle system
			lua_pushobject (sysTable);
			lua_pushstring ("set_position");
			lua_pushusertag (sys, psysTag);
			lua_pushcclosure (setParticleSystemPosition, 1);
			lua_settable ();

			lua_pushobject (sysTable);
			lua_pushstring ("set_emission");
			lua_pushusertag (sys, psysTag);
			lua_pushcclosure (setParticleSystemEmission, 1);
			lua_settable ();

			lua_pushobject (sysTable);
			lua_pushstring ("start");
			lua_pushusertag (sys, psysTag);
			lua_pushcclosure (startParticleSystem, 1);
			lua_settable ();

			lua_pushobject (sysTable);
			lua_pushstring ("stop");
			lua_pushusertag (sys, psysTag);
			lua_pushcclosure (stopParticleSystem, 1);
			lua_settable ();

			lua_pushobject (sysTable);
			lua_pushstring ("reset");
			lua_pushusertag (sys, psysTag);
			lua_pushcclosure (resetParticleSystem, 1);
			lua_settable ();

			// Set the tag for the particle system table
			lua_pushobject (sysTable);
			lua_settag (psysTableTag);

			// Return the table
			lua_pushobject (sysTable);

			// Add the particle system to the render list.
			add_psys (sys);
		}
	}
}

void destroyParticleSystem ()
{
	// Syntax: DestroyParticleSystem (ParticleSystemTable psysTable)

	lua_Object oPsysTable = lua_getparam(1);

	if (oPsysTable == LUA_NOOBJECT)
	{
		return;
	}

	if (!lua_istable(oPsysTable) || lua_tag (oPsysTable) != psysTableTag)
	{
		return;
	}

	lua_pushobject (oPsysTable);
	lua_pushstring ("_sys");
	lua_Object oSys = lua_gettable ();

	lua_pushobject (oPsysTable);
	lua_pushstring ("_emit");
	lua_Object oEmit = lua_gettable ();

	lua_pushobject (oPsysTable);
	lua_pushstring ("_behave");
	lua_Object oBehave = lua_gettable ();

	lua_pushobject (oPsysTable);
	lua_pushstring ("_renderer");
	lua_Object oRender = lua_gettable ();

	IParticleSystem *sys = (IParticleSystem *) lua_getuserdata (oSys);
	IParticleEmitter *emit = (IParticleEmitter *) lua_getuserdata (oEmit);
	IParticleBehavior *behave = (IParticleBehavior *) lua_getuserdata (oBehave);
	IParticleRenderer *renderer = (IParticleRenderer *) lua_getuserdata (oRender);

	// Remove the particle system from the render list
	del_psys (sys);

	// Shut down the particle system
	sys->shutdown ();
	delete emit;
	delete behave;
	delete renderer;
	delete sys;
}

//
// Exported functions
//

bool init_particles ()
{
	// Create an IViewConstructor and initialize it with the particle system related data structures
	DACOMDESC vdesc = "IViewConstructor";
	if (DACOM->CreateInstance (&vdesc, (void **) &PARSER) != GR_OK)
	{
		return false;
	}

	PARSER->ParseMemory
	(
		"struct NozzleParams { float angle; float rate; float minVel; float maxVel; };\n"
	);

	// Initialize particle scripting

	psysTag = lua_newtag();
	psysTableTag = lua_newtag();

	lua_pushnumber (PSYS_BASIC);
	lua_setglobal ("BASIC");
	lua_pushnumber (PEMIT_NOZZLE);
	lua_setglobal ("NOZZLE");
	lua_pushnumber (PBEHAVE_BALLISTIC);
	lua_setglobal ("BALLISTIC");
	lua_pushnumber (PRENDER_BILLBOARD);
	lua_setglobal ("BILLBOARD");
	lua_pushnumber (PRENDER_OBJECT);
	lua_setglobal ("OBJECTPARTICLE");

	lua_pushcfunction (createParticleSystem);
	lua_setglobal ("CreateParticleSystem");
	lua_pushcfunction (destroyParticleSystem);
	lua_setglobal ("DestroyParticleSystem");

	return true;
}

