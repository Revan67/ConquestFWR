#ifndef PFSPAWN_H
#define PFSPAWN_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFSpawn.h                                   //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct SpawnProgramer : public ParticleProgramer
{
	SpawnProgramer();
	~SpawnProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

SpawnProgramer::SpawnProgramer()
{
}

SpawnProgramer::~SpawnProgramer()
{
}

//IParticleProgramer
U32 SpawnProgramer::GetNumOutput()
{
	return 1;
}

const char * SpawnProgramer::GetOutputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 SpawnProgramer::GetNumInput()
{
	return 1;
}

const char * SpawnProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 SpawnProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader);
}

void SpawnProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_SPAWNER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	header->size = offset;
}

void SpawnProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
}

struct SpawnEffect : public IParticleEffect
{
	IParticleEffect * spawn;

	U32 numInst;

	SpawnEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer,U32 inputID);
	virtual ~SpawnEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual U32 OutputRange();
};

SpawnEffect::SpawnEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer,U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	numInst = 1;
	spawn = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
}

SpawnEffect::~SpawnEffect()
{
}

IParticleEffectInstance * SpawnEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(spawn)
		delete spawn;
	spawn = createParticalEffect(parentSystem,type,buffer,this,inputID);
	return spawn;
}

void SpawnEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(spawn)
			delete spawn;
		spawn = (IParticleEffect*)spawn;
		spawn->parentEffect[inputID] = this;
	}
}

void SpawnEffect::Render(SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(spawn)
	{
		for(U32 i = 0; i < numInput; ++i)
		{
			spawn->Render(t,inputStart+i,1,i*(instance+1),parentTrans);
		}
	}
}

bool SpawnEffect::Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
{
	bool bRetVal = false;
	if(spawn)
	{
		for(U32 i = 0; i < numInput; ++i)
		{
			Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
			bool bVal = spawn->Update(inputStart+i,1,bShutdown||(!(p2[i].bLive)),i*(instance+1));
			bRetVal = bRetVal || bVal;
		}
	}
	return bRetVal;
}

U32 SpawnEffect::ParticlesUsed()
{
	U32 parentRange = parentEffect[0]->OutputRange();
	if(spawn)
	{
		spawn->SetInstanceNumber(parentRange*numInst);
		return spawn->ParticlesUsed();
	}
	return 0;
}

void SpawnEffect::FindAllocation(U32 & startPos)
{
	if(spawn)
		spawn->FindAllocation(startPos);
}

void SpawnEffect::DeleteOutput()
{
	if(spawn)
	{
		delete spawn;
		spawn = NULL;
	}
}

void SpawnEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
};

bool SpawnEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(spawn)
		return spawn->GetParentPosition(index,postion,lastIndex);
	return false;
}

U32 SpawnEffect::OutputRange()
{
	return 1;
};

IParticleEffect * SpawnEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(spawn)
		return spawn->FindFilter(searchName);
	return NULL;
}

#endif
