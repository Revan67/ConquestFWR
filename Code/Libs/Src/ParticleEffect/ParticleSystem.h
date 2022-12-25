#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ParticleSystem.h							//
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*

   $Author: Tmauer $
*/

struct Particle
{
	Vector pos;
	Vector vel;
	S32 birthTime;
	S32 lifeTime;
	bool bLive:1;
	bool bNew:1;
	bool bParented:1;
	bool bNoComputeVelocity:1;
};

struct ParticleBatch
{
	Particle * particles1;
	Particle * particles2;
	U16 numAllocated;
	ParticleBatch * next;
};

struct IParticleSystem
{
	virtual struct IParticleEffectInstance * FindFilter(const char * name) = 0;

	virtual Particle * GetParticle(U32 i) = 0;

	virtual DWORD GetContext() = 0;

	virtual U32 StartTime() = 0;

	virtual struct IInternalParticleManager * GetOwner() = 0;

	virtual S32 GetCurrentTimeMS() = 0;

	virtual SINGLE GetRenderTime() = 0;
};

struct IInternalParticleManager
{
	virtual S32 GetRealTimeMS() = 0;

	virtual S32 GetLastUpdateTimeMS() = 0;

	virtual ICamera* GetCamera() = 0;

	virtual IGamePositionCallback * GetCallback() = 0;

	virtual struct IMaterialManager * GetMaterialManager() = 0;

	virtual struct IMeshManager * GetMeshManager() = 0;

	virtual void AddToParticleFreeList(ParticleBatch * old) = 0;

	virtual ParticleBatch * AllocParticles(U32 allocateNumber) = 0;
};

/*

*/
//--------------------------------------------------------------------------//


#endif
