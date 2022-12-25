#ifndef __EVENTDEF_H__
#define __EVENTDEF_H__

#ifndef _VECTOR4_H_
#include "vector4.h"
#endif

#define NUM_COLOR_KEYS 32

struct EventDef
{
    SINGLE      alpha;              // how transparent 0 = clear, 255 = opaque
    SINGLE      alphaDecay;         // increase in transparency over time
    SINGLE      alphaRandom;        // randomness of alpha
    SINGLE      bounciness;         // how high particles bounce on ground
    Vector      color;              // color of particle
    Vector      endColor;           // color to morph to
    Vector      colorVelocity;      // change in color over time
    SINGLE      frequency;          // number of new particles per sec
    SINGLE      gravity;            // how particles travel on z axis (up/down)
    S32         lifetime;           // lifetime of effect (ms)
    S32         maxParticles;       // maximum number of particles to be generated 
    S32         meshIndex;          // index of mesh if event uses meshes
    SINGLE      nozzle;             // affects direction of spray of particles - values between 0.0 -> +-infinity
    Vector      nozzleDamp;         // defines how much nozzle affects an axis - 0.0 to 1.0 on each axis 
    SINGLE      nParticles;         // number of particles in effect - will be truncated to a max limit
    U32         partLife;           // lifetime of particle (in ms)
    U32         partLifeRandom;     // % randomness in particle life
    SINGLE      randPosition;       // randomness in world coords of beginning position of particle
    SINGLE      size;               // size of particles
    SINGLE      sizeVelocity;       // speed particles expand or contract
    S32         texture;            // texture of particles 
    C8          textureName[16];    // name of texture file in UTF 
    SINGLE      twistSpeed;         // speed particle cloud rotates
    BOOL32      usesMesh;           // event uses meshes for particles
    SINGLE      velocity;           // speed of particles
    SINGLE      velocityRand;       // amount of randomness in velocity (% from 0.0 to 1.0 )

    //------------ new stuff down here for compatibility ---------------

    U8          dither;             // dither the effect (bool)
    SINGLE      radius;             // max radius of effect over its lifetime
	Vector		direction;			// particle emission direction
	BOOL32		bInherit:1;			// do the particles inherit the emitter velocity upon emission?
	BOOL32		bBound:1;			// are the particles rendered relative to the emitter?
	BOOL32		bIgnoreOrientation:1;	// does the effect ignore orientation?
	BOOL32		bAnimating:1;		// does the effect have an animating texture?
	SINGLE		fps;
	U32			src_blend;			// src blend mode, value is equal to D3DBLEND_ enum values
	U32			dst_blend;			// dst blend mode, value is equal to D3DBLEND_ enum values
	Vector      gravityVec;         // vector gravity long and in the direction which gravity should be applied
	BOOL32		bUseParticleLifetime;	// TRUE == use the particle lifetime to determine whether to render 
										// FALSE == use event lifetime to determine whether to render.
	BOOL32		bFog;				// TRUE to enable fogging, FALSE to disable fogging.

	//KB particle color change stuff
	//this makes the struct quite large... hope this isn't a problem
	U32			PrimaryKeys;		// Actual keys in use indicated by bits
	Vector4		ColorKeyFrames[NUM_COLOR_KEYS];	// Hope this is enough
};


#endif